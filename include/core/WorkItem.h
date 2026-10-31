#ifndef WORKITEM_H
#define WORKITEM_H

#include "patterns/structural/WorkNode.h"
#include <map>
#include <string>
#include <utility>

// A single unit of work, e.g. "Implement backend" or "Code review".
//
// NOTE (integration point): this is the minimal shape the assignment
// strategies and approval chain need - an id, a name, the ROLE that should do
// the work, and key/value data (e.g. risk = high). The class diagram grows this
// into WorkNode -> WorkItem -> LeafWorkItem (Composite) with a lifecycle
// (State). Whoever lands those patterns should extend this class and keep
// id(), name(), role(), setData(), hasData() and getData() working, so the
// strategies and approval handlers do not need to change.
/**
 * @class WorkItem
 * @brief A single unit of work, e.g. "Implement backend" or "Code review".
 * 
 * Acts as the base node for workflows and maintains an internal state lifecycle 
 * (Created, Assigned, InProgress, etc.) while holding dynamic key/value data 
 * required by assignment strategies and approval chains.
 */
class WorkItem : public WorkNode {
public:
    static const char* const CREATED;           /*!< String constant representing the Created state. */
    static const char* const AVAILABLE;         /*!< String constant representing the Available state. */
    static const char* const ASSIGNED;          /*!< String constant representing the Assigned state. */
    static const char* const IN_PROGRESS;       /*!< String constant representing the InProgress state. */
    static const char* const AWAITING_APPROVAL; /*!< String constant representing the AwaitingApproval state. */
    static const char* const COMPLETED;         /*!< String constant representing the Completed state. */
    static const char* const REJECTED;          /*!< String constant representing the Rejected state. */
    static const char* const CANCELLED;         /*!< String constant representing the Cancelled state. */

private:
    std::string role_;
    std::string assignee_;
    std::map<std::string, std::string> data_;

protected:
    std::string status_;
    std::string reason_;
    
    /**
     * @brief Helper to enforce valid state transitions.
     * 
     * @param[in] action The name of the action being attempted.
     * @param[in] allowed Array of allowed statuses for the action.
     * @param[in] count The number of elements in the allowed array.
     * @throws std::logic_error if the current status is not in the allowed array.
     */
    void require(const std::string& action, const char* const allowed[], int count) const;

public:
    std::string itemId; /*!< The unique identifier for this work item. */
    std::string state;  /*!< The generic state description of this work item. */

    /**
     * @brief Constructs a WorkItem with only an ID.
     * @param[in] id The unique identifier for the work item.
     */
    WorkItem(std::string id) : WorkNode(id, id), status_(CREATED), itemId(std::move(id)), state("Pending") {}
    
    /**
     * @brief Constructs a WorkItem with an ID, name, and assigned role.
     * @param[in] id The unique identifier for the work item.
     * @param[in] name The display name for the work item.
     * @param[in] role The role required to perform the work.
     */
    WorkItem(const std::string& id, const std::string& name, const std::string& role);
    
    /**
     * @brief Default virtual destructor.
     */
    virtual ~WorkItem() = default;

    /**
     * @brief Gets the role required to perform this work item.
     * @return const std::string& The role string.
     */
    const std::string& role() const;
    
    /**
     * @brief Gets the current lifecycle status of the work item.
     * @return const std::string& The status string.
     */
    const std::string& status() const;
    
    /**
     * @brief Gets the reason string (typically set upon rejection).
     * @return const std::string& The reason string.
     */
    const std::string& reason() const;
    
    /**
     * @brief Gets the participant currently assigned to this work item.
     * @return const std::string& The assignee's name.
     */
    const std::string& assignee() const;
    
    /**
     * @brief Sets the participant assigned to this work item.
     * @param[in] participant The name of the assignee.
     */
    void setAssignee(const std::string& participant);

    /**
     * @brief Sets a custom key/value pair in the work item's data map.
     * @param[in] key The data key.
     * @param[in] value The data value.
     */
    void setData(const std::string& key, const std::string& value);
    
    /**
     * @brief Checks if a specific data key exists in the data map.
     * @param[in] key The data key to search for.
     * @return true if the key exists, false otherwise.
     */
    bool hasData(const std::string& key) const;
    
    /**
     * @brief Retrieves the value for a specific data key.
     * @param[in] key The data key to retrieve.
     * @param[in] fallback The default value to return if the key is not found.
     * @return std::string The data value, or the fallback string.
     */
    std::string getData(const std::string& key, const std::string& fallback = "") const;

    /**
     * @brief Transitions the work item to the Available state.
     */
    virtual void makeAvailable();
    
    /**
     * @brief Assigns the work item and transitions it to the Assigned state.
     * @param[in] participant The name of the participant to assign.
     */
    virtual void assign(const std::string& participant);
    
    /**
     * @brief Starts execution and transitions the work item to InProgress.
     */
    virtual void start();
    
    /**
     * @brief Marks the work item as complete, transitioning to AwaitingApproval.
     */
    virtual void complete();
    
    /**
     * @brief Approves the work item, transitioning it to Completed.
     */
    virtual void approve();
    
    /**
     * @brief Rejects the work item and records the reason.
     * @param[in] reason The explanation for the rejection.
     */
    virtual void reject(const std::string& reason);

    /**
     * @brief Checks if the work item has reached the Completed status.
     * @return true if completed, false otherwise.
     */
    bool isComplete() const override;
    
    /**
     * @brief Returns the completion progress of the work item.
     * @return double The progress ratio (1.0 for completed, 0.0 otherwise).
     */
    double progress() const override;
    
    /**
     * @brief Cancels the work item if it is not already completed or cancelled.
     */
    void cancel() override;
    
    /**
     * @brief Checks if the work item is currently in the Available state.
     * @return true if ready, false otherwise.
     */
    virtual bool isReady() const;
};

#endif // WORKITEM_H