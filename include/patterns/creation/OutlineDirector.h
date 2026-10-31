#ifndef OUTLINEDIRECTOR_H
#define OUTLINEDIRECTOR_H

#include <string>
#include "patterns/creation/WorkflowBuilder.h"

/**
 * @class OutlineDirector
 * @brief Directs the construction of a basic workflow outline.
 * 
 * Uses a provided WorkflowBuilder to assemble a predefined workflow 
 * consisting of standard stages (Initialization, Processing, Completion), 
 * basic work items, and their sequential dependencies.
 */
class OutlineDirector {
private:
    std::string outline;
    WorkflowBuilder* builder;

public:
    /**
     * @brief Constructs an OutlineDirector with a specific outline name and builder.
     * 
     * @param[in] outline The name or description of the workflow outline.
     * @param[in] builder A pointer to the WorkflowBuilder used to construct the workflow.
     */
    OutlineDirector(std::string outline, WorkflowBuilder* builder);
    
    /**
     * @brief Executes the step-by-step workflow construction process.
     * 
     * Resets the builder, then sequentially adds the predefined stages, 
     * work items, and dependencies to formulate the complete outline.
     */
    void construct();
};

#endif // OUTLINEDIRECTOR_H