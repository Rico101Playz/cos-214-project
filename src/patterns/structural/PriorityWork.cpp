#include "patterns/structural/PriorityWork.h"

PriorityWork::PriorityWork(WorkItem* item, bool isUrgentFlag)
    : WorkItemDecorator(item), urgent(isUrgentFlag) {}

bool PriorityWork::isUrgent() const {
    return urgent;
}