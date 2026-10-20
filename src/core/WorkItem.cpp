#include "core/WorkItem.h"

WorkItem::WorkItem(const std::string& id, const std::string& name, const std::string& role)
    : id_(id), name_(name), role_(role) {}

const std::string& WorkItem::id() const { return id_; }

const std::string& WorkItem::name() const { return name_; }

const std::string& WorkItem::role() const { return role_; }

void WorkItem::setData(const std::string& key, const std::string& value) {
    data_[key] = value;
}

bool WorkItem::hasData(const std::string& key) const {
    return data_.find(key) != data_.end();
}

std::string WorkItem::getData(const std::string& key, const std::string& fallback) const {
    std::map<std::string, std::string>::const_iterator it = data_.find(key);
    if (it == data_.end()) {
        return fallback;
    }
    return it->second;
}
