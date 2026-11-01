#include "server/ViewBuilder.h"
#include "patterns/structural/Tividy.h"
#include "patterns/structural/SecuredTividy.h"
#include "patterns/structural/LeafWorkItem.h"
#include "patterns/structural/OrgMember.h"
#include "patterns/creation/PolicyCatalog.h"
#include "server/ActivityLog.h"
#include <set>

namespace {

std::string unitIdOf(const Company& org, const std::string& name) {
    OrgUnit* unit = org.unitOf(name);
    return unit != nullptr ? unit->id() : std::string();
}

std::string unitPath(const Company& org, const std::string& unitId) {
    OrgUnit* unit = org.findUnit(unitId);
    return unit != nullptr ? unit->path() : std::string();
}

Json orgTree(const OrgComponent& node) {
    Json out = Json::object();
    out.set("id", node.id());
    out.set("name", node.name());
    out.set("unit", node.isUnit());
    if (node.isUnit()) {
        Json children = Json::array();
        for (std::size_t i = 0; i < node.childCount(); ++i) {
            children.push(orgTree(*node.child(i)));
        }
        out.set("children", children);
    } else {
        const OrgMember& member = static_cast<const OrgMember&>(node);
        out.set("role", member.participant()->role());
    }
    return out;
}

Json personJson(const Company& org, const Participant& p) {
    Json out = Json::object();
    out.set("name", p.name());
    out.set("role", p.role());
    out.set("unit", unitIdOf(org, p.name()));
    out.set("unitPath", unitPath(org, unitIdOf(org, p.name())));
    out.set("load", p.load());
    out.set("manager", org.isManager(p.name()));
    out.set("owner", org.isOwner(p.name()));
    return out;
}

Json strings(const std::vector<std::string>& values) {
    Json out = Json::array();
    for (std::size_t i = 0; i < values.size(); ++i) out.push(values[i]);
    return out;
}

Json refs(const std::vector<ItemRef>& values) {
    Json out = Json::array();
    for (std::size_t i = 0; i < values.size(); ++i) {
        Json ref = Json::object();
        ref.set("project", values[i].instanceId);
        ref.set("item", values[i].itemId);
        out.push(ref);
    }
    return out;
}

void collect(Tividy& engine, const SecuredTividy& viewer, const WorkflowInstance& instance,
             WorkNode& node, const std::string& parentStage, Json& stages, Json& items) {
    const Company& org = engine.organisation();
    CompositeStage* stage = dynamic_cast<CompositeStage*>(&node);
    if (stage != nullptr) {
        std::string stageId = parentStage;
        if (&node != &instance.root()) {
            Json s = Json::object();
            s.set("id", stage->id());
            s.set("name", stage->name());
            s.set("parent", parentStage);
            s.set("mode", stage->mode().name());
            s.set("active", instance.isActive(stage));
            s.set("complete", stage->isComplete());
            stages.push(s);
            stageId = stage->id();
        }
        for (std::size_t i = 0; i < stage->childCount(); ++i) {
            collect(engine, viewer, instance, *stage->child(i), stageId, stages, items);
        }
        return;
    }

    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(&node);
    if (leaf == nullptr) return;
    const std::string& id = instance.id();
    LeafWorkItem* parentItem = leaf->parentItem();

    Json item = Json::object();
    item.set("id", leaf->id());
    item.set("name", leaf->name());
    item.set("stage", parentStage);
    item.set("parentItem", parentItem != nullptr ? parentItem->id() : std::string());
    item.set("kind", leaf->getData("kind", parentItem != nullptr ? "subtask" : "task"));
    item.set("status", leaf->status());
    item.set("assignee", leaf->assignee());
    item.set("assignedVia", leaf->getData("assignedVia"));
    item.set("role", leaf->role());
    item.set("unit", leaf->unit());
    item.set("unitPath", unitPath(org, leaf->unit()));
    item.set("priority", leaf->priority());
    item.set("approverRoles", strings(leaf->approverRoles()));
    item.set("pendingApprovers", strings(leaf->pendingApprovers()));
    Json signatures = Json::array();
    for (std::size_t i = 0; i < leaf->signatures().size(); ++i) {
        Json sig = Json::object();
        sig.set("role", leaf->signatures()[i].first);
        sig.set("person", leaf->signatures()[i].second);
        signatures.push(sig);
    }
    item.set("signatures", signatures);
    item.set("reason", leaf->reason());
    item.set("waiting", leaf->isWaiting());
    item.set("dueHours", leaf->dueHours());
    item.set("blockedBy", strings(leaf->blockedBy()));
    item.set("openBlockers", strings(leaf->openBlockers()));
    Json subtasks = Json::array();
    for (std::size_t i = 0; i < leaf->childCount(); ++i) subtasks.push(leaf->child(i)->id());
    item.set("subtasks", subtasks);
    item.set("openSubtasks", strings(leaf->openSubtasks()));

    Json actions = Json::array();
    bool running = instance.state() == WorkflowInstance::RUNNING;
    const std::string& status = leaf->status();
    bool finished = status == WorkItem::COMPLETED || status == WorkItem::CANCELLED;
    if (running) {
        if (viewer.mayClaim(id, leaf->id()) && !viewer.isManager()) actions.push("claim");
        if (viewer.isManager() && (status == WorkItem::AVAILABLE || status == WorkItem::ASSIGNED ||
                                   status == WorkItem::REJECTED || status == WorkItem::ESCALATED)) {
            actions.push("assign");
            actions.push("autoAssign");
        }
        if (viewer.isManager() && status == WorkItem::AVAILABLE) actions.push("claim");
        if (viewer.mayWork(id, leaf->id())) {
            if (status == WorkItem::ASSIGNED || status == WorkItem::REJECTED) actions.push("begin");
            if (status == WorkItem::IN_PROGRESS) actions.push("done");
        }
        if (viewer.mayWithdraw(id, leaf->id())) actions.push("withdraw");
        if (viewer.mayDecide(id, leaf->id())) {
            actions.push("approve");
            actions.push("reject");
        }
        if (viewer.mayEscalate(id, leaf->id()) &&
            (status == WorkItem::ASSIGNED || status == WorkItem::IN_PROGRESS)) {
            actions.push("escalate");
        }
        if (!finished && viewer.mayAddSubtask(id, leaf->id())) actions.push("subtask");
        if (!finished && viewer.isManager()) actions.push("priority");
    }
    item.set("actions", actions);
    items.push(item);
    for (std::size_t i = 0; i < leaf->childCount(); ++i) {
        collect(engine, viewer, instance, *leaf->child(i), parentStage, stages, items);
    }
}

}

Json ViewBuilder::hello(const Tividy& engine) {
    const Company& org = engine.organisation();
    Json out = Json::object();
    out.set("type", "hello");
    out.set("company", org.name());
    Json people = Json::array();
    std::vector<Participant*> all = org.participants();
    for (std::size_t i = 0; i < all.size(); ++i) {
        people.push(personJson(org, *all[i]));
    }
    out.set("people", people);
    return out;
}

Json ViewBuilder::state(Tividy& engine, const SecuredTividy& viewer, const ActivityLog* log) {
    const Company& org = engine.organisation();
    if (org.find(viewer.actor()) == nullptr) return hello(engine);
    Json out = Json::object();
    out.set("type", "state");
    out.set("company", org.name());
    out.set("me", personJson(org, *org.find(viewer.actor())));

    Json people = Json::array();
    std::set<std::string> roles;
    roles.insert("Developer");
    roles.insert("Designer");
    roles.insert("QA");
    roles.insert("Reviewer");
    roles.insert("TechLead");
    roles.insert("ReleaseManager");
    roles.insert(Company::MANAGER_ROLE);
    roles.insert(Company::OWNER_ROLE);
    std::vector<Participant*> all = org.participants();
    for (std::size_t i = 0; i < all.size(); ++i) {
        people.push(personJson(org, *all[i]));
        roles.insert(all[i]->role());
    }
    out.set("people", people);
    out.set("roles", strings(std::vector<std::string>(roles.begin(), roles.end())));
    out.set("org", orgTree(org.structure()));
    out.set("templates", strings(engine.definitionNames()));
    Json templateInfo = Json::array();
    std::vector<std::string> names = engine.definitionNames();
    for (std::size_t i = 0; i < names.size(); ++i) {
        Json info = Json::object();
        info.set("name", names[i]);
        info.set("signOff", engine.definition(names[i]).signOff);
        info.set("policy", engine.definition(names[i]).policy);
        templateInfo.push(info);
    }
    out.set("templateInfo", templateInfo);
    out.set("policies", strings(PolicyCatalog::names()));

    Json projects = Json::array();
    std::vector<std::string> ids = engine.instanceIds();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const WorkflowInstance& instance = engine.instance(ids[i]);
        Json project = Json::object();
        project.set("id", instance.id());
        project.set("name", instance.name());
        project.set("template", instance.definition().name);
        project.set("policy", engine.policyOf(ids[i]));
        project.set("state", instance.state());
        project.set("reason", instance.reason());
        project.set("progress", engine.progress(ids[i]));
        project.set("canSignOff", viewer.maySignOff(ids[i]));
        project.set("signOffRequired", instance.requiresSignOff());
        WorkNode* current = instance.currentStage();
        project.set("currentStage", current != nullptr && current != &instance.root() ? current->id() : std::string());
        Json stages = Json::array();
        Json items = Json::array();
        collect(engine, viewer, instance, instance.root(), "", stages, items);
        project.set("stages", stages);
        project.set("items", items);
        projects.push(project);
    }
    out.set("projects", projects);
    out.set("myWork", refs(engine.workFor(viewer.actor())));
    out.set("approvals", refs(engine.approvalsFor(viewer.actor())));
    out.set("signOffs", strings(engine.signOffsFor(viewer.actor())));
    out.set("priorities", strings(std::vector<std::string>(LeafWorkItem::PRIORITIES, LeafWorkItem::PRIORITIES + 4)));

    Json history = Json::object();
    history.set("undo", strings(engine.undoLabels(8)));
    history.set("redo", strings(engine.redoLabels(8)));
    history.set("canUndo", viewer.mayUndo());
    history.set("canRedo", viewer.mayRedo());
    out.set("history", history);
    out.set("activity", strings(log != nullptr ? log->latest(60) : std::vector<std::string>()));
    return out;
}
