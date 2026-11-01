#ifndef VIEWBUILDER_H
#define VIEWBUILDER_H

#include "server/Json.h"

class TividyMediator;
class SecuredMediator;
class ActivityLog;

class ViewBuilder {
public:
    static Json hello(const TividyMediator& engine);
    static Json state(TividyMediator& engine, const SecuredMediator& viewer, const ActivityLog* log = nullptr);
};

#endif
