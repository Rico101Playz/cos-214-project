#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <iosfwd>
#include <string>
#include <vector>
#include "patterns/execution/WorkflowObserver.h"

// OBSERVER pattern - a CONCRETE OBSERVER.
//
// Turns the events people care about (assignments, rejections, completion and
// cancellation) into short messages. Messages are kept, and also written to
// the stream when one is given. The other events are ignored.
class Notifier : public WorkflowObserver {
private:
    std::vector<std::string> messages_;
    std::ostream* out_;     // non-owning; nullptr to only keep the messages

public:
    explicit Notifier(std::ostream* out = nullptr);

    void onEvent(const WorkflowEvent& event) override;

    const std::vector<std::string>& messages() const;
};

#endif // NOTIFIER_H