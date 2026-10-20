#include "patterns/execution/WorkCommand.h"

void WorkCommand::execute() {
    perform();
}

void WorkCommand::undo() {}