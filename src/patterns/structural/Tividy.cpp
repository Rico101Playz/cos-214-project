#include "patterns/structural/Tividy.h"
#include <sstream>
#include <stdexcept>
#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/creation/FeatureDeliveryProcess.h"
#include "patterns/execution/ReadyOnlyIterator.h"

Tividy::Tividy(const std::string& organisationName)
    : organisation_(organisationName), nextInstance_(1) {}

Organisation& Tividy::organisation() { return organisation_; }

WorkflowInstance& Tividy::findInstance(const std::string& instanceId) const {
    std::map<std::string, std::unique_ptr<WorkflowInstance> >::const_iterator it =
        instances_.find(instanceId);
    if (it == instances_.end()) {
        throw std::invalid_argument("Unknown instance: " + instanceId);
    }
    return *it->second;
}

void Tividy::loadDefinition(const std::string& name) {
    ConcreteDefinitionBuilder builder;
    FeatureDeliveryProcess process;
    process.construct(&builder);
    loadDefinition(name, builder.build());
}

void Tividy::loadDefinition(const std::string& name, const WorkflowDefinition& definition) {
    definitions_[name] = definition;
}

bool Tividy::hasDefinition(const std::string& name) const {
    return definitions_.find(name) != definitions_.end();
}

std::string Tividy::startInstance(const std::string& definition) {
    std::map<std::string, WorkflowDefinition>::const_iterator it = definitions_.find(definition);
    if (it == definitions_.end()) {
        throw std::invalid_argument("Unknown definition: " + definition);
    }

    std::ostringstream id;
    id << "instance-" << nextInstance_++;

    std::unique_ptr<WorkflowInstance> instance(new WorkflowInstance(id.str(), it->second));
    instance->start();
    instances_[id.str()] = std::move(instance);
    return id.str();
}

const WorkflowInstance& Tividy::instance(const std::string& instanceId) const {
    return findInstance(instanceId);
}

std::vector<std::string> Tividy::readyItems(const std::string& instanceId) const {
    std::vector<std::string> ids;
    ReadyOnlyIterator it(&findInstance(instanceId).root());
    for (it.first(); !it.isDone(); it.next()) {
        ids.push_back(it.currentItem()->id());
    }
    return ids;
}

double Tividy::progress(const std::string& instanceId) const {
    return findInstance(instanceId).root().progress();
}
