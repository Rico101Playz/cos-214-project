#ifndef BLOCKED_WORK_H
#define BLOCKED_WORK_H

#include <memory>
#include <string>
#include <vector>
#include "decorator/TaskDecorator.h"

// DECORATOR pattern - a CONCRETE DECORATOR.
//
// Adds dependencies to a task: the ids of the tasks that must be complete
// before it can open. While one of them is still open the task is blocked:
// it is not released when its stage opens, and Assign, Start and Complete
// are refused here before they reach the task. Every task in a project
// wears one, so its blockers never live on the task itself.
class BlockedWork : public TaskDecorator {
private:
    std::vector<std::string> blockedBy_;

public:
    explicit BlockedWork(std::unique_ptr<Task> inner);

    const std::vector<std::string>& blockedBy() const;
    // Throws std::invalid_argument when the task would block itself.
    void addBlocker(const std::string& taskId);
    // The blockers that are not complete yet.
    std::vector<std::string> openBlockers() const;
    bool isBlocked() const;

    BlockedWork* blockedWork() override;
    void handle(const Request& request) override;
};

#endif // BLOCKED_WORK_H
