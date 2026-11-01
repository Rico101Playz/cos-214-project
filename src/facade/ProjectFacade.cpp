#include "facade/ProjectFacade.h"
#include <set>
#include <sstream>
#include <stdexcept>
#include "builder/ConcreteDefinitionBuilder.h"
#include "builder/OutlineDirector.h"
#include "iterator/DepthFirstIterator.h"
#include "iterator/FilterIterator.h"
#include "composite/LeafTask.h"
#include "decorator/ApprovalWork.h"
#include "decorator/BlockedWork.h"
#include "memento/Memento.h"

WorkflowInstance& ProjectFacade::findInstance(const std::string& instanceId) const {
    std::map<std::string, std::unique_ptr<WorkflowInstance> >::const_iterator it =
        instances_.find(instanceId);
    if (it == instances_.end()) {
        throw std::invalid_argument("Unknown instance: " + instanceId);
    }
    return *it->second;
}

LeafTask& ProjectFacade::findItem(const std::string& instanceId, const std::string& itemId) const {
    LeafTask* item = findInstance(instanceId).item(itemId);
    if (item == nullptr) {
        throw std::invalid_argument("Unknown item " + itemId + " in instance " + instanceId);
    }
    return *item;
}

LeafTask& ProjectFacade::findLeaf(const std::string& instanceId, const std::string& itemId) const {
    LeafTask* leaf = leafOf(&findItem(instanceId, itemId));
    if (leaf == nullptr) {
        throw std::invalid_argument("Item " + itemId + " is not a work item leaf");
    }
    return *leaf;
}

ProjectFacade::Runtime& ProjectFacade::runtime(const std::string& instanceId) const {
    std::map<std::string, std::unique_ptr<Runtime> >::const_iterator it = runtimes_.find(instanceId);
    if (it == runtimes_.end()) {
        throw std::invalid_argument("Unknown instance: " + instanceId);
    }
    return *it->second;
}

void ProjectFacade::loadDefinition(const std::string& name) {
    ConcreteDefinitionBuilder builder;
    OutlineDirector process("Feature Delivery Outline", &builder);
    process.construct();
    loadDefinition(name, builder.build());
}

void ProjectFacade::loadDefinition(const std::string& name, const WorkflowDefinition& definition) {
    definitions_[name] = definition;
}

bool ProjectFacade::hasDefinition(const std::string& name) const {
    return definitions_.find(name) != definitions_.end();
}

std::vector<std::string> ProjectFacade::definitionNames() const {
    std::vector<std::string> names;
    for (std::map<std::string, WorkflowDefinition>::const_iterator it = definitions_.begin();
         it != definitions_.end(); ++it) {
        names.push_back(it->first);
    }
    return names;
}

const WorkflowDefinition& ProjectFacade::definition(const std::string& name) const {
    std::map<std::string, WorkflowDefinition>::const_iterator it = definitions_.find(name);
    if (it == definitions_.end()) {
        throw std::invalid_argument("Unknown definition: " + name);
    }
    return it->second;
}

bool ProjectFacade::dependsOn(const LeafTask& from, const LeafTask& target) const {
    std::set<const LeafTask*> waitingOn;
    for (const LeafTask* up = &target; up != nullptr; up = up->parentItem()) waitingOn.insert(up);
    const Task* top = &from;
    while (top->parent() != nullptr) top = top->parent();
    std::vector<const LeafTask*> todo(1, &from);
    std::set<const LeafTask*> seen;
    while (!todo.empty()) {
        const LeafTask* item = todo.back();
        todo.pop_back();
        if (!seen.insert(item).second) continue;
        if (waitingOn.count(item) != 0) return true;
        const Task& node = item->outermost();
        for (std::size_t i = 0; i < node.childCount(); ++i) {
            const LeafTask* sub = leafOf(node.child(i));
            if (sub != nullptr) todo.push_back(sub);
        }
        std::vector<std::string> blockers = item->openBlockers();
        for (std::size_t i = 0; i < blockers.size(); ++i) {
            const LeafTask* blocker =
                leafOf(const_cast<Task*>(top)->find(blockers[i]));
            if (blocker != nullptr) todo.push_back(blocker);
        }
    }
    return false;
}

std::string ProjectFacade::nextTaskId(const WorkflowInstance& instance) {
    std::string id;
    do {
        std::ostringstream out;
        out << "T-" << nextTask_++;
        id = out.str();
    } while (instance.find(id) != nullptr);
    return id;
}

const WorkflowInstance& ProjectFacade::instance(const std::string& instanceId) const {
    return findInstance(instanceId);
}

const std::vector<std::string>& ProjectFacade::instanceIds() const {
    return instanceOrder_;
}

std::string ProjectFacade::policyOf(const std::string& instanceId) const {
    return runtime(instanceId).factory->name();
}

std::vector<ItemRef> ProjectFacade::workFor(const std::string& participant) const {
    std::vector<ItemRef> refs;
    for (std::size_t i = 0; i < instanceOrder_.size(); ++i) {
        FilterIterator it(&findInstance(instanceOrder_[i]).root(), TaskFilter::assignedTo(participant));
        for (it.first(); !it.isDone(); it.next()) {
            refs.push_back(ItemRef(instanceOrder_[i], it.currentItem()->id()));
        }
    }
    return refs;
}

std::vector<std::string> ProjectFacade::readyItems(const std::string& instanceId) const {
    std::vector<std::string> ids;
    FilterIterator it(&findInstance(instanceId).root());
    for (it.first(); !it.isDone(); it.next()) {
        ids.push_back(it.currentItem()->id());
    }
    return ids;
}

double ProjectFacade::progress(const std::string& instanceId) const {
    // Counted per work item, so an empty stage does not inflate the figure.
    int total = 0;
    int done = 0;
    DepthFirstIterator it(&findInstance(instanceId).root());
    for (it.first(); !it.isDone(); it.next()) {
        LeafTask* item = leafOf(it.currentItem());
        if (item != nullptr) {
            ++total;
            if (item->isComplete()) {
                ++done;
            }
        }
    }
    return total == 0 ? 1.0 : static_cast<double>(done) / total;
}

const std::map<std::string, WorkflowDefinition>& ProjectFacade::definitions() const { return definitions_; }

void ProjectFacade::setDefinitions(const std::map<std::string, WorkflowDefinition>& definitions) {
    definitions_ = definitions;
}

std::string ProjectFacade::nextInstanceId() const { return "instance-" + std::to_string(nextInstance_); }

int ProjectFacade::nextInstanceNumber() const { return nextInstance_; }

int ProjectFacade::nextTaskNumber() const { return nextTask_; }

void ProjectFacade::addInstance(std::unique_ptr<WorkflowInstance> instance, std::unique_ptr<Runtime> runtime) {
    const std::string id = instance->id();
    ++nextInstance_;
    instances_[id] = std::move(instance);
    runtimes_[id] = std::move(runtime);
    instanceOrder_.push_back(id);
}

std::unique_ptr<WorkflowInstance> ProjectFacade::takeInstance(const std::string& instanceId) {
    std::map<std::string, std::unique_ptr<WorkflowInstance> >::iterator it = instances_.find(instanceId);
    if (it == instances_.end()) return std::unique_ptr<WorkflowInstance>();
    return std::move(it->second);
}

void ProjectFacade::replaceInstances(std::map<std::string, std::unique_ptr<WorkflowInstance> >& instances,
                                     std::map<std::string, std::unique_ptr<Runtime> >& runtimes,
                                     const std::vector<std::string>& order, int nextInstance, int nextTask) {
    instances_.swap(instances);
    runtimes_.swap(runtimes);
    instanceOrder_ = order;
    nextInstance_ = nextInstance;
    nextTask_ = nextTask;
}

Response ProjectFacade::handle(const Request& request) {
    const std::string name = toString(request.action());
    const std::string& projectId = request.ref().projectId;
    const std::string& taskId = request.ref().taskId;
    WorkflowInstance& instance = findInstance(projectId);
    Response response;

    switch (request.action()) {
        case Action::AddTask:
        case Action::AddSubtask: {
            std::string id = nextTaskId(instance);
            std::unique_ptr<Task> task = runtime(projectId).factory->createWorkItem(id, request.arg("name"), request.arg("role"));
            LeafTask* leaf = task->leaf();
            if (!request.arg("unit").empty()) leaf->setUnit(request.arg("unit"));
            task->approvalWork()->setApproverRoles(request.list("approvers"));
            if (!request.arg("priority").empty()) leaf->setPriority(request.arg("priority"));
            if (!request.arg("kind").empty()) leaf->setData("kind", request.arg("kind"));
            std::vector<std::string> blockers = request.list("blockedBy");
            for (std::size_t i = 0; i < blockers.size(); ++i) {
                task->blockedWork()->addBlocker(blockers[i]);
            }
            if (!instance.adoptItem(request.arg("parent"), std::move(task))) {
                throw std::logic_error(name + ": cannot add item '" + id + "' under '" + request.arg("parent") + "'");
            }
            response.put("item", id);
            changed(response);
            return response;
        }
        case Action::SignOff:
            instance.signOff();
            changed(response);
            return response;
        case Action::ReopenForFixes:
            instance.reopenForFixes(*runtime(projectId).factory);
            changed(response);
            return response;
        case Action::CancelProject:
            if (taskId.empty()) {
                try {
                    instance.cancel(request.arg("reason"));
                } catch (const std::logic_error& e) {
                    throw std::logic_error(name + ": cannot cancel instance: " + e.what());
                }
                response.put("loads", "changed");
                changed(response);
                return response;
            }
            break;
        default:
            break;
    }

    // The Request goes to the outermost node, so any decorators see it before the task does.
    Task* task = instance.find(taskId);
    if (leafOf(task) == nullptr) throw std::invalid_argument(name + ": unknown item '" + taskId + "'");
    try {
        task->handle(request);
    } catch (const std::invalid_argument&) {
        throw;
    } catch (const std::logic_error& e) {
        switch (request.action()) {
            case Action::Assign:
                throw std::logic_error(name + ": item '" + taskId + "' cannot be assigned to '" + request.arg("participant") + "'");
            case Action::Start: throw std::logic_error(name + ": item '" + taskId + "' cannot be started in its current state");
            case Action::Complete: throw std::logic_error(name + ": item '" + taskId + "' cannot be completed in its current state");
            case Action::Approve: throw std::logic_error(name + ": item '" + taskId + "' is not in a state that accepts approval");
            case Action::Reject: throw std::logic_error(name + ": item '" + taskId + "' cannot be rejected in its current state");
            case Action::Withdraw: throw std::logic_error("Only work waiting for approval can be taken back");
            default: throw std::logic_error(name + ": " + e.what());
        }
    }
    if (request.action() == Action::Approve) instance.advance();
    switch (request.action()) {
        case Action::Assign:
        case Action::Escalate:
        case Action::Approve:
        case Action::Reject:
        case Action::CancelProject:
            response.put("loads", "changed");
            break;
        default:
            break;
    }
    changed(response);
    return response;
}

std::shared_ptr<Memento> ProjectFacade::createMemento(const TaskRef& scope, const std::string& label) const {
    return findInstance(scope.projectId).createMemento(label);
}

void ProjectFacade::restore(const TaskRef& scope, Memento& memento) {
    findInstance(scope.projectId).restoreFromMemento(&memento);
}
