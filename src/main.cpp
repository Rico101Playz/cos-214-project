#include <iostream>
#include <string>
#include <vector>
#include "core/WorkflowDefinition.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/Tividy.h"

// Prints what can be picked up right now and how far the instance is.
static void report(Tividy& tividy, const std::string& id) {
    const WorkflowInstance& instance = tividy.instance(id);
    std::cout << "  [" << instance.state() << "] "
              << static_cast<int>(tividy.progress(id) * 100) << "% done, ready:";
    std::vector<std::string> ready = tividy.readyItems(id);
    if (ready.empty()) {
        std::cout << " (nothing)";
    }
    for (std::size_t i = 0; i < ready.size(); ++i) {
        std::cout << " " << ready[i];
    }
    std::cout << "\n";
}

// Assigns, starts, completes and approves one item through the facade.
static void deliver(Tividy& tividy, const std::string& id, const std::string& item,
                    const std::string& participant) {
    tividy.assign(id, item, participant);
    tividy.start(id, item);
    tividy.complete(id, item);
    tividy.approve(id, item);
    std::cout << "  " << participant << " delivered " << item << "\n";
}

int main() {
    std::cout << "TIVIDY Workflow Management System\n\n";

    Tividy tividy("Acme Software");
    tividy.organisation().addParticipant("Alice", "Developer");
    tividy.organisation().addParticipant("Bob", "Reviewer");
    tividy.organisation().addParticipant("Erin", "ReleaseManager");

    WorkflowDefinition delivery;
    delivery.name = "Software Feature Delivery";
    delivery.stages.push_back("Delivery");
    delivery.items.push_back("Implement feature");
    delivery.items.push_back("Code review");
    delivery.items.push_back("Deploy");
    tividy.loadDefinition("feature-delivery", delivery);

    std::string id = tividy.startInstance("feature-delivery");
    std::cout << "Started " << id << ": " << tividy.instance(id).title() << "\n";
    report(tividy, id);

    deliver(tividy, id, "Implement feature", "Alice");
    report(tividy, id);

    tividy.assign(id, "Code review", "Bob");
    tividy.start(id, "Code review");
    tividy.complete(id, "Code review");
    tividy.reject(id, "Code review", "Missing tests");
    std::cout << "  Code review rejected: missing tests\n";
    report(tividy, id);

    deliver(tividy, id, "Code review", "Bob");
    deliver(tividy, id, "Deploy", "Erin");
    report(tividy, id);

    return 0;
}
