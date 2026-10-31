#ifndef COMMANDINVOKER_H
#define COMMANDINVOKER_H

#include <vector>
#include <memory>
#include "patterns/execution/WorkCommand.h"

class WorkflowInstance;

/**
 * @class CommandInvoker
 * @brief Invoker class in the Command pattern responsible for executing and rolling back commands.
 * 
 * Manages the dispatching of commands to a target WorkflowInstance and maintains
 * an internal execution history stack to enable reverse-order (undo) rollbacks.
 */
class CommandInvoker {
private:
    std::vector<std::unique_ptr<WorkCommand>> history; /*!< Stack tracking executed commands for undo operations. */

public:
    /**
     * @brief Executes a command on the target workflow instance and logs it in the history stack.
     * 
     * @param[in] command Unique pointer to the WorkCommand to execute.
     * @param[in,out] instance Pointer to the target WorkflowInstance on which the command is executed.
     */
    void invoke(std::unique_ptr<WorkCommand> command, WorkflowInstance* instance);

    /**
     * @brief Reverses the most recently executed command on the workflow instance.
     * 
     * Pops the last command from the history stack and invokes its undo method.
     * 
     * @param[in,out] instance Pointer to the target WorkflowInstance to roll back.
     */
    void undoLast(WorkflowInstance* instance);
};

#endif // COMMANDINVOKER_H