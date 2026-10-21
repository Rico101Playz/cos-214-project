#ifndef APPROVAL_HANDLER_H
#define APPROVAL_HANDLER_H

#include <string>
#include "patterns/execution/ApprovalRequest.h"

class ApprovalHandler {
private:
    ApprovalHandler* next_; // pointer to the next handler in the chain
public:
    ApprovalHandler();
    virtual ~ApprovalHandler();

    void setNext(ApprovalHandler* next);
    // passes the request to the next handler if one exists
    virtual void handle(ApprovalRequest& request);
    virtual std::string describe() = 0;
};

#endif