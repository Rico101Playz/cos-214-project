#ifndef OWNER_HANDLER_H
#define OWNER_HANDLER_H

#include "patterns/execution/ApprovalHandler.h"

class Company;

class OwnerHandler : public ApprovalHandler {
private:
    const Company& organisation_;

public:
    explicit OwnerHandler(const Company& organisation);
    void handle(ApprovalRequest& request) override;
    std::string describe() override;
};

#endif
