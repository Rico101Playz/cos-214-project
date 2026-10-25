#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

#include "core/WorkflowDefinition.h"
#include "patterns/structural/Tividy.h"
#include "patterns/execution/DepthFirstIterator.h"

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
        "  assign instance-1 \"Task A\" Alice\n"
        "\n";
}

static void printHelp() {
    std::cout <<
        "\n--- definitions ---\n"
        "  load <name>            load the built-in Feature Delivery definition\n"
        "  defs                   list stored definitions\n"
        "\n--- instances ---\n"
        "  start <defName>        start an instance from a definition\n"
        "  show <instanceId>      show the instance's stage tree and items\n"
        "\n--- work items (quote ids with spaces) ---\n"
        "  assign <inst> <item> <participant>\n"
        "  begin  <inst> <item>\n"
        "  done   <inst> <item>\n"
        "  approve <inst> <item>\n"
        "  reject  <inst> <item> <reason>\n"
        "  cancel  <inst> <reason>\n"
        "\n--- inspection ---\n"
        "  ready <inst>           show which items are ready\n"
        "  progress <inst>        show completion percentage\n"
        "  team                   show participants and their loads\n"
        "\n--- history ---\n"
        "  undo                   undo the most recent command\n"
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
        std::cout << indent << "- [" << item->status() << "] "
                  << item->id();
        if (!item->assignee().empty()) {
            std::cout << "  (" << item->assignee() << ")";
        }
        if (!item->reason().empty()) {
            std::cout << "  ! " << item->reason();
        }
        std::cout << "\n";
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
    const char* names[] = {"Alice", "Bob", "Carol"};
    for (int i = 0; i < 3; ++i) {
        Participant* p = engine.organisation().find(names[i]);
        if (p != nullptr) {
            std::cout << "  " << p->name()
                      << " (" << p->role() << ")  load=" << p->load() << "\n";
        }
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
            const char* known[] = {"delivery", "outline"};
            for (int i = 0; i < 2; ++i) {
                if (engine.hasDefinition(known[i])) {
                    std::cout << "  - " << known[i] << "\n";
                }
            }
        }
        else if (cmd == "load") {
            std::string name = readToken(in);
            engine.loadDefinition(name);
            std::cout << "  loaded '" << name << "'\n";
        }
        else if (cmd == "start") {
            std::string defName = readToken(in);
            std::string id = engine.startInstance(defName);
            std::cout << "  started " << id << "\n";
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

int main() {
    Tividy engine("Acme Software");
    engine.organisation().addParticipant("Alice", "Developer");
    engine.organisation().addParticipant("Bob", "Developer");
    engine.organisation().addParticipant("Carol", "QA");

    printBanner();

    bool quit = false;
    std::string line;
    while (!quit) {
        std::cout << "tividy> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        dispatch(engine, line, quit);
    }

    std::cout << "\nbye.\n";
    return 0;
}