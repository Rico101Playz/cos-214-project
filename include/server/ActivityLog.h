#ifndef ACTIVITYLOG_H
#define ACTIVITYLOG_H

#include <cstddef>
#include <string>
#include <vector>
#include "patterns/execution/WorkflowObserver.h"

class Tividy;

class ActivityLog : public WorkflowObserver {
private:
    const Tividy& engine_;
    std::vector<std::string> entries_;
    std::size_t limit_;
    unsigned long version_;

public:
    ActivityLog(const Tividy& engine, std::size_t limit = 1000);

    void onEvent(const WorkflowEvent& event) override;

    void add(const std::string& text);
    void replace(const std::vector<std::string>& entries);
    const std::vector<std::string>& entries() const;
    std::vector<std::string> latest(std::size_t count) const;
    unsigned long version() const;

    static std::string timestamp();
};

#endif
