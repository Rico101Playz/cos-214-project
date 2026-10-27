#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

#include "core/WorkflowDefinition.h"
#include "patterns/structural/Tividy.h"
#include "patterns/execution/DepthFirstIterator.h"
#include "patterns/structural/LeafWorkItem.h"
#include "server/ActivityLog.h"
#include "server/ScenarioStore.h"

#ifndef TIVIDY_SCENARIOS
#define TIVIDY_SCENARIOS "scenarios"
#endif

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void printBanner() {
    std::cout <<
        "\n"
        "==============================================\n"
        "  TIVIDY  —  Interactive Workflow Engine\n"
        "==============================================\n"
        "Type 'help' for commands, 'quit' to exit.\n"
        "Item ids containing spaces must be quoted, e.g.:\n"
        "  assign instance-1 \"Task A\" Bob\n"
        "\n";
}

static void printHelp() {
    std::cout <<
        "\n--- definitions ---\n"
        "  load <name>            load the built-in Feature Delivery outline\n"
        "  defs                   list stored definitions\n"
        "\n--- instances ---\n"
        "  start <defName> [signoff|nosignoff]  start an instance (sign-off rule optional)\n"
        "  projects               list every project\n"
        "  show <instanceId>      show the instance's stage tree and items\n"
        "  signoff <inst>         owner sign-off of a finished project\n"
        "  flag <inst> <unit> <title>  owner flags an issue for a team's manager\n"
        "\n--- work items (quote ids with spaces) ---\n"
        "  assign <inst> <item> <participant>\n"
        "  begin  <inst> <item>\n"
        "  done   <inst> <item>\n"
        "  approve <inst> <item>\n"
        "  reject  <inst> <item> <reason>\n"
        "  cancel  <inst> <reason>\n"
        "  withdraw <inst> <item> take work back out of approval\n"
        "  priority <inst> <item> <Low|Medium|High|Urgent>\n"
        "  subtask <inst> <item> <title>   add a sub-task and assign it\n"
        "\n--- inspection ---\n"
        "  ready <inst>           show which items are ready\n"
        "  progress <inst>        show completion percentage\n"
        "  team                   show participants and their loads\n"
        "\n--- history ---\n"
        "  undo / redo            undo or redo the most recent action\n"
        "  history                show what undo and redo would do\n"
        "\n--- misc ---\n"
        "  help                   this message\n"
        "  quit                   exit\n"
        "\n";
}

static std::string readToken(std::istringstream& in) {
    in >> std::ws;
    if (!in) return "";

    if (in.peek() == '"') {
        in.get();                      // consume opening quote
        std::string tok;
        std::getline(in, tok, '"');    // read until closing quote
        return tok;
    }

    std::string tok;
    in >> tok;
    return tok;
}

static std::string readRest(std::istringstream& in) {
    std::string rest;
    std::getline(in, rest);
    if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
    return rest;
}

// ---------------------------------------------------------------------------
// Printing
// ---------------------------------------------------------------------------

static void printTree(WorkNode* node, int depth) {
    std::string indent(depth * 2, ' ');
    WorkItem* item = dynamic_cast<WorkItem*>(node);
    if (item != nullptr) {
        LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(item);
        std::cout << indent << "- [" << item->status() << "] "
                  << item->id();
        if (leaf != nullptr && item->name() != item->id()) {
            std::cout << " " << item->name() << " {" << leaf->priority() << "}";
        }
        if (!item->assignee().empty()) {
            std::cout << "  (" << item->assignee() << ")";
        }
        if (!item->reason().empty()) {
            std::cout << "  ! " << item->reason();
        }
        std::cout << "\n";
        for (std::size_t i = 0; i < node->childCount(); ++i) {
            printTree(node->child(i), depth + 1);
        }
        return;
    }
    std::cout << indent << "+ " << node->id() << "\n";
    for (std::size_t i = 0; i < node->childCount(); ++i) {
        printTree(node->child(i), depth + 1);
    }
}

static void printInstance(Tividy& engine, const std::string& id) {
    const WorkflowInstance& inst = engine.instance(id);
    std::cout << "\nInstance: " << inst.id()
              << "  (" << inst.title() << ")\n"
              << "  state: " << inst.state();
    if (!inst.reason().empty()) {
        std::cout << "  reason: " << inst.reason();
    }
    std::cout << "\n\n";

    printTree(&inst.root(), 0);

    std::cout << "\n  progress: " << (engine.progress(id) * 100.0) << "%\n";
}

static void printReady(Tividy& engine, const std::string& id) {
    std::vector<std::string> items = engine.readyItems(id);
    if (items.empty()) {
        std::cout << "  (nothing is ready)\n";
        return;
    }
    for (std::size_t i = 0; i < items.size(); ++i) {
        std::cout << "  - " << items[i] << "\n";
    }
}

static void printTeam(Tividy& engine) {
    std::cout << "\nOrganisation: " << engine.organisation().name() << "\n";
    std::vector<Participant*> people = engine.organisation().participants();
    for (std::size_t i = 0; i < people.size(); ++i) {
        Participant* p = people[i];
        OrgUnit* unit = engine.organisation().unitOf(p->name());
        std::cout << "  " << p->name()
                  << " (" << p->role() << ")  load=" << p->load()
                  << (unit != nullptr ? "  team=" + unit->path() : std::string()) << "\n";
    }
}

// ---------------------------------------------------------------------------
// Command dispatch
// ---------------------------------------------------------------------------

static void dispatch(Tividy& engine, const std::string& line, bool& quit) {
    std::istringstream in(line);
    std::string cmd = readToken(in);

    if (cmd.empty()) return;

    try {
        if (cmd == "help") {
            printHelp();
        }
        else if (cmd == "quit" || cmd == "exit") {
            quit = true;
        }
        else if (cmd == "defs") {
            std::vector<std::string> names = engine.definitionNames();
            for (std::size_t i = 0; i < names.size(); ++i) {
                std::cout << "  - " << names[i] << "\n";
            }
        }
        else if (cmd == "load") {
            std::string name = readToken(in);
            engine.loadDefinition(name);
            std::cout << "  loaded '" << name << "'\n";
        }
        else if (cmd == "start") {
            std::string defName = readToken(in);
            std::string rule = readToken(in);
            std::string id = rule.empty() ? engine.startInstance(defName)
                : engine.startProject(defName, "", "", rule == "signoff" ? SignOffRule::Required : SignOffRule::NotRequired);
            std::cout << "  started " << id << "\n";
        }
        else if (cmd == "projects") {
            std::vector<std::string> ids = engine.instanceIds();
            for (std::size_t i = 0; i < ids.size(); ++i) {
                const WorkflowInstance& inst = engine.instance(ids[i]);
                std::cout << "  " << ids[i] << "  " << inst.name() << "  [" << inst.state() << "]  "
                          << static_cast<int>(engine.progress(ids[i]) * 100.0) << "%\n";
            }
            if (ids.empty()) std::cout << "  (no projects yet)\n";
        }
        else if (cmd == "signoff") {
            std::string inst = readToken(in);
            engine.signOff(inst);
            std::cout << "  signed off\n";
        }
        else if (cmd == "flag") {
            std::string inst = readToken(in);
            IssueSpec spec;
            spec.units.push_back(readToken(in));
            spec.name = readRest(in);
            std::vector<std::string> ids = engine.flagIssue(inst, spec);
            for (std::size_t i = 0; i < ids.size(); ++i) {
                std::cout << "  flagged " << ids[i] << " -> " << engine.instance(inst).item(ids[i])->assignee() << "\n";
            }
        }
        else if (cmd == "withdraw") {
            std::string inst = readToken(in);
            std::string item = readToken(in);
            engine.withdraw(inst, item);
            std::cout << "  taken back out of approval\n";
        }
        else if (cmd == "priority") {
            std::string inst = readToken(in);
            std::string item = readToken(in);
            std::string level = readToken(in);
            engine.setPriority(inst, item, level);
            std::cout << "  priority set\n";
        }
        else if (cmd == "subtask") {
            std::string inst = readToken(in);
            std::string item = readToken(in);
            TaskSpec spec;
            spec.name = readRest(in);
            std::string id = engine.addSubtask(inst, item, spec);
            std::string who = engine.instance(inst).item(id)->assignee();
            std::cout << "  added " << id << (who.empty() ? std::string(" (not assigned yet)") : " -> " + who) << "\n";
        }
        else if (cmd == "redo") {
            engine.redo();
            std::cout << "  redone\n";
        }
        else if (cmd == "history") {
            std::vector<std::string> undo = engine.undoLabels(10);
            std::vector<std::string> redo = engine.redoLabels(10);
            std::cout << "  undo would revert:\n";
            for (std::size_t i = 0; i < undo.size(); ++i) std::cout << "    " << (i + 1) << ". " << undo[i] << "\n";
            if (undo.empty()) std::cout << "    (nothing)\n";
            std::cout << "  redo would reapply:\n";
            for (std::size_t i = 0; i < redo.size(); ++i) std::cout << "    " << (i + 1) << ". " << redo[i] << "\n";
            if (redo.empty()) std::cout << "    (nothing)\n";
        }
        else if (cmd == "show") {
            std::string id = readToken(in);
            printInstance(engine, id);
        }
        else if (cmd == "assign") {
            std::string inst = readToken(in);
            std::string item = readToken(in);
            std::string who  = readToken(in);
            engine.assign(inst, item, who);
            std::cout << "  assigned\n";
        }
        else if (cmd == "begin") {
            std::string inst = readToken(in);
            std::string item = readToken(in);
            engine.start(inst, item);
            std::cout << "  started work\n";
        }
        else if (cmd == "done") {
            std::string inst = readToken(in);
            std::string item = readToken(in);
            engine.complete(inst, item);
            std::cout << "  submitted for approval\n";
        }
        else if (cmd == "approve") {
            std::string inst = readToken(in);
            std::string item = readToken(in);
            engine.approve(inst, item);
            std::cout << "  approved\n";
        }
        else if (cmd == "reject") {
            std::string inst = readToken(in);
            std::string item = readToken(in);
            std::string reason = readRest(in);
            engine.reject(inst, item, reason);
            std::cout << "  rejected: " << reason << "\n";
        }
        else if (cmd == "cancel") {
            std::string inst = readToken(in);
            std::string reason = readRest(in);
            engine.cancel(inst, reason);
            std::cout << "  cancelled\n";
        }
        else if (cmd == "ready") {
            std::string inst = readToken(in);
            printReady(engine, inst);
        }
        else if (cmd == "progress") {
            std::string inst = readToken(in);
            std::cout << "  " << (engine.progress(inst) * 100.0) << "%\n";
        }
        else if (cmd == "team") {
            printTeam(engine);
        }
        else if (cmd == "undo") {
            engine.undo();
            std::cout << "  undone\n";
        }
        else {
            std::cout << "  ? unknown command '" << cmd << "' (try 'help')\n";
        }
    }
    catch (const std::invalid_argument& e) {
        std::cout << "  [invalid] " << e.what() << "\n";
    }
    catch (const std::logic_error& e) {
        std::cout << "  [illegal] " << e.what() << "\n";
    }
    catch (const std::exception& e) {
        std::cout << "  [error] " << e.what() << "\n";
    }
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    std::string scenarios = TIVIDY_SCENARIOS;
    std::string scenario;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--scenarios" && i + 1 < argc) scenarios = argv[++i];
        else if (arg == "--scenario" && i + 1 < argc) scenario = argv[++i];
        else {
            std::cout << "Usage: TIVIDY_App [--scenarios <folder>] [--scenario <file.txt>]\n";
            return arg == "--help" ? 0 : 1;
        }
    }
    if (scenario.empty()) {
        ScenarioStore::ensureDirectory(scenarios);
        scenario = ScenarioMenu::choose(scenarios, std::cin, std::cout);
        if (scenario.empty()) {
            std::cout << "No scenario chosen.\n";
            return 0;
        }
    }

    Tividy engine("Company");
    ActivityLog log(engine);
    engine.bus().attach(&log);
    try {
        ScenarioStore::load(scenario, engine, &log);
    } catch (const std::exception& e) {
        std::cerr << "Could not load " << scenario << ": " << e.what() << "\n";
        return 1;
    }

    printBanner();
    std::cout << "Loaded " << engine.organisation().name() << " from " << scenario
              << ". Every change is saved back to that file.\n\n";

    bool quit = false;
    std::string line;
    unsigned long saved = log.version();
    while (!quit) {
        std::cout << "tividy> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        dispatch(engine, line, quit);
        if (log.version() != saved) {
            saved = log.version();
            try {
                ScenarioStore::save(scenario, engine, &log);
            } catch (const std::exception& e) {
                std::cout << "  [save failed] " << e.what() << "\n";
            }
        }
    }
    engine.bus().detach(&log);

    std::cout << "\nbye.\n";
    return 0;
}