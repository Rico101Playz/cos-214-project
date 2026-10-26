#ifndef COMPOSITESTAGE_H
#define COMPOSITESTAGE_H

#include <memory>
#include <string>
#include <vector>
#include "patterns/structural/WorkNode.h"
#include "patterns/execution/StageMode.h"

// COMPOSITE pattern - the COMPOSITE.
//
// A stage (or sub-process) that owns an ordered list of child nodes. Its
// completion and progress are worked out from its children, so a whole tree
// can be asked "are you done?" through the root alone.
class CompositeStage : public WorkNode {
private:
    std::vector<std::unique_ptr<WorkNode> > children_;

public:
    CompositeStage(const std::string& id, const std::string& name);
    std::unique_ptr<StageMode> mode_;
    StageMode& mode();

    // Takes ownership and returns the added node.
    // Throws std::invalid_argument for a null child.
    WorkNode& add(std::unique_ptr<WorkNode> child);

    // Removes a direct child by id; returns false when there is no such child.
    bool remove(const std::string& id);

    std::size_t childCount() const override;
    WorkNode* child(std::size_t index) const override;

    // A stage is complete when every child is (an empty stage is complete).
    bool isComplete() const override;
    // The average progress of the children (1.0 for an empty stage).
    double progress() const override;
    // Cancels every child.
    void cancel() override;

    void setMode(std::unique_ptr<StageMode> mode);

    void activate();
    
    void childCompleted(WorkNode& child);
};

#endif // COMPOSITESTAGE_H
