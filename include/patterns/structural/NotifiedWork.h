#ifndef NOTIFIED_WORK_H
#define NOTIFIED_WORK_H

#include "patterns/structural/WorkItemDecorator.h"
#include <vector>
#include <string>

class NotifiedWork : public WorkItemDecorator {
private:
    std::vector<std::string> watchers;
    void notifyWatchers(const std::string& message);

public:
    NotifiedWork(WorkItem* item, const std::vector<std::string>& watcherList = {});

    void complete() override;
    void reject(const std::string& reason) override;
    void escalate() override;
};

#endif