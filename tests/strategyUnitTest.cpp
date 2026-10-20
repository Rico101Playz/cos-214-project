#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "test_utils.h"

#include "core/Participant.h"
#include "core/Organisation.h"

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
// MAIN
// ============================================================================
void run_strategy_tests() {
    std::cout << "\n--- Running Strategy (assignment) Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_Participant();
    test_Organisation();

    std::cout << "Strategy Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}
