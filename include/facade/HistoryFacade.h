#ifndef HISTORY_FACADE_H
#define HISTORY_FACADE_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include "command/CommandHistory.h"
#include "command/UndoHistory.h"
#include "observer/EventBus.h"
#include "mediator/Colleague.h"

// FACADE pattern - one of the three facades behind TividyMediator. MEDIATOR pattern - a COLLEAGUE.
//
// Fronts the undo history: the engine-wide UndoHistory, the commands the mediator has run (it is the
// invoker; their CommandHistory lives here) and the history bus the undo steps are announced on.
// Undo and Redo arrive as Requests; the engine-wide snapshots come from the mediator.
// It never refers to the other facades; TividyMediator coordinates them through the public interface below.
class HistoryFacade : public Colleague {
private:
    CommandHistory commands_;
    EventBus historyBus_;
    std::unique_ptr<UndoHistory> ownHistory_;
    UndoHistory* history_;

public:
    HistoryFacade() : ownHistory_(new UndoHistory()), history_(nullptr) {}

    HistoryFacade(const HistoryFacade&) = delete;
    HistoryFacade& operator=(const HistoryFacade&) = delete;

    // Undo or Redo for request.actor(); arg("anyone") "0" limits it to their own steps.
    // Answers with data("label"), the step that was undone or redone.
    Response handle(const Request& request) override;

    CommandHistory& commands();
    EventBus& historyBus();
    // The history the facade owns, used unless another one is plugged in.
    UndoHistory* ownHistory() const;
    UndoHistory* history() const;
    void useHistory(UndoHistory* history);
    bool canUndo(const std::string& actor, bool anyone) const;
    bool canRedo(const std::string& actor, bool anyone) const;
    std::vector<std::string> undoLabels(std::size_t max) const;
    std::vector<std::string> redoLabels(std::size_t max) const;
    void clearHistory();
};

#endif // HISTORY_FACADE_H
