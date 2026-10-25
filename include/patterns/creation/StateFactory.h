#ifndef STATEFACTORY_H
#define STATEFACTORY_H

#include <string>
#include <memory>

class WorkItemState;
class InstanceState;

std::unique_ptr<WorkItemState> createWorkItemState(const std::string& name);
std::unique_ptr<InstanceState> createInstanceState(const std::string& name);

#endif // STATEFACTORY_H