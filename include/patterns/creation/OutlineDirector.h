#ifndef OUTLINEDIRECTOR_H
#define OUTLINEDIRECTOR_H

#include <string>
#include "patterns/creation/WorkflowBuilder.h"

class OutlineDirector {
private:
    std::string outline;
    WorkflowBuilder* builder;

public:
    OutlineDirector(std::string outline, WorkflowBuilder* builder);
    
    void construct();
};

#endif // OUTLINEDIRECTOR_H