#ifndef VALIDATED_WORK_H
#define VALIDATED_WORK_H

#include "patterns/structural/WorkItemDecorator.h"
#include <vector>
#include <string>

class ValidatedWork : public WorkItemDecorator {
private:
    std::vector<std::string> requiredKeys;

public:
    ValidatedWork(WorkItem* item, const std::vector<std::string>& keys = {});
    void complete() override;
};

#endif