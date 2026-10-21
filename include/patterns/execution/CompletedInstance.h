#ifndef COMPLETEDINSTANCE_H
#define COMPLETEDINSTANCE_H

#include "InstanceState.h"

class CompletedInstance : public InstanceState {
public:
    std::string name() const override;
};

#endif