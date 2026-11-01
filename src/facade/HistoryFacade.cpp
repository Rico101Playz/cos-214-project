#include "facade/HistoryFacade.h"
#include <stdexcept>
#include "mediator/Mediator.h"

Response HistoryFacade::handle(const Request& request) {
    if (request.action() != Action::Undo && request.action() != Action::Redo) {
        throw std::invalid_argument(toString(request.action()) + " is not something the history does");
    }
    if (history_ == nullptr || mediator() == nullptr) {
        throw std::logic_error("There is no history to " + toString(request.action()));
    }
    bool anyone = request.arg("anyone") != "0";
    UndoHistory::Entry entry = request.action() == Action::Undo
        ? history_->undo(*mediator(), request.actor(), anyone)
        : history_->redo(*mediator(), request.actor(), anyone);
    Response response;
    response.put("label", entry.label);
    return response;
}

CommandHistory& HistoryFacade::commands() { return commands_; }

EventBus& HistoryFacade::historyBus() { return historyBus_; }

UndoHistory* HistoryFacade::ownHistory() const { return ownHistory_.get(); }

UndoHistory* HistoryFacade::history() const { return history_; }

void HistoryFacade::useHistory(UndoHistory* history) {
    if (history_ != nullptr) historyBus_.detach(history_);
    history_ = history;
    if (history_ != nullptr) historyBus_.attach(history_);
}

bool HistoryFacade::canUndo(const std::string& actor, bool anyone) const {
    return history_ != nullptr && history_->canUndo(actor, anyone);
}

bool HistoryFacade::canRedo(const std::string& actor, bool anyone) const {
    return history_ != nullptr && history_->canRedo(actor, anyone);
}

std::vector<std::string> HistoryFacade::undoLabels(std::size_t max) const {
    std::vector<std::string> labels;
    if (history_ == nullptr) return labels;
    std::vector<UndoHistory::Entry> entries = history_->undoEntries(max);
    for (std::size_t i = 0; i < entries.size(); ++i) labels.push_back(entries[i].label);
    return labels;
}

std::vector<std::string> HistoryFacade::redoLabels(std::size_t max) const {
    std::vector<std::string> labels;
    if (history_ == nullptr) return labels;
    std::vector<UndoHistory::Entry> entries = history_->redoEntries(max);
    for (std::size_t i = 0; i < entries.size(); ++i) labels.push_back(entries[i].label);
    return labels;
}

void HistoryFacade::clearHistory() {
    if (history_ != nullptr) history_->clear();
}
