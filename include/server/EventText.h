#ifndef EVENTTEXT_H
#define EVENTTEXT_H

#include <string>

class Tividy;
class WorkflowEvent;

class EventText {
public:
    static std::string describe(const Tividy& engine, const WorkflowEvent& event);
};

#endif
