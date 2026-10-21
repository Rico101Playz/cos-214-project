#ifndef WORK_ITEM_DECORATOR_H
#define WORK_ITEM_DECORATOR_H

#include "core/WorkItem.h"
#include <string>

class WorkItemDecorator : public WorkItem {
protected:
    WorkItem* inner;

public:
    WorkItemDecorator(WorkItem* item);
    virtual ~WorkItemDecorator();

    void makeAvailable() override;
    void assign(const std::string& participant) override;
    void start() override;
    void complete() override;
    void approve() override;
    void reject(const std::string& reason) override;
    virtual void escalate(); // not in workitem.h yet

    bool isComplete() const override;
    double progress() const override;
    void cancel() override;
    bool isReady() const override;
};

#endif