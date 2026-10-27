#ifndef POLICYCATALOG_H
#define POLICYCATALOG_H

#include <memory>
#include <string>
#include <vector>
#include "patterns/creation/WorkflowFactory.h"

class PolicyCatalog {
public:
    static std::unique_ptr<WorkflowFactory> create(const std::string& policy);
    static std::vector<std::string> names();
    static bool exists(const std::string& policy);
};

#endif
