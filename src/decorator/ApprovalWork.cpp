#include "decorator/ApprovalWork.h"
#include <cstdlib>
#include <stdexcept>
#include "composite/LeafTask.h"
#include "mediator/Request.h"

ApprovalWork::ApprovalWork(std::unique_ptr<Task> inner) : TaskDecorator(std::move(inner)) {}

std::string ApprovalWork::approverRole() const {
    return approverRoles_.empty() ? std::string() : approverRoles_.front();
}

void ApprovalWork::setApproverRole(const std::string& role) {
    approverRoles_.clear();
    if (!role.empty()) approverRoles_.push_back(role);
}

const std::vector<std::string>& ApprovalWork::approverRoles() const { return approverRoles_; }

void ApprovalWork::setApproverRoles(const std::vector<std::string>& roles) {
    approverRoles_.clear();
    for (std::size_t i = 0; i < roles.size(); ++i) {
        if (!roles[i].empty()) approverRoles_.push_back(roles[i]);
    }
}

std::string ApprovalWork::pendingApprover() const {
    return pendingApprovers_.empty() ? std::string() : pendingApprovers_.front();
}

void ApprovalWork::setPendingApprover(const std::string& role) {
    pendingApprovers_.clear();
    if (!role.empty()) pendingApprovers_.push_back(role);
}

const std::vector<std::string>& ApprovalWork::pendingApprovers() const { return pendingApprovers_; }

void ApprovalWork::setPendingApprovers(const std::vector<std::string>& roles) { pendingApprovers_ = roles; }

bool ApprovalWork::takePendingSlot(const std::string& role) {
    for (std::size_t i = 0; i < pendingApprovers_.size(); ++i) {
        if (pendingApprovers_[i] == role) {
            pendingApprovers_.erase(pendingApprovers_.begin() + i);
            return true;
        }
    }
    return false;
}

const std::vector<ApprovalWork::Signature>& ApprovalWork::signatures() const { return signatures_; }

void ApprovalWork::addSignature(const std::string& role, const std::string& person) {
    signatures_.push_back(Signature(role, person));
}

bool ApprovalWork::hasSigned(const std::string& person) const {
    for (std::size_t i = 0; i < signatures_.size(); ++i) {
        if (signatures_[i].second == person) return true;
    }
    return false;
}

void ApprovalWork::clearApprovals() {
    pendingApprovers_.clear();
    signatures_.clear();
}

ApprovalWork* ApprovalWork::approvalWork() { return this; }

void ApprovalWork::handle(const Request& request) {
    switch (request.action()) {
        case Action::RouteApproval: {
            clearApprovals();
            int automatic = std::atoi(request.arg("automatic").c_str());
            for (int i = 0; i < automatic; ++i) addSignature("Auto", "System");
            setPendingApprovers(request.list("pending"));
            return;
        }
        case Action::Sign:
            if (leafOf(this)->status() != LeafTask::AWAITING_APPROVAL) {
                throw std::logic_error("item '" + id() + "' is not waiting for approval");
            }
            if (!takePendingSlot(request.arg("slot"))) {
                throw std::logic_error("item '" + id() + "' is not waiting for a " + request.arg("slot"));
            }
            addSignature(request.arg("slot"), request.arg("person"));
            return;
        case Action::Approve:
            TaskDecorator::handle(request);
            setPendingApprovers(std::vector<std::string>());
            return;
        case Action::Reject:
        case Action::Withdraw:
            TaskDecorator::handle(request);
            clearApprovals();
            return;
        default:
            TaskDecorator::handle(request);
    }
}
