#ifndef APPROVAL_HANDLER_H
#define APPROVAL_HANDLER_H

#include <string>
#include "patterns/execution/ApprovalRequest.h"

/**
 * @class ApprovalHandler
 * @brief Abstract base class representing a node in the approval Chain of Responsibility.
 * 
 * Defines the handler interface for processing and forwarding an ApprovalRequest.
 * It holds a successor pointer to the next handler in line, enabling dynamic 
 * composition and traversal of approval pipelines at runtime.
 */
class ApprovalHandler {
private:
    ApprovalHandler* next_; // pointer to the next handler in the chain

public:
    /**
     * @brief Constructs an ApprovalHandler with an empty successor link.
     */
    ApprovalHandler();

    /**
     * @brief Virtual destructor ensuring clean polymorphic teardown of the entire chain.
     */
    virtual ~ApprovalHandler();

    /**
     * @brief Sets the successor handler in the chain of responsibility.
     * @param[in] next Pointer to the next ApprovalHandler to be linked.
     */
    void setNext(ApprovalHandler* next);

    // passes the request to the next handler if one exists
    /**
     * @brief Handles the incoming approval request or delegates it to the successor.
     * @param[in,out] request The approval request being processed through the chain.
     */
    virtual void handle(ApprovalRequest& request);

    /**
     * @brief Returns a textual description or name of the concrete handler.
     * @return std::string The name or role of this handler.
     */
    virtual std::string describe() = 0;
};

#endif