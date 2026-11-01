#include "command/Command.h"
#include <stdexcept>
#include "mediator/Colleague.h"
#include "memento/Memento.h"

Command::Command(Colleague* receiver, const TaskRef& scope, SnapshotStore* store)
    : receiver_(receiver), scope_(scope), store_(store) {}

Response Command::execute() {
    if (receiver_ == nullptr) throw std::invalid_argument(describe() + ": no receiver");
    before_ = receiver_->createMemento(scope_, describe());
    Response response = perform();
    if (store_ != nullptr && before_) store_->save(scope_.projectId, before_);
    return response;
}

void Command::undo() {
    if (receiver_ != nullptr && before_) receiver_->restore(scope_, *before_);
}
