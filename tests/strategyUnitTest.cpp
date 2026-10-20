#include <iostream>
#include <string>

#include "test_utils.h"

#include "core/Participant.h"

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
// MAIN
// ============================================================================
void run_strategy_tests() {
    std::cout << "\n--- Running Strategy (assignment) Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_Participant();

    std::cout << "Strategy Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}
