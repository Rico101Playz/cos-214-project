#ifndef STATE_TEST_HELPERS_H
#define STATE_TEST_HELPERS_H

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/WorkflowDefinition.h"
#include "iterator/WorkIterator.h"
#include "composite/CompositeTask.h"
#include "composite/LeafTask.h"

// ---- exception helpers (C++11 lambdas) ----
template <typename F>
inline bool throwsLogicError(F action) {
    try { action(); } catch (const std::logic_error&) { return true; } catch (...) { return false; }
    return false;
}

template <typename F>
inline bool throwsLogicErrorContaining(F action, const std::string& text) {
    try {
        action();
    } catch (const std::logic_error& e) {
        return std::string(e.what()).find(text) != std::string::npos;
    } catch (...) {
        return false;
    }
    return false;
}

// std::invalid_argument derives from std::logic_error, so it gets its own helper.
template <typename F>
inline bool throwsInvalidArgument(F action) {
    try { action(); } catch (const std::invalid_argument&) { return true; } catch (...) { return false; }
    return false;
}

inline bool nearly(double a, double b) { return std::fabs(a - b) < 1e-9; }

// ---- tree-building helpers ----
inline LeafTask* addLeaf(CompositeTask& stage, const std::string& id,
                             const std::string& role = "Developer") {
    LeafTask* leaf = new LeafTask(id, id, role);
    stage.add(std::unique_ptr<Task>(leaf));
    return leaf;
}

inline CompositeTask* addSubStage(CompositeTask& parent, const std::string& id) {
    CompositeTask* sub = new CompositeTask(id, id);
    parent.add(std::unique_ptr<Task>(sub));
    return sub;
}

// Drives an item (Created or Available) all the way to Completed.
inline void finishItem(LeafTask& item) {
    if (item.status() == LeafTask::CREATED) item.makeAvailable();
    item.assign("tester");
    item.start();
    item.complete();
    item.approve();
}

// Walks an iterator and returns the visited ids as "a,b,c".
inline std::string walkIds(WorkIterator& it) {
    std::string result;
    for (it.first(); !it.isDone(); it.next()) {
        if (!result.empty()) result += ",";
        result += it.currentItem()->id();
    }
    return result;
}

// ---- definition helper (adapt here if WorkflowDefinition is built differently) ----
inline WorkflowDefinition makeDefinition(const std::string& name,
                                         const std::vector<std::string>& stages,
                                         const std::vector<std::string>& items) {
    WorkflowDefinition definition;
    definition.name = name;
    definition.stages = stages;
    definition.items = items;
    return definition;
}

#endif // STATE_TEST_HELPERS_H