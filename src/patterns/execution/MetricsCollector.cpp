#include "patterns/execution/MetricsCollector.h"
#include <sstream>

MetricsCollector::MetricsCollector() : total_(0) {}

void MetricsCollector::onEvent(const WorkflowEvent& event) {
    ++counts_[event.type];
    ++total_;
}

int MetricsCollector::count(const std::string& type) const {
    std::map<std::string, int>::const_iterator it = counts_.find(type);
    return it == counts_.end() ? 0 : it->second;
}

int MetricsCollector::total() const { return total_; }

std::string MetricsCollector::summary() const {
    std::ostringstream text;
    text << total_ << " events";
    if (!counts_.empty()) {
        text << " (";
        for (std::map<std::string, int>::const_iterator it = counts_.begin();
             it != counts_.end(); ++it) {
            if (it != counts_.begin()) {
                text << ", ";
            }
            text << it->first << "=" << it->second;
        }
        text << ")";
    }
    return text.str();
}