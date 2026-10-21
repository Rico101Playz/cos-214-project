#ifndef PRIORITY_WORK_H
#define PRIORITY_WORK_H

#include "patterns/structural/WorkItemDecorator.h"

class PriorityWork : public WorkItemDecorator {
private:
    bool urgent;

public:
    PriorityWork(WorkItem* item, bool isUrgentFlag = true);
    bool isUrgent() const;
};

#endif