#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "test_utils.h"

#include "core/Participant.h"
#include "core/Organisation.h"
#include "core/WorkItem.h"
#include "patterns/execution/AssignmentStrategy.h"
#include "patterns/execution/RoleBasedAssignment.h"
#include "patterns/execution/LeastLoadedAssignment.h"
#include "patterns/execution/RuleBasedAssignment.h"
#include "patterns/execution/AssignmentService.h"
#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/creation/FeatureDeliveryProcess.h"
#include "core/WorkflowDefinition.h"

// ============================================================================
// TEST: Participant
// ============================================================================
void test_Participant() {
    std::cout << "\n--- Testing Participant ---\n";

    Participant alice("Alice", "Developer");

    check(alice.name() == "Alice", "Participant: stores its name");
    check(alice.role() == "Developer", "Participant: stores its role");
    check(alice.load() == 0, "Participant: starts with no load");
    check(alice.isAvailable() == true, "Participant: starts available");

    alice.addLoad(2);
    check(alice.load() == 2, "Participant: addLoad() increases the load");

    alice.addLoad(-1);
    check(alice.load() == 1, "Participant: addLoad() with a negative delta decreases the load");

    alice.addLoad(-10);
    check(alice.load() == 0, "Participant: load never drops below zero");

    alice.setAvailable(false);
    check(alice.isAvailable() == false, "Participant: setAvailable(false) marks them unavailable");
}

// ============================================================================
// TEST: Organisation
// ============================================================================
void test_Organisation() {
    std::cout << "\n--- Testing Organisation ---\n";

    Organisation org("Acme Software");
    check(org.name() == "Acme Software", "Organisation: stores its name");
    check(org.size() == 0, "Organisation: starts with no participants");

    Participant& alice = org.addParticipant("Alice", "Developer");
    org.addParticipant("Bob", "Developer");
    Participant& carol = org.addParticipant("Carol", "Reviewer");

    check(org.size() == 3, "Organisation: addParticipant() adds participants");
    check(alice.name() == "Alice" && alice.role() == "Developer",
          "Organisation: addParticipant() returns the new participant");
    check(org.find("Carol") == &carol, "Organisation: find() returns the stored participant");
    check(org.find("Nobody") == nullptr, "Organisation: find() returns nullptr for an unknown name");

    bool threw = false;
    try {
        org.addParticipant("Alice", "Reviewer");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "Organisation: a duplicate name is rejected");
    check(org.size() == 3, "Organisation: a rejected duplicate is not stored");

    Participant* firstPtr = org.find("Alice");
    for (int i = 0; i < 50; ++i) {
        org.addParticipant("Extra" + std::to_string(i), "Developer");
    }
    check(org.find("Alice") == firstPtr,
          "Organisation: participant addresses stay valid as the organisation grows");

    // eligible(): same role AND available, in registration order
    Organisation team("Team");
    team.addParticipant("Dev1", "Developer");
    team.addParticipant("Rev1", "Reviewer");
    team.addParticipant("Dev2", "Developer");

    std::vector<Participant*> devs = team.eligible("Developer");
    check(devs.size() == 2, "Organisation: eligible() finds everyone with the role");
    check(devs[0]->name() == "Dev1" && devs[1]->name() == "Dev2",
          "Organisation: eligible() keeps registration order");

    team.find("Dev1")->setAvailable(false);
    devs = team.eligible("Developer");
    check(devs.size() == 1 && devs[0]->name() == "Dev2",
          "Organisation: eligible() skips unavailable participants");

    check(team.eligible("Release Manager").empty(),
          "Organisation: eligible() is empty when nobody has the role");
}

// ============================================================================
// TEST: WorkItem (minimal core item)
// ============================================================================
void test_WorkItem() {
    std::cout << "\n--- Testing WorkItem ---\n";

    WorkItem backend("F06", "Implement backend", "Developer");

    check(backend.id() == "F06", "WorkItem: stores its id");
    check(backend.name() == "Implement backend", "WorkItem: stores its name");
    check(backend.role() == "Developer", "WorkItem: stores the role that should do it");

    check(backend.hasData("risk") == false, "WorkItem: has no data initially");
    check(backend.getData("risk") == "", "WorkItem: missing data defaults to an empty string");
    check(backend.getData("risk", "normal") == "normal", "WorkItem: missing data uses the given fallback");

    backend.setData("risk", "high");
    check(backend.hasData("risk") == true, "WorkItem: setData() stores a value");
    check(backend.getData("risk", "normal") == "high", "WorkItem: getData() returns the stored value");

    backend.setData("risk", "low");
    check(backend.getData("risk") == "low", "WorkItem: setData() overwrites an existing value");
}

// ============================================================================
// SCENARIO HELPER: the Software Feature Delivery team (Task 2 scenario)
// ============================================================================
static void buildFeatureDeliveryTeam(Organisation& org) {
    org.addParticipant("Pia", "Product Owner");
    org.addParticipant("Alice", "Developer");
    org.addParticipant("Bob", "Developer");
    org.addParticipant("Cara", "Developer");
    org.addParticipant("Rita", "Reviewer");
    org.addParticipant("Sam", "QA Tester");
    org.addParticipant("Tess", "Tech Lead");
    org.addParticipant("Rex", "Release Manager");
}

// ============================================================================
// TEST: RoleBasedAssignment
// ============================================================================
void test_RoleBasedAssignment() {
    std::cout << "\n--- Testing RoleBasedAssignment ---\n";

    Organisation org("Acme Software");
    buildFeatureDeliveryTeam(org);
    WorkItem backend("F06", "Implement backend", "Developer");

    RoleBasedAssignment roleBased;
    const AssignmentStrategy& strategy = roleBased;   // used through the interface

    check(strategy.name() == "RoleBased", "RoleBased: reports its name");

    Participant* chosen = strategy.assign(backend, backend.role(), org);
    check(chosen != nullptr && chosen->name() == "Alice",
          "RoleBased: picks the first available participant with the role");

    org.find("Alice")->addLoad(5);
    chosen = strategy.assign(backend, backend.role(), org);
    check(chosen != nullptr && chosen->name() == "Alice",
          "RoleBased: ignores workload (still Alice although she is busiest)");
    check(org.find("Alice")->load() == 5,
          "RoleBased: choosing does not change anyone's load");

    org.find("Alice")->setAvailable(false);
    chosen = strategy.assign(backend, backend.role(), org);
    check(chosen != nullptr && chosen->name() == "Bob",
          "RoleBased: skips an unavailable participant");

    chosen = strategy.assign(backend, "Reviewer", org);
    check(chosen != nullptr && chosen->name() == "Rita",
          "RoleBased: uses the role it is given, not the item's own role");

    org.find("Rex")->setAvailable(false);
    chosen = strategy.assign(backend, "Release Manager", org);
    check(chosen == nullptr, "RoleBased: returns nullptr when nobody is eligible");

    chosen = strategy.assign(backend, "Astronaut", org);
    check(chosen == nullptr, "RoleBased: returns nullptr for a role nobody holds");
}

// ============================================================================
// TEST: LeastLoadedAssignment
// ============================================================================
void test_LeastLoadedAssignment() {
    std::cout << "\n--- Testing LeastLoadedAssignment ---\n";

    Organisation org("Acme Software");
    buildFeatureDeliveryTeam(org);
    WorkItem frontend("F07", "Implement frontend", "Developer");

    LeastLoadedAssignment leastLoaded;
    const AssignmentStrategy& strategy = leastLoaded;

    check(strategy.name() == "LeastLoaded", "LeastLoaded: reports its name");

    Participant* chosen = strategy.assign(frontend, frontend.role(), org);
    check(chosen != nullptr && chosen->name() == "Alice",
          "LeastLoaded: on equal load the first registered participant wins");

    org.find("Alice")->addLoad(3);
    org.find("Bob")->addLoad(1);
    chosen = strategy.assign(frontend, frontend.role(), org);
    check(chosen != nullptr && chosen->name() == "Cara",
          "LeastLoaded: picks the participant with the lowest load");

    org.find("Cara")->setAvailable(false);
    chosen = strategy.assign(frontend, frontend.role(), org);
    check(chosen != nullptr && chosen->name() == "Bob",
          "LeastLoaded: an unavailable participant is not chosen even with the lowest load");

    check(strategy.assign(frontend, "Release Manager", org)->name() == "Rex",
          "LeastLoaded: works for a role with a single holder");
    check(strategy.assign(frontend, "Astronaut", org) == nullptr,
          "LeastLoaded: returns nullptr for a role nobody holds");

    // Same team, same item: the two strategies genuinely differ.
    RoleBasedAssignment roleBased;
    org.find("Cara")->setAvailable(true);
    check(roleBased.assign(frontend, "Developer", org)->name() == "Alice" &&
              strategy.assign(frontend, "Developer", org)->name() == "Cara",
          "LeastLoaded: gives a different answer to RoleBased for the same item and team");

    // A busy team: handing out items one after another spreads the work.
    Organisation busy("Busy Team");
    buildFeatureDeliveryTeam(busy);
    std::string order;
    for (int i = 0; i < 4; ++i) {
        WorkItem item("T" + std::to_string(i), "Task", "Developer");
        Participant* p = strategy.assign(item, item.role(), busy);
        p->addLoad(1);          // the caller records the assignment
        order += p->name();
        order += " ";
    }
    check(order == "Alice Bob Cara Alice ",
          "LeastLoaded: successive assignments rotate through the team as load builds");
}

// ============================================================================
// TEST: RuleBasedAssignment
// ============================================================================
void test_RuleBasedAssignment() {
    std::cout << "\n--- Testing RuleBasedAssignment ---\n";

    Organisation org("Acme Software");
    buildFeatureDeliveryTeam(org);

    // Feature Delivery policy: high-risk reviews go to the Tech Lead.
    RuleBasedAssignment rules(std::unique_ptr<AssignmentStrategy>(new LeastLoadedAssignment()));
    rules.addRule("Reviewer", "risk", "high", "Tech Lead");
    const AssignmentStrategy& strategy = rules;

    check(strategy.name() == "RuleBased(LeastLoaded)", "RuleBased: name includes its fallback");

    WorkItem normalReview("F11", "Code review", "Reviewer");
    normalReview.setData("risk", "normal");
    Participant* chosen = strategy.assign(normalReview, normalReview.role(), org);
    check(chosen != nullptr && chosen->name() == "Rita",
          "RuleBased: with no matching rule the fallback picks for the original role");

    WorkItem noRiskData("F11b", "Code review", "Reviewer");
    chosen = strategy.assign(noRiskData, noRiskData.role(), org);
    check(chosen != nullptr && chosen->name() == "Rita",
          "RuleBased: an item without the data key matches no rule");

    WorkItem securityReview("F16", "Security review", "Reviewer");
    securityReview.setData("risk", "high");
    chosen = strategy.assign(securityReview, securityReview.role(), org);
    check(chosen != nullptr && chosen->name() == "Tess",
          "RuleBased: a high-risk review is routed to the Tech Lead");

    WorkItem riskyBackend("F06", "Implement backend", "Developer");
    riskyBackend.setData("risk", "high");
    chosen = strategy.assign(riskyBackend, riskyBackend.role(), org);
    check(chosen != nullptr && chosen->name() == "Alice",
          "RuleBased: a rule only applies to the role it names (developers are not rerouted)");

    org.find("Tess")->setAvailable(false);
    chosen = strategy.assign(securityReview, securityReview.role(), org);
    check(chosen == nullptr,
          "RuleBased: high-risk work does not fall back to an ordinary reviewer");
    org.find("Tess")->setAvailable(true);

    // The fallback still decides WHO among the rerouted role's holders.
    Organisation twoLeads("Two Leads");
    twoLeads.addParticipant("Tess", "Tech Lead");
    twoLeads.addParticipant("Tom", "Tech Lead");
    twoLeads.find("Tess")->addLoad(2);
    chosen = strategy.assign(securityReview, securityReview.role(), twoLeads);
    check(chosen != nullptr && chosen->name() == "Tom",
          "RuleBased: the fallback (least loaded) chooses between several Tech Leads");

    // Order matters: the first matching rule wins.
    RuleBasedAssignment ordered(std::unique_ptr<AssignmentStrategy>(new RoleBasedAssignment()));
    ordered.addRule("Reviewer", "risk", "high", "Tech Lead");
    ordered.addRule("Reviewer", "risk", "high", "Release Manager");
    chosen = ordered.assign(securityReview, securityReview.role(), org);
    check(chosen != nullptr && chosen->name() == "Tess",
          "RuleBased: the first matching rule wins");

    // A missing fallback defaults to RoleBasedAssignment.
    RuleBasedAssignment defaulted((std::unique_ptr<AssignmentStrategy>()));
    check(defaulted.name() == "RuleBased(RoleBased)", "RuleBased: a null fallback defaults to RoleBased");
}

// ============================================================================
// TEST: AssignmentService (the Strategy context)
// ============================================================================
void test_AssignmentService() {
    std::cout << "\n--- Testing AssignmentService ---\n";

    Organisation org("Acme Software");
    buildFeatureDeliveryTeam(org);
    AssignmentService service(org);
    WorkItem backend("F06", "Implement backend", "Developer");

    check(service.strategy().name() == "RoleBased", "AssignmentService: starts with RoleBased");
    check(service.chooseAssignee(backend, backend.role())->name() == "Alice",
          "AssignmentService: delegates to the current strategy");

    // The team gets busier: swap the policy while the workflow is running.
    org.find("Alice")->addLoad(4);
    service.setStrategy(std::unique_ptr<AssignmentStrategy>(new LeastLoadedAssignment()));
    check(service.strategy().name() == "LeastLoaded", "AssignmentService: setStrategy() swaps the policy");
    check(service.chooseAssignee(backend, backend.role())->name() == "Bob",
          "AssignmentService: the same request now gets a different answer");

    // Swap again to a composed strategy.
    RuleBasedAssignment* rules = new RuleBasedAssignment(std::unique_ptr<AssignmentStrategy>(new LeastLoadedAssignment()));
    rules->addRule("Reviewer", "risk", "high", "Tech Lead");
    service.setStrategy(std::unique_ptr<AssignmentStrategy>(rules));

    WorkItem securityReview("F16", "Security review", "Reviewer");
    securityReview.setData("risk", "high");
    check(service.chooseAssignee(securityReview, securityReview.role())->name() == "Tess",
          "AssignmentService: works with a composed rule-based strategy");

    // A null strategy is refused and the current one is kept.
    bool threw = false;
    try {
        service.setStrategy(std::unique_ptr<AssignmentStrategy>());
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "AssignmentService: a null strategy is rejected");
    check(service.strategy().name() == "RuleBased(LeastLoaded)",
          "AssignmentService: a rejected strategy leaves the current one in place");

    // The service reads the organisation it was given, so later changes are seen.
    org.addParticipant("Dina", "Designer");
    check(service.chooseAssignee(backend, "Designer")->name() == "Dina",
          "AssignmentService: sees participants added to the organisation later");

    check(service.chooseAssignee(backend, "Astronaut") == nullptr,
          "AssignmentService: returns nullptr when nobody is eligible");

    int loadBefore = org.find("Bob")->load();
    service.chooseAssignee(backend, "Developer");
    check(org.find("Bob")->load() == loadBefore,
          "AssignmentService: choosing an assignee does not change anyone's load");
}

// ============================================================================
// TEST: Scenario - Feature Delivery (Builder output + Strategy together)
// ============================================================================
void test_FeatureDeliveryScenario() {
    std::cout << "\n--- Testing Feature Delivery scenario (Builder + Strategy) ---\n";

    // 1. The Builder (already on dev) produces the Feature Delivery definition.
    ConcreteDefinitionBuilder builder;
    FeatureDeliveryProcess process;
    process.construct(&builder);
    WorkflowDefinition definition = builder.build();
    check(definition.items.size() == 2, "Scenario: the Builder's definition provides work items to assign");

    // 2. Each item in the definition becomes a work item for a Developer, and the
    //    team's policy decides who gets it. Busy team => least loaded.
    Organisation org("Acme Software");
    buildFeatureDeliveryTeam(org);
    AssignmentService service(org);
    service.setStrategy(std::unique_ptr<AssignmentStrategy>(new LeastLoadedAssignment()));

    std::string leastLoadedOrder;
    for (std::size_t i = 0; i < definition.items.size(); ++i) {
        WorkItem item("I" + std::to_string(i), definition.items[i], "Developer");
        Participant* p = service.chooseAssignee(item, item.role());
        p->addLoad(1);   // recording the assignment is the caller's job
        leastLoadedOrder += p->name() + " ";
    }
    check(leastLoadedOrder == "Alice Bob ",
          "Scenario: least-loaded spreads the definition's items across developers");

    // 3. A small team uses the simple policy instead: the same items all go to one person.
    Organisation smallTeam("Small Team");
    buildFeatureDeliveryTeam(smallTeam);
    AssignmentService smallService(smallTeam);   // default RoleBased
    std::string roleBasedOrder;
    for (std::size_t i = 0; i < definition.items.size(); ++i) {
        WorkItem item("I" + std::to_string(i), definition.items[i], "Developer");
        Participant* p = smallService.chooseAssignee(item, item.role());
        p->addLoad(1);
        roleBasedOrder += p->name() + " ";
    }
    check(roleBasedOrder == "Alice Alice ",
          "Scenario: the same definition under role-based assignment piles on the first developer");

    // 4. Walk the work items of the Feature Delivery activity diagram (F06 - F20).
    Organisation company("Acme Software");
    buildFeatureDeliveryTeam(company);
    AssignmentService delivery(company);
    RuleBasedAssignment* policy = new RuleBasedAssignment(
        std::unique_ptr<AssignmentStrategy>(new LeastLoadedAssignment()));
    policy->addRule("Reviewer", "risk", "high", "Tech Lead");   // F16: high-risk goes to the Tech Lead
    delivery.setStrategy(std::unique_ptr<AssignmentStrategy>(policy));

    WorkItem f06("F06", "Implement backend and unit tests", "Developer");
    WorkItem f07("F07", "Implement frontend and unit tests", "Developer");
    WorkItem f08("F08", "Write feature and release documentation", "Developer");
    WorkItem f11("F11", "Obtain first code approval", "Reviewer");
    WorkItem f15("F15", "QA verifies acceptance criteria", "QA Tester");
    WorkItem f16("F16", "Security review", "Reviewer");
    f16.setData("risk", "high");
    WorkItem f20("F20", "Release Manager approval", "Release Manager");

    std::string devOrder;
    WorkItem* parallelWork[3] = { &f06, &f07, &f08 };   // the F06/F07/F08 parallel fork
    for (int i = 0; i < 3; ++i) {
        Participant* p = delivery.chooseAssignee(*parallelWork[i], parallelWork[i]->role());
        p->addLoad(1);
        devOrder += p->name() + " ";
    }
    check(devOrder == "Alice Bob Cara ",
          "Scenario: the three parallel implementation items go to three different developers");

    check(delivery.chooseAssignee(f11, f11.role())->name() == "Rita", "Scenario: F11 code review goes to the Reviewer");
    check(delivery.chooseAssignee(f15, f15.role())->name() == "Sam", "Scenario: F15 QA verification goes to QA");
    check(delivery.chooseAssignee(f16, f16.role())->name() == "Tess",
          "Scenario: F16 high-risk security review is routed to the Tech Lead");

    // 5. Activity A04: nobody holds the role right now, so the item waits.
    company.find("Rex")->setAvailable(false);
    check(delivery.chooseAssignee(f20, f20.role()) == nullptr,
          "Scenario: F20 has no eligible Release Manager, so assignment waits (A04)");
    company.find("Rex")->setAvailable(true);
    check(delivery.chooseAssignee(f20, f20.role())->name() == "Rex",
          "Scenario: once the Release Manager is available F20 is assigned");
}

// ============================================================================
// MAIN
// ============================================================================
void run_strategy_tests() {
    std::cout << "\n--- Running Strategy (assignment) Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_Participant();
    test_Organisation();
    test_WorkItem();
    test_RoleBasedAssignment();
    test_LeastLoadedAssignment();
    test_RuleBasedAssignment();
    test_AssignmentService();
    test_FeatureDeliveryScenario();

    std::cout << "Strategy Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}
