#include "patterns/structural/ValidatedWork.h"
#include <iostream>

ValidatedWork::ValidatedWork(WorkItem* item, const std::vector<std::string>& keys)
    : WorkItemDecorator(item), requiredKeys(keys) {}

void ValidatedWork::complete() {
    if (inner) {
        for (size_t i = 0; i < requiredKeys.size(); ++i) {
            if (!inner->hasData(requiredKeys[i])) { // no required key
                std::cout << "ValidatedWork missing required key: " << requiredKeys[i] << std::endl;
                return;
            }
        }
    }
    WorkItemDecorator::complete();
}