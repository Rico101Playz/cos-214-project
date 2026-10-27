#ifndef UNDOHISTORY_H
#define UNDOHISTORY_H

#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include "patterns/execution/EngineMemento.h"
#include "patterns/execution/WorkflowObserver.h"

class Tividy;

class UndoHistory : public WorkflowObserver {
public:
    struct Entry {
        std::string label;
        std::string actor;
        std::shared_ptr<const EngineMemento> state;
    };

private:
    std::deque<Entry> undo_;
    std::deque<Entry> redo_;
    std::size_t limit_;
    bool replaying_;

    static bool mayTake(const std::deque<Entry>& stack, const std::string& actor, bool anyone);

public:
    explicit UndoHistory(std::size_t limit = 200);

    void onEvent(const WorkflowEvent& event) override;

    bool canUndo(const std::string& actor = "", bool anyone = true) const;
    bool canRedo(const std::string& actor = "", bool anyone = true) const;
    Entry undo(Tividy& engine, const std::string& actor = "", bool anyone = true);
    Entry redo(Tividy& engine, const std::string& actor = "", bool anyone = true);
    void clear();

    std::vector<Entry> undoEntries(std::size_t max) const;
    std::vector<Entry> redoEntries(std::size_t max) const;
};

#endif
