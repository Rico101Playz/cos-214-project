#include "tui/Controller.h"

#include <algorithm>
#include <stdexcept>

#include "core/WorkItem.h"

namespace tui {

// ---------------------------------------------------------------------------
// Allowed actions per status (UI hint only)
// ---------------------------------------------------------------------------

Allowed allowedFor(const std::string& s) {
    Allowed a;
    if (s == WorkItem::CREATED || s == WorkItem::AVAILABLE) {
        a.assign = true;
    } else if (s == WorkItem::ASSIGNED) {
        a.assign = true;      // reassign
        a.begin = true;
    } else if (s == WorkItem::IN_PROGRESS) {
        a.done = true;
    } else if (s == WorkItem::AWAITING_APPROVAL) {
        a.approve = true;
        a.reject = true;
    } else if (s == WorkItem::REJECTED) {
        a.assign = true;
        a.begin = true;
    }
    return a;
}

// ---------------------------------------------------------------------------
// Observer
// ---------------------------------------------------------------------------

void LogObserver::onEvent(const WorkflowEvent& e) {
   lines.push_back(e.describe());
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

// Runs f (which returns the success message) and converts exceptions into a
// failed Result, using the same invalid/illegal split as the old REPL.
// invalid_argument derives from logic_error, so it must be caught first.
template <class F>
Result attempt(F f) {
    try {
        return Result{true, f()};
    } catch (const std::invalid_argument& e) {
        return Result{false, std::string("[invalid] ") + e.what()};
    } catch (const std::logic_error& e) {
        return Result{false, std::string("[illegal] ") + e.what()};
    } catch (const std::exception& e) {
        return Result{false, std::string("[error] ") + e.what()};
    }
}

Result noInstance() {
    return Result{false, "No instance yet - press n to start one."};
}

void walk(WorkNode* node, int depth, const std::vector<std::string>& ready,
          std::vector<Row>& out) {
    Row r;
    r.id = node->id();
    r.depth = depth;

    WorkItem* item = dynamic_cast<WorkItem*>(node);
    if (item != nullptr) {
        r.isItem = true;
        r.status = item->status();
        r.assignee = item->assignee();
        r.reason = item->reason();
        r.role = item->role();
        r.ready = std::find(ready.begin(), ready.end(), r.id) != ready.end();
    }
    out.push_back(r);

    for (std::size_t i = 0; i < node->childCount(); ++i) {
        walk(node->child(i), depth + 1, ready, out);
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Controller
// ---------------------------------------------------------------------------

Controller::Controller() : engine_("Acme Software") {
    engine_.bus().attach(&log_);

    const char* names[][2] = {
        {"Alice", "Developer"}, {"Bob", "Developer"}, {"Carol", "QA"}};
    for (int i = 0; i < 3; ++i) {
        engine_.organisation().addParticipant(names[i][0], names[i][1]);
        people_.push_back(std::make_pair(std::string(names[i][0]),
                                         std::string(names[i][1])));
    }
}

const std::string& Controller::organisationName() {
    return engine_.organisation().name();
}

std::string Controller::instanceSummary() {
    if (current_.empty()) return "no instance";
    const WorkflowInstance& inst = engine_.instance(current_);
    std::string s = inst.id() + " (" + inst.title() + ")  state: " + inst.state();
    if (!inst.reason().empty()) s += "  reason: " + inst.reason();
    return s;
}

double Controller::progress() {
    return current_.empty() ? 0.0 : engine_.progress(current_);
}

std::vector<Row> Controller::rows() {
    std::vector<Row> out;
    if (current_.empty()) return out;
    const WorkflowInstance& inst = engine_.instance(current_);
    std::vector<std::string> ready = engine_.readyItems(current_);
    walk(&inst.root(), 0, ready, out);
    return out;
}

std::vector<Person> Controller::team() {
    std::vector<Person> out;
    for (std::size_t i = 0; i < people_.size(); ++i) {
        Participant* p = engine_.organisation().find(people_[i].first);
        if (p == nullptr) continue;
        Person person;
        person.name = p->name();
        person.role = p->role();
        person.load = static_cast<int>(p->load());
        out.push_back(person);
    }
    return out;
}

Result Controller::newInstance() {
    return attempt([this]() -> std::string {
        if (!engine_.hasDefinition("delivery")) {
            engine_.loadDefinition("delivery");
        }
        std::string id = engine_.startInstance("delivery");
        instances_.push_back(id);
        current_ = id;
        return "Started " + id;
    });
}

void Controller::nextInstance() {
    if (instances_.size() < 2) return;
    std::size_t i = std::find(instances_.begin(), instances_.end(), current_)
                    - instances_.begin();
    current_ = instances_[(i + 1) % instances_.size()];
}

Result Controller::assign(const std::string& item, const std::string& person) {
    if (current_.empty()) return noInstance();
    return attempt([&]() -> std::string {
        engine_.assign(current_, item, person);
        return item + " assigned to " + person;
    });
}

Result Controller::begin(const std::string& item) {
    if (current_.empty()) return noInstance();
    return attempt([&]() -> std::string {
        engine_.start(current_, item);
        return item + " started";
    });
}

Result Controller::done(const std::string& item) {
    if (current_.empty()) return noInstance();
    return attempt([&]() -> std::string {
        engine_.complete(current_, item);
        return item + " submitted for approval";
    });
}

Result Controller::approve(const std::string& item) {
    if (current_.empty()) return noInstance();
    return attempt([&]() -> std::string {
        engine_.approve(current_, item);
        return item + " approved";
    });
}

Result Controller::reject(const std::string& item, const std::string& reason) {
    if (current_.empty()) return noInstance();
    return attempt([&]() -> std::string {
        engine_.reject(current_, item, reason);
        return item + " rejected";
    });
}

Result Controller::cancel(const std::string& reason) {
    if (current_.empty()) return noInstance();
    return attempt([&]() -> std::string {
        engine_.cancel(current_, reason);
        return current_ + " cancelled";
    });
}

Result Controller::undo() {
    return attempt([&]() -> std::string {
        engine_.undo();
        // The facade header notes snapshots don't cover the work tree yet.
        return "History entry removed (work tree is not restored yet)";
    });
}

}  // namespace tui
