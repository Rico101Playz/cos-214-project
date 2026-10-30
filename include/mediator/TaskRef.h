#ifndef TASK_REF_H
#define TASK_REF_H

#include <string>

// Names one task in one project, so the pair of ids travels as one value
// instead of being copied into every call.
struct TaskRef {
    std::string projectId;
    std::string taskId;

    TaskRef() = default;
    TaskRef(const std::string& projectId, const std::string& taskId = "")
        : projectId(projectId), taskId(taskId) {}
};

#endif // TASK_REF_H
