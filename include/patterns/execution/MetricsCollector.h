#ifndef METRICSCOLLECTOR_H
#define METRICSCOLLECTOR_H

#include <map>
#include <string>
#include "patterns/execution/WorkflowObserver.h"

// OBSERVER pattern - a CONCRETE OBSERVER.
//
// Counts events by type, so the engine's activity can be reported on.
class MetricsCollector : public WorkflowObserver {
private:
    std::map<std::string, int> counts_;
    int total_;

public:
    MetricsCollector();

    void onEvent(const WorkflowEvent& event) override;

    // How many events of the given type were seen.
    int count(const std::string& type) const;
    int total() const;

    // e.g. "3 events (ItemApproved=2, ItemAssigned=1)"
    std::string summary() const;
};

#endif // METRICSCOLLECTOR_H