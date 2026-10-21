#include "patterns/structural/SecuredWork.h"
#include <iostream>

SecuredWork::SecuredWork(WorkItem* item, std::string clearance)
    : WorkItemDecorator(item), clearanceLevel(clearance) {}

void SecuredWork::assign(const std::string& participant) {
    if (participant.empty()) {
        std::cout << "SecuredWork: Assignment DENIED: empty participant." << std::endl;
        return;
    }
    std::cout << "SecuredWork: Verified clearance level " << clearanceLevel << " for " << participant << std::endl;
    WorkItemDecorator::assign(participant);
}

std::string SecuredWork::requiredClearance() const {
    return clearanceLevel;
}