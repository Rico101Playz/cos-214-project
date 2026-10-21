#include "patterns/structural/WorkItemDecorator.h"

WorkItemDecorator::WorkItemDecorator(WorkItem* item)
    : WorkItem(item ? item->id() : "", item ? item->name() : "", item ? item->role() : ""),
      inner(item) {}

WorkItemDecorator::~WorkItemDecorator() {
    delete inner;
}

void WorkItemDecorator::makeAvailable() {
    if (inner) inner->makeAvailable();
}

void WorkItemDecorator::assign(const std::string& participant) {
    if (inner) inner->assign(participant);
}

void WorkItemDecorator::start() {
    if (inner) inner->start();
}

void WorkItemDecorator::complete() {
    if (inner) inner->complete();
}

void WorkItemDecorator::approve() {
    if (inner) inner->approve();
}

void WorkItemDecorator::reject(const std::string& reason) {
    if (inner) inner->reject(reason);
}

void WorkItemDecorator::escalate() {
    WorkItemDecorator* innerDec = dynamic_cast<WorkItemDecorator*>(inner);
    if (innerDec) {
        innerDec->escalate();
    }
}

bool WorkItemDecorator::isComplete() const {
    return inner ? inner->isComplete() : false;
}

double WorkItemDecorator::progress() const {
    return inner ? inner->progress() : 0.0;
}

void WorkItemDecorator::cancel() {
    if (inner) inner->cancel();
}

bool WorkItemDecorator::isReady() const {
    return inner ? inner->isReady() : false;
}