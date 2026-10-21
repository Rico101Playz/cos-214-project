#include "patterns/structural/AuditedWork.h"
#include <iostream>

AuditedWork::AuditedWork(WorkItem* item) : WorkItemDecorator(item) {}

void AuditedWork::assign(const std::string& participant) {
    std::cout << "Audit: assigned to: " << participant << std::endl;
    WorkItemDecorator::assign(participant);
}

void AuditedWork::start() {
    std::cout << "Audit: WorkItem started" << std::endl;
    WorkItemDecorator::start();
}

void AuditedWork::complete() {
    std::cout << "Audit: workItem completed" << std::endl;
    WorkItemDecorator::complete();
}

void AuditedWork::reject(const std::string& reason) {
    std::cout << "Audit: WorkItem rejected. Reason: " << reason << std::endl;
    WorkItemDecorator::reject(reason);
}

void AuditedWork::escalate() {
    std::cout << "Audit: WorkItem escalated." << std::endl;
    WorkItemDecorator::escalate();
}