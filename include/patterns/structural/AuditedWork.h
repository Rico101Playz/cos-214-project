#ifndef AUDITED_WORK_H
#define AUDITED_WORK_H

#include "patterns/structural/WorkItemDecorator.h"
#include <string>

class AuditedWork : public WorkItemDecorator {
public:
    AuditedWork(WorkItem* item);

    void assign(const std::string& participant) override;
    void start() override;
    void complete() override;
    void reject(const std::string& reason) override;
    void escalate() override;
};

#endif