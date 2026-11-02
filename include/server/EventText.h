#ifndef EVENTTEXT_H
#define EVENTTEXT_H

#include <string>

class TividyMediator;
class WorkflowEvent;

class EventText {
public:
    static std::string describe(const TividyMediator& engine, const WorkflowEvent& event);
};

#endif
