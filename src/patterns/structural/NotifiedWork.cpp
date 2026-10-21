#include "patterns/structural/NotifiedWork.h"
#include <iostream>

NotifiedWork::NotifiedWork(WorkItem* item, const std::vector<std::string>& watcherList)
    : WorkItemDecorator(item), watchers(watcherList) {}

void NotifiedWork::notifyWatchers(const std::string& message) {
    for (size_t i = 0; i < watchers.size(); ++i) {
        std::cout << "Notification to " << watchers[i] << ": " << message << std::endl;
    }
}

//above
void NotifiedWork::complete() {
    WorkItemDecorator::complete();
    notifyWatchers("WorkItem completed.");
}

void NotifiedWork::reject(const std::string& reason) {
    WorkItemDecorator::reject(reason);
    notifyWatchers("WorkItem rejected: " + reason);
}

void NotifiedWork::escalate() {
    WorkItemDecorator::escalate();
    notifyWatchers("WorkItem escalated.");
}