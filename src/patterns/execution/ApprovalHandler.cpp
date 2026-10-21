#include "patterns/execution/ApprovalHandler.h"

ApprovalHandler::ApprovalHandler() : next_(nullptr) {}

ApprovalHandler::~ApprovalHandler() {
    delete next_; // composition
}

void ApprovalHandler::setNext(ApprovalHandler* next) {
    next_ = next;
}

void ApprovalHandler::handle(ApprovalRequest& request) {
    if (next_ != nullptr) { // default implementation
        next_->handle(request);
    }
}