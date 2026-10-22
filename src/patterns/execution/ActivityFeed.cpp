#include "patterns/execution/ActivityFeed.h"

void ActivityFeed::onEvent(const WorkflowEvent& event) {
    entries_.push_back(event.describe());
}

const std::vector<std::string>& ActivityFeed::entries() const { return entries_; }

std::size_t ActivityFeed::size() const { return entries_.size(); }