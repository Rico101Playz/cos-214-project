#ifndef VIEWBUILDER_H
#define VIEWBUILDER_H

#include "server/Json.h"

class Tividy;
class SecuredTividy;
class ActivityLog;

class ViewBuilder {
public:
    static Json hello(const Tividy& engine);
    static Json state(Tividy& engine, const SecuredTividy& viewer, const ActivityLog* log = nullptr);
};

#endif
