#include "server/ActivityLog.h"
#include "server/EventText.h"
#include <ctime>

ActivityLog::ActivityLog(const Tividy& engine, std::size_t limit)
    : engine_(engine), limit_(limit), version_(0) {}

std::string ActivityLog::timestamp() {
    std::time_t now = std::time(nullptr);
    std::tm local;
#ifdef _WIN32
    bool ok = localtime_s(&local, &now) == 0;
#else
    bool ok = localtime_r(&now, &local) != nullptr;
#endif
    char buf[32];
    if (!ok || std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &local) == 0) {
        return "";
    }
    return buf;
}

void ActivityLog::onEvent(const WorkflowEvent& event) {
    add(EventText::describe(engine_, event));
}

void ActivityLog::add(const std::string& text) {
    entries_.push_back(timestamp() + "  " + text);
    if (entries_.size() > limit_) {
        entries_.erase(entries_.begin(), entries_.begin() + (entries_.size() - limit_));
    }
    ++version_;
}

void ActivityLog::replace(const std::vector<std::string>& entries) {
    entries_ = entries;
    ++version_;
}

const std::vector<std::string>& ActivityLog::entries() const { return entries_; }

std::vector<std::string> ActivityLog::latest(std::size_t count) const {
    std::vector<std::string> out;
    for (std::size_t i = entries_.size(); i > 0 && out.size() < count; --i) {
        out.push_back(entries_[i - 1]);
    }
    return out;
}

unsigned long ActivityLog::version() const { return version_; }
