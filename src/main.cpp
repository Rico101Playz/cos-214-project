// TIVIDY terminal UI (FTXUI). Replaces the old REPL main.
// All engine access goes through tui::Controller.

#include <algorithm>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/WorkItem.h"
#include "tui/Controller.h"

using namespace ftxui;

namespace {

enum class Mode { Normal, PickPerson, TypeReason };

Color statusColor(const std::string& s) {
    if (s == WorkItem::ASSIGNED)           return Color::Blue;
    if (s == WorkItem::IN_PROGRESS)        return Color::Yellow;
    if (s == WorkItem::AWAITING_APPROVAL)  return Color::Magenta;
    if (s == WorkItem::COMPLETED)          return Color::Green;
    if (s == WorkItem::REJECTED)           return Color::Red;
    if (s == WorkItem::CANCELLED)          return Color::GrayDark;
    return Color::GrayLight;               // Created / Available
}

Element hint(const char* label, bool enabled) {
    return enabled ? Element(text(label) | bold) : Element(text(label) | dim);
}

Element percentBar(double fraction) {
    float f = static_cast<float>(std::max(0.0, std::min(1.0, fraction)));
    return hbox({gauge(f) | flex,
                 text(" " + std::to_string(static_cast<int>(f * 100)) + "%")});
}

}  // namespace

int main() {
    tui::Controller ctl;
    auto screen = ScreenInteractive::Fullscreen();

    Mode mode = Mode::Normal;
    int sel = 0;                    // selected row in the tree
    int pick = 0;                   // selected person in the assign picker
    std::string input;              // text typed in the reason prompt
    std::string pendingAction;      // "reject" or "cancel"
    std::string pendingItem;        // item the reject applies to
    std::string statusMsg = "Press n to start a workflow instance.";
    bool statusOk = true;

    auto report = [&](const tui::Result& r) {
        statusMsg = r.message;
        statusOk = r.ok;
    };

    // -----------------------------------------------------------------------
    // Rendering
    // -----------------------------------------------------------------------
    auto view = Renderer([&] {
        std::vector<tui::Row> rows = ctl.rows();
        if (rows.empty()) sel = 0;
        else sel = std::max(0, std::min(sel, static_cast<int>(rows.size()) - 1));

        // ---- tree ----
        Elements lines;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const tui::Row& r = rows[i];
            std::string indent(static_cast<std::size_t>(r.depth) * 2, ' ');
            Element line;
            if (r.isItem) {
                line = hbox({
                    text(indent + "- "),
                    text("[" + r.status + "]") | color(statusColor(r.status)),
                    text(" " + r.id),
                    text(r.assignee.empty() ? "" : "  (" + r.assignee + ")") | dim,
                    text(r.ready ? "  * ready" : "") | color(Color::Green),
                });
            } else {
                line = text(indent + "+ " + r.id) | bold;
            }
            if (static_cast<int>(i) == sel) line = line | inverted | focus;
            lines.push_back(line);
        }
        if (lines.empty()) lines.push_back(text("(no instance)") | dim);
        Element tree = window(text(" Work tree "),
                              vbox(std::move(lines)) | vscroll_indicator | yframe) | flex;

        // ---- details ----
        Elements d;
        d.push_back(text(ctl.instanceSummary()));
        d.push_back(percentBar(ctl.progress()));
        d.push_back(separator());
        tui::Allowed allowed;
        if (!rows.empty() && rows[sel].isItem) {
            const tui::Row& r = rows[sel];
            allowed = tui::allowedFor(r.status);
            d.push_back(text(r.id) | bold);
            d.push_back(hbox({text("status:   "),
                              text(r.status) | color(statusColor(r.status))}));
            d.push_back(text("role:     " + r.role));
            d.push_back(text("assignee: " + (r.assignee.empty() ? std::string("-") : r.assignee)));
            if (!r.reason.empty())
                d.push_back(text("reason:   " + r.reason) | color(Color::Red));
        } else {
            d.push_back(text("Select a work item.") | dim);
        }
        d.push_back(separator());
        d.push_back(hbox({hint("[a]ssign ", allowed.assign), hint("[b]egin ", allowed.begin),
                          hint("[d]one ", allowed.done)}));
        d.push_back(hbox({hint("[p]approve ", allowed.approve),
                          hint("[r]eject", allowed.reject)}));
        Element details = window(text(" Details "), vbox(std::move(d)));

        // ---- team ----
        Elements t;
        for (const tui::Person& p : ctl.team()) {
            t.push_back(text(p.name + " (" + p.role + ")  load " + std::to_string(p.load)));
            t.push_back(gauge(std::min(1.0f, p.load / 5.0f)));
        }
        Element team = window(text(" " + ctl.organisationName() + " "), vbox(std::move(t)));

        // ---- activity log (last 7 lines) ----
        const std::vector<std::string>& log = ctl.log();
        Elements l;
        std::size_t from = log.size() > 7 ? log.size() - 7 : 0;
        for (std::size_t i = from; i < log.size(); ++i) l.push_back(text(log[i]));
        if (l.empty()) l.push_back(text("(no events yet)") | dim);
        Element activity = window(text(" Activity "), vbox(std::move(l))) |
                           size(HEIGHT, EQUAL, 9);

        Element title = text(" TIVIDY  -  Workflow Engine ") | bold | inverted;
        Element keys = text(" arrows move | n new | tab switch | c cancel | u undo | q quit ") | dim;
        Element status = text(" " + statusMsg) |
                         color(statusOk ? Color::Green : Color::Red);

        Element main = vbox({
            title,
            hbox({tree, vbox({details, team}) | size(WIDTH, EQUAL, 46)}) | flex,
            activity,
            status,
            keys,
        });

        // ---- modals ----
        if (mode == Mode::PickPerson) {
            Elements p;
            std::vector<tui::Person> people = ctl.team();
            for (std::size_t i = 0; i < people.size(); ++i) {
                Element e = text(" " + people[i].name + " (" + people[i].role + ") ");
                if (static_cast<int>(i) == pick) e = e | inverted;
                p.push_back(e);
            }
            Element modal = window(text(" Assign to (Enter / Esc) "), vbox(std::move(p))) |
                            size(WIDTH, GREATER_THAN, 32) | clear_under | center;
            return dbox({main, modal});
        }
        if (mode == Mode::TypeReason) {
            Element modal = window(text(" Reason (Enter = ok, Esc = back) "),
                                   text(" " + input + "_ ")) |
                            size(WIDTH, GREATER_THAN, 50) | clear_under | center;
            return dbox({main, modal});
        }
        return main;
    });

    // -----------------------------------------------------------------------
    // Input handling
    // -----------------------------------------------------------------------
    auto app = CatchEvent(view, [&](Event e) {
        std::vector<tui::Row> rows = ctl.rows();
        int count = static_cast<int>(rows.size());
        bool haveItem = count > 0 && sel < count && rows[sel].isItem;

        if (mode == Mode::PickPerson) {
            std::vector<tui::Person> people = ctl.team();
            int n = static_cast<int>(people.size());
            if (e == Event::Escape)    { mode = Mode::Normal; }
            else if (e == Event::ArrowUp)   { pick = std::max(0, pick - 1); }
            else if (e == Event::ArrowDown) { pick = std::min(n - 1, pick + 1); }
            else if (e == Event::Return) {
                if (haveItem && n > 0) report(ctl.assign(rows[sel].id, people[pick].name));
                mode = Mode::Normal;
            }
            return true;    // modal swallows everything
        }

        if (mode == Mode::TypeReason) {
            if (e == Event::Escape) {
                mode = Mode::Normal;
            } else if (e == Event::Return) {
                if (pendingAction == "reject") report(ctl.reject(pendingItem, input));
                else                           report(ctl.cancel(input));
                mode = Mode::Normal;
            } else if (e == Event::Backspace) {
                if (!input.empty()) input.pop_back();   // ASCII only
            } else if (e.is_character()) {
                input += e.character();
            }
            return true;
        }

        // ---- normal mode ----
        if (e == Event::ArrowUp)   { sel = std::max(0, sel - 1); return true; }
        if (e == Event::ArrowDown) { sel = std::min(std::max(0, count - 1), sel + 1); return true; }
        if (e == Event::Tab)       { ctl.nextInstance(); sel = 0; return true; }
        if (e == Event::Character('q')) { screen.ExitLoopClosure()(); return true; }
        if (e == Event::Character('n')) { report(ctl.newInstance()); sel = 0; return true; }
        if (e == Event::Character('u')) { report(ctl.undo()); return true; }
        if (e == Event::Character('c')) {
            pendingAction = "cancel"; input.clear(); mode = Mode::TypeReason; return true;
        }

        bool itemKey = e == Event::Character('a') || e == Event::Character('b') ||
                       e == Event::Character('d') || e == Event::Character('p') ||
                       e == Event::Character('r');
        if (itemKey && !haveItem) {
            report(tui::Result{false, "Select a work item first."});
            return true;
        }
        if (e == Event::Character('a')) { pick = 0; mode = Mode::PickPerson; return true; }
        if (e == Event::Character('b')) { report(ctl.begin(rows[sel].id));   return true; }
        if (e == Event::Character('d')) { report(ctl.done(rows[sel].id));    return true; }
        if (e == Event::Character('p')) { report(ctl.approve(rows[sel].id)); return true; }
        if (e == Event::Character('r')) {
            pendingAction = "reject"; pendingItem = rows[sel].id;
            input.clear(); mode = Mode::TypeReason; return true;
        }
        return false;
    });

    screen.Loop(app);
    return 0;
}
