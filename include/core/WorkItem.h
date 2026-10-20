#ifndef WORKITEM_H
#define WORKITEM_H

#include <string>

class WorkItem {
public:
    std::string itemId;
    std::string state;
    std::string assignee;

    WorkItem(std::string id) : itemId(std::move(id)), state("Pending") {}
};
#endif // WORKITEM_H