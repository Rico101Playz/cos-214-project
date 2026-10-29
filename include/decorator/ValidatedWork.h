#ifndef VALIDATED_WORK_H
#define VALIDATED_WORK_H

#include <memory>
#include <string>
#include <vector>
#include "decorator/TaskDecorator.h"

// DECORATOR pattern - a CONCRETE DECORATOR.
//
// Adds a completion check to a task: the data keys that must be filled in
// before the task may be marked complete. Complete is refused here while one
// of them is missing; every other Request passes straight through. A task
// requires nothing until a key is added, so wearing one changes nothing by
// default.
class ValidatedWork : public TaskDecorator {
private:
    std::vector<std::string> requiredKeys_;

public:
    explicit ValidatedWork(std::unique_ptr<Task> inner);

    const std::vector<std::string>& requiredKeys() const;
    void require(const std::string& key);
    // The required keys the task has no value for yet.
    std::vector<std::string> missingKeys() const;

    ValidatedWork* validatedWork() override;
    void handle(const Request& request) override;
};

// Wraps a bare task in every decorator a project task wears:
// BlockedWork(ValidatedWork(ApprovalWork(task))).
std::unique_ptr<Task> decorateProjectTask(std::unique_ptr<Task> task);

#endif // VALIDATED_WORK_H
