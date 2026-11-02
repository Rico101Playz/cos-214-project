#include "decorator/ValidatedWork.h"
#include <stdexcept>
#include "composite/LeafTask.h"
#include "decorator/ApprovalWork.h"
#include "decorator/BlockedWork.h"
#include "mediator/Request.h"

ValidatedWork::ValidatedWork(std::unique_ptr<Task> inner) : TaskDecorator(std::move(inner)) {}

const std::vector<std::string>& ValidatedWork::requiredKeys() const { return requiredKeys_; }

void ValidatedWork::require(const std::string& key) {
    if (key.empty()) throw std::invalid_argument("A required field needs a name");
    for (std::size_t i = 0; i < requiredKeys_.size(); ++i) {
        if (requiredKeys_[i] == key) return;
    }
    requiredKeys_.push_back(key);
}

std::vector<std::string> ValidatedWork::missingKeys() const {
    std::vector<std::string> missing;
    const LeafTask* work = leafOf(&inner());
    for (std::size_t i = 0; i < requiredKeys_.size(); ++i) {
        if (work == nullptr || !work->hasData(requiredKeys_[i])) missing.push_back(requiredKeys_[i]);
    }
    return missing;
}

ValidatedWork* ValidatedWork::validatedWork() { return this; }

void ValidatedWork::handle(const Request& request) {
    if (request.action() == Action::Complete) {
        std::vector<std::string> missing = missingKeys();
        if (!missing.empty()) {
            std::string list;
            for (std::size_t i = 0; i < missing.size(); ++i) list += (i ? ", " : "") + missing[i];
            throw std::logic_error(id() + " still needs " + list);
        }
    }
    inner_->handle(request);
}

std::unique_ptr<Task> decorateProjectTask(std::unique_ptr<Task> task) {
    std::unique_ptr<Task> approved(new ApprovalWork(std::move(task)));
    std::unique_ptr<Task> validated(new ValidatedWork(std::move(approved)));
    return std::unique_ptr<Task>(new BlockedWork(std::move(validated)));
}
