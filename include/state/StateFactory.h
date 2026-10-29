#ifndef STATEFACTORY_H
#define STATEFACTORY_H

#include <string>
#include <memory>

class TaskState;

std::unique_ptr<TaskState> createWorkItemState(const std::string& name);

#endif // STATEFACTORY_H