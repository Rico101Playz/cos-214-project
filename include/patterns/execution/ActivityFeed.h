#ifndef ACTIVITYFEED_H
#define ACTIVITYFEED_H

#include <cstddef>
#include <string>
#include <vector>
#include "patterns/execution/WorkflowObserver.h"

// OBSERVER pattern - a CONCRETE OBSERVER.
//
// Keeps a readable, chronological log of everything that happened.
class ActivityFeed : public WorkflowObserver {
private:
    std::vector<std::string> entries_;

public:
    void onEvent(const WorkflowEvent& event) override;

    const std::vector<std::string>& entries() const;
    std::size_t size() const;
};

#endif // ACTIVITYFEED_H