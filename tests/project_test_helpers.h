#ifndef PROJECT_TEST_HELPERS_H
#define PROJECT_TEST_HELPERS_H

#include <memory>
#include <string>
#include "abstract_factory/PolicyCatalog.h"
#include "core/WorkflowInstance.h"
#include "facade/ProjectFacade.h"

// Files a new (not yet started) instance of the definition in the ProjectFacade under the SmallTeam
// policy, so RequestCommands bound to the facade can run against it.
inline WorkflowInstance& adopt(ProjectFacade& projects, const std::string& id, const WorkflowDefinition& definition) {
    std::unique_ptr<ProjectFacade::Runtime> runtime(new ProjectFacade::Runtime());
    runtime->factory = PolicyCatalog::create("SmallTeam");
    runtime->policy = runtime->factory->name();
    runtime->structured = false;
    std::unique_ptr<WorkflowInstance> instance(new WorkflowInstance(id, definition));
    WorkflowInstance& filed = *instance;
    projects.addInstance(std::move(instance), std::move(runtime));
    return filed;
}

#endif // PROJECT_TEST_HELPERS_H
