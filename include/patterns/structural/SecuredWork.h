#ifndef SECURED_WORK_H
#define SECURED_WORK_H

#include "patterns/structural/WorkItemDecorator.h"
#include <string>

class SecuredWork : public WorkItemDecorator {
private:
    std::string clearanceLevel;

public:
    SecuredWork(WorkItem* item, std::string clearance = "Standard");

    void assign(const std::string& participant) override;
    std::string requiredClearance() const;
};

#endif