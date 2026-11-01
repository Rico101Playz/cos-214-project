#include "patterns/execution/UndoHistory.h"
#include "patterns/structural/Tividy.h"
#include <stdexcept>

UndoHistory::UndoHistory(std::size_t limit) : limit_(limit), replaying_(false) {}

void UndoHistory::onEvent(const WorkflowEvent& event) {
    if (event.type != WorkflowEvent::STEP_COMMITTED || !event.before || replaying_) return;
    Entry entry;
    entry.label = event.detail;
    entry.actor = event.actor;
    entry.state = event.before;
    undo_.push_back(entry);
    while (undo_.size() > limit_) undo_.pop_front();
    redo_.clear();
}

bool UndoHistory::mayTake(const std::deque<Entry>& stack, const std::string& actor, bool anyone) {
    return !stack.empty() && (anyone || (!actor.empty() && stack.back().actor == actor));
}

bool UndoHistory::canUndo(const std::string& actor, bool anyone) const { return mayTake(undo_, actor, anyone); }

bool UndoHistory::canRedo(const std::string& actor, bool anyone) const { return mayTake(redo_, actor, anyone); }

UndoHistory::Entry UndoHistory::undo(Tividy& engine, const std::string& actor, bool anyone) {
    if (!mayTake(undo_, actor, anyone)) {
        throw std::logic_error(undo_.empty() ? "Nothing to undo"
                                             : "The last action was " + undo_.back().actor + "'s, so only a manager can undo it");
    }
    Entry entry = undo_.back();
    Entry back = entry;
    back.state = engine.createMemento(entry.label);
    replaying_ = true;
    try {
        engine.restoreMemento(*entry.state);
    } catch (...) {
        replaying_ = false;
        throw;
    }
    replaying_ = false;
    undo_.pop_back();
    redo_.push_back(back);
    return entry;
}

UndoHistory::Entry UndoHistory::redo(Tividy& engine, const std::string& actor, bool anyone) {
    if (!mayTake(redo_, actor, anyone)) {
        throw std::logic_error(redo_.empty() ? "Nothing to redo"
                                             : "The next redo is " + redo_.back().actor + "'s, so only a manager can redo it");
    }
    Entry entry = redo_.back();
    Entry back = entry;
    back.state = engine.createMemento(entry.label);
    replaying_ = true;
    try {
        engine.restoreMemento(*entry.state);
    } catch (...) {
        replaying_ = false;
        throw;
    }
    replaying_ = false;
    redo_.pop_back();
    undo_.push_back(back);
    return entry;
}

void UndoHistory::clear() {
    undo_.clear();
    redo_.clear();
}

std::vector<UndoHistory::Entry> UndoHistory::undoEntries(std::size_t max) const {
    std::vector<Entry> out;
    for (std::size_t i = undo_.size(); i > 0 && out.size() < max; --i) out.push_back(undo_[i - 1]);
    return out;
}

std::vector<UndoHistory::Entry> UndoHistory::redoEntries(std::size_t max) const {
    std::vector<Entry> out;
    for (std::size_t i = redo_.size(); i > 0 && out.size() < max; --i) out.push_back(redo_[i - 1]);
    return out;
}
