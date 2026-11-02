#ifndef ACTIVITYFEED_H
#define ACTIVITYFEED_H

#include <cstddef>
#include <string>
#include <vector>
#include "observer/WorkflowObserver.h"

// OBSERVER pattern - a CONCRETE OBSERVER.
//
// Keeps a readable, chronological log of everything that happened.
/**
 * @class ActivityFeed
 * @brief Concrete observer that maintains a chronological activity log.
 * 
 * Keeps a readable, chronological log of everything that happened in the workflow
 * by recording human-readable descriptions of workflow events as they occur.
 */
class ActivityFeed : public WorkflowObserver {
private:
    std::vector<std::string> entries_; /*!< Chronological list of recorded event descriptions. */

public:
    /**
     * @brief Handles an incoming workflow event by adding its description to the log.
     * @param[in] event The workflow event to record.
     */
    void onEvent(const WorkflowEvent& event) override;

    /**
     * @brief Gets the list of all recorded activity log entries.
     * @return const std::vector<std::string>& A constant reference to the collection of log entries.
     */
    const std::vector<std::string>& entries() const;

    /**
     * @brief Retrieves the total number of events recorded in the activity feed.
     * @return std::size_t The count of entries.
     */
    std::size_t size() const;
};

#endif // ACTIVITYFEED_H