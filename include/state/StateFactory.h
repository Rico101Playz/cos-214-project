#ifndef STATEFACTORY_H
#define STATEFACTORY_H

#include <string>
#include <memory>

class TaskState;

/**
 * @brief Creates a specific TaskState based on the provided name.
 *
 * @param[in] name The string identifier for the desired work item state
 *                 (e.g., "Created", "Available", "Assigned").
 * @return std::unique_ptr<TaskState> A unique pointer to the newly instantiated state.
 * @throws std::invalid_argument if the provided state name is unknown.
 */
std::unique_ptr<TaskState> createWorkItemState(const std::string& name);

#endif // STATEFACTORY_H