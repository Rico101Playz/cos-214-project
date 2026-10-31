#ifndef STATEFACTORY_H
#define STATEFACTORY_H

#include <string>
#include <memory>

class WorkItemState;
class InstanceState;

/**
 * @brief Creates a specific WorkItemState based on the provided name.
 * 
 * @param[in] name The string identifier for the desired work item state 
 *                 (e.g., "Created", "Available", "Assigned").
 * @return std::unique_ptr<WorkItemState> A unique pointer to the newly instantiated state.
 * @throws std::invalid_argument if the provided state name is unknown.
 */
std::unique_ptr<WorkItemState> createWorkItemState(const std::string& name);

/**
 * @brief Creates a specific InstanceState based on the provided name.
 * 
 * @param[in] name The string identifier for the desired instance state 
 *                 (e.g., "Created", "Running", "Completed").
 * @return std::unique_ptr<InstanceState> A unique pointer to the newly instantiated state.
 * @throws std::invalid_argument if the provided instance state name is unknown.
 */
std::unique_ptr<InstanceState> createInstanceState(const std::string& name);

#endif // STATEFACTORY_H