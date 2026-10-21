#ifndef CANCELLEDINSTANCE_H
#define CANCELLEDINSTANCE_H

#include "InstanceState.h"

class CancelledInstance : public InstanceState {
public:
    std::string name() const override;
};

#endif