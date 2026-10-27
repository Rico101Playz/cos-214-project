#include "core/WorkItem.h"
#include <stdexcept>

const char* const WorkItem::CREATED = "Created";
const char* const WorkItem::AVAILABLE = "Available";
const char* const WorkItem::ASSIGNED = "Assigned";
const char* const WorkItem::IN_PROGRESS = "InProgress";
const char* const WorkItem::AWAITING_APPROVAL = "AwaitingApproval";
const char* const WorkItem::COMPLETED = "Completed";
const char* const WorkItem::REJECTED = "Rejected";
const char* const WorkItem::CANCELLED = "Cancelled";
const char* const WorkItem::ESCALATED = "Escalated";

WorkItem::WorkItem(const std::string& id, const std::string& name, const std::string& role)
    : WorkNode(id, name), role_(role), status_(CREATED) {}

const std::string& WorkItem::role() const { return role_; }

void WorkItem::setData(const std::string& key, const std::string& value) {
    data_[key] = value;
}

bool WorkItem::hasData(const std::string& key) const {
    return data_.find(key) != data_.end();
}

std::string WorkItem::getData(const std::string& key, const std::string& fallback) const {
    std::map<std::string, std::string>::const_iterator it = data_.find(key);
    if (it == data_.end()) {
        return fallback;
    }
    return it->second;
}

void WorkItem::removeData(const std::string& key) { data_.erase(key); }

const std::map<std::string, std::string>& WorkItem::data() const { return data_; }

const std::string& WorkItem::status() const { return status_; }


const std::string& WorkItem::assignee() const { return assignee_; }

void WorkItem::setAssignee(const std::string& participant) { assignee_ = participant; }

const std::string& WorkItem::reason() const { return reason_; }

void WorkItem::require(const std::string& action, const char* const allowed[], int count) const {
    for (int i = 0; i < count; ++i) {
        if (status_ == allowed[i]) {
            return;
        }
    }
    throw std::logic_error("Cannot " + action + " item " + id() + " while it is " + status_);
}

void WorkItem::makeAvailable() {
    const char* const allowed[] = { CREATED };
    require("make available", allowed, 1);
    status_ = AVAILABLE;
}

void WorkItem::assign(const std::string& participant) {
    const char* const allowed[] = { AVAILABLE, ASSIGNED, REJECTED };
    require("assign", allowed, 3);
    assignee_ = participant;
    reason_.clear();
    status_ = ASSIGNED;
}

void WorkItem::start() {
    const char* const allowed[] = { ASSIGNED };
    require("start", allowed, 1);
    status_ = IN_PROGRESS;
}

void WorkItem::complete() {
    const char* const allowed[] = { IN_PROGRESS };
    require("complete", allowed, 1);
    status_ = AWAITING_APPROVAL;
}

void WorkItem::approve() {
    const char* const allowed[] = { AWAITING_APPROVAL };
    require("approve", allowed, 1);
    status_ = COMPLETED;
}

void WorkItem::reject(const std::string& reason) {
    const char* const allowed[] = { AWAITING_APPROVAL };
    require("reject", allowed, 1);
    reason_ = reason;
    status_ = REJECTED;
}

bool WorkItem::isComplete() const { return status_ == COMPLETED; }

bool WorkItem::holdsLoad() const {
    return !assignee_.empty() &&
           (status_ == ASSIGNED || status_ == IN_PROGRESS || status_ == AWAITING_APPROVAL || status_ == ESCALATED);
}

double WorkItem::progress() const { return isComplete() ? 1.0 : 0.0; }

void WorkItem::cancel() {
    if (status_ == COMPLETED || status_ == CANCELLED) {
        return;
    }
    status_ = CANCELLED;
}

bool WorkItem::isReady() const { return status_ == AVAILABLE; }
