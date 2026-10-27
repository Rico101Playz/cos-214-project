#include "patterns/structural/Tividy.h"
#include "patterns/structural/LeafWorkItem.h"
#include "patterns/structural/OrgMember.h"
#include "patterns/execution/SequentialMode.h"
#include "patterns/execution/ParallelMode.h"
#include "core/TextRecord.h"
#include "patterns/creation/PolicyCatalog.h"
#include "patterns/creation/StateFactory.h"
#include <set>

#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace {

typedef std::vector<std::string> Fields;

std::string num(long long value) { return std::to_string(value); }

void put(std::string& out, const Fields& fields) { out += TextRecord::line(fields); }

void writeDefinition(std::string& out, const WorkflowDefinition& d) {
    put(out, Fields{ "dname", d.name });
    put(out, Fields{ "dpolicy", d.policy });
    put(out, Fields{ "dsignoff", d.signOff ? "1" : "0" });
    for (std::size_t i = 0; i < d.stages.size(); ++i) put(out, Fields{ "dstage", d.stages[i] });
    for (std::size_t i = 0; i < d.items.size(); ++i) put(out, Fields{ "ditem", d.items[i] });
    for (std::size_t i = 0; i < d.dependencies.size(); ++i) put(out, Fields{ "ddep", d.dependencies[i] });
    for (std::size_t i = 0; i < d.layout.size(); ++i) {
        put(out, Fields{ "dlayout", d.layout[i].id, d.layout[i].name, d.layout[i].parent });
    }
    for (std::size_t i = 0; i < d.templates.size(); ++i) {
        const ItemTemplate& t = d.templates[i];
        put(out, Fields{ "dtmpl", t.id, t.name, t.stage, t.role, t.unit, t.priority });
        for (std::size_t a = 0; a < t.approverRoles.size(); ++a) put(out, Fields{ "dtmpl-approver", t.approverRoles[a] });
        for (std::size_t b = 0; b < t.blockedBy.size(); ++b) put(out, Fields{ "dtmpl-blocker", t.blockedBy[b] });
    }
}

bool readDefinitionLine(const Fields& f, WorkflowDefinition& d) {
    const std::string& k = f[0];
    if (k == "dname" && f.size() >= 2) d.name = f[1];
    else if (k == "dpolicy" && f.size() >= 2) d.policy = f[1];
    else if (k == "dsignoff" && f.size() >= 2) d.signOff = f[1] == "1";
    else if (k == "dstage" && f.size() >= 2) d.stages.push_back(f[1]);
    else if (k == "ditem" && f.size() >= 2) d.items.push_back(f[1]);
    else if (k == "ddep" && f.size() >= 2) d.dependencies.push_back(f[1]);
    else if (k == "dlayout" && f.size() >= 4) d.layout.push_back(StageTemplate(f[1], f[2], f[3]));
    else if (k == "dtmpl" && f.size() >= 7) {
        ItemTemplate t(f[1], f[2], f[4], f[5], "", f[6]);
        t.stage = f[3];
        d.templates.push_back(t);
    } else if (k == "dtmpl-approver" && f.size() >= 2 && !d.templates.empty()) {
        d.templates.back().approverRoles.push_back(f[1]);
    } else if (k == "dtmpl-blocker" && f.size() >= 2 && !d.templates.empty()) {
        d.templates.back().blockedBy.push_back(f[1]);
    } else {
        return false;
    }
    return true;
}

void writeNode(std::string& out, WorkNode& node, const std::string& parentId) {
    CompositeStage* stage = dynamic_cast<CompositeStage*>(&node);
    if (stage != nullptr) {
        put(out, Fields{ "stage", stage->id(), stage->name(), parentId, stage->mode().name() });
    } else {
        LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(&node);
        if (leaf == nullptr) return;
        put(out, Fields{ "item", leaf->id(), leaf->name(), parentId, leaf->role(), num(leaf->dueHours()),
                         leaf->status(), leaf->assignee(), leaf->reason(), leaf->priority(),
                         leaf->isReleased() ? "1" : "0" });
        for (std::size_t i = 0; i < leaf->blockedBy().size(); ++i) put(out, Fields{ "blocker", leaf->blockedBy()[i] });
        for (std::size_t i = 0; i < leaf->approverRoles().size(); ++i) put(out, Fields{ "approver", leaf->approverRoles()[i] });
        for (std::size_t i = 0; i < leaf->pendingApprovers().size(); ++i) put(out, Fields{ "pending", leaf->pendingApprovers()[i] });
        for (std::size_t i = 0; i < leaf->signatures().size(); ++i) {
            put(out, Fields{ "signed", leaf->signatures()[i].first, leaf->signatures()[i].second });
        }
        for (std::map<std::string, std::string>::const_iterator it = leaf->data().begin(); it != leaf->data().end(); ++it) {
            put(out, Fields{ "data", it->first, it->second });
        }
    }
    for (std::size_t i = 0; i < node.childCount(); ++i) {
        writeNode(out, *node.child(i), node.id());
    }
}

void writeUnits(std::string& out, const OrgUnit& unit) {
    std::vector<OrgUnit*> subs = unit.subUnits();
    for (std::size_t i = 0; i < subs.size(); ++i) {
        put(out, Fields{ "unit", subs[i]->id(), subs[i]->name(), unit.parent() == nullptr ? "" : unit.id() });
        writeUnits(out, *subs[i]);
    }
}

std::unique_ptr<StageMode> modeNamed(const std::string& name) {
    if (name == "Sequential") return std::unique_ptr<StageMode>(new SequentialMode());
    return std::unique_ptr<StageMode>(new ParallelMode());
}

int toInt(const std::string& text) { return std::atoi(text.c_str()); }

struct PendingInstance {
    std::string id;
    std::string name;
    std::string policy;
    std::string state;
    std::size_t stageIndex;
    std::string reason;
    bool structured;
    bool signOff;
    WorkflowDefinition definition;
    std::unique_ptr<CompositeStage> root;
    std::map<std::string, WorkNode*> nodes;
    LeafWorkItem* lastItem;

    PendingInstance() : stageIndex(0), structured(false), signOff(false), lastItem(nullptr) {}
};

}

std::string Tividy::snapshot() const {
    std::string out;
    put(out, Fields{ "tividy", "2" });
    put(out, Fields{ "company", organisation_.name() });
    put(out, Fields{ "counters", num(nextInstance_), num(nextTask_) });
    writeUnits(out, organisation_.structure());
    std::vector<Participant*> people = organisation_.participants();
    for (std::size_t i = 0; i < people.size(); ++i) {
        OrgUnit* unit = organisation_.unitOf(people[i]->name());
        put(out, Fields{ "person", people[i]->name(), people[i]->role(), unit ? unit->id() : "",
                         people[i]->isAvailable() ? "1" : "0" });
    }
    for (std::map<std::string, WorkflowDefinition>::const_iterator it = definitions_.begin();
         it != definitions_.end(); ++it) {
        put(out, Fields{ "definition", it->first });
        writeDefinition(out, it->second);
        put(out, Fields{ "end" });
    }
    for (std::size_t i = 0; i < instanceOrder_.size(); ++i) {
        const WorkflowInstance& instance = findInstance(instanceOrder_[i]);
        const Runtime& rt = runtime(instanceOrder_[i]);
        put(out, Fields{ "instance", instance.id(), instance.name(), rt.factory->name(), instance.state(),
                         num(static_cast<long long>(instance.currentStageIndex())), instance.reason(),
                         rt.structured ? "1" : "0", instance.requiresSignOff() ? "1" : "0" });
        writeDefinition(out, instance.definition());
        put(out, Fields{ "root", instance.root().name(), instance.root().mode().name() });
        for (std::size_t c = 0; c < instance.root().childCount(); ++c) {
            writeNode(out, *instance.root().child(c), instance.id());
        }
        put(out, Fields{ "end" });
    }
    return out;
}

void Tividy::restore(const std::string& text) {
    std::istringstream in(text);
    std::string raw;
    std::string company;
    int nextInstance = 1;
    int nextTask = 1;
    std::vector<Fields> units;
    std::vector<Fields> people;
    std::map<std::string, WorkflowDefinition> definitions;
    std::vector<std::unique_ptr<PendingInstance> > pending;
    WorkflowDefinition* currentDefinition = nullptr;
    PendingInstance* current = nullptr;
    bool versionSeen = false;
    int lineNo = 0;

    while (std::getline(in, raw)) {
        ++lineNo;
        if (raw.empty() || raw[0] == '#' || raw == "\r") continue;
        Fields f = TextRecord::split(raw);
        const std::string& k = f[0];
        if (k == "log") continue;
        if (k == "tividy") { versionSeen = true; continue; }

        if (current != nullptr) {
            if (k == "end") { current = nullptr; continue; }
            if (readDefinitionLine(f, current->definition)) continue;
            if (k == "root" && f.size() >= 3) {
                current->root.reset(new CompositeStage(current->id, f[1]));
                current->root->setMode(modeNamed(f[2]));
                current->nodes[current->id] = current->root.get();
                continue;
            }
            if (!current->root) {
                throw std::invalid_argument("Line " + std::to_string(lineNo) + ": node before root");
            }
            if (k == "stage" && f.size() >= 5) {
                CompositeStage* parent = dynamic_cast<CompositeStage*>(current->nodes[f[3]]);
                if (parent == nullptr) throw std::invalid_argument("Line " + std::to_string(lineNo) + ": unknown parent " + f[3]);
                std::unique_ptr<CompositeStage> stage(new CompositeStage(f[1], f[2]));
                stage->setMode(modeNamed(f[4]));
                current->nodes[f[1]] = stage.get();
                parent->add(std::unique_ptr<WorkNode>(stage.release()));
                current->lastItem = nullptr;
                continue;
            }
            if (k == "item" && f.size() >= 11) {
                WorkNode* parentNode = current->nodes[f[3]];
                if (parentNode == nullptr) throw std::invalid_argument("Line " + std::to_string(lineNo) + ": unknown parent " + f[3]);
                std::unique_ptr<LeafWorkItem> leaf(new LeafWorkItem(f[1], f[2], f[4], toInt(f[5])));
                leaf->restore(f[6], f[7], f[8]);
                leaf->setPriority(LeafWorkItem::validPriority(f[9]) ? f[9] : std::string("Medium"));
                leaf->setReleased(f[10] == "1");
                LeafWorkItem* added = leaf.get();
                CompositeStage* stage = dynamic_cast<CompositeStage*>(parentNode);
                if (stage != nullptr) {
                    stage->add(std::unique_ptr<WorkNode>(leaf.release()));
                } else {
                    LeafWorkItem* parentItem = dynamic_cast<LeafWorkItem*>(parentNode);
                    if (parentItem == nullptr) throw std::invalid_argument("Line " + std::to_string(lineNo) + ": bad parent");
                    parentItem->addSubtask(std::move(leaf));
                }
                current->nodes[f[1]] = added;
                current->lastItem = added;
                continue;
            }
            LeafWorkItem* item = current->lastItem;
            if (item == nullptr) throw std::invalid_argument("Line " + std::to_string(lineNo) + ": detail without an item");
            if (k == "blocker" && f.size() >= 2) item->addBlocker(f[1]);
            else if (k == "approver" && f.size() >= 2) {
                std::vector<std::string> roles = item->approverRoles();
                roles.push_back(f[1]);
                item->setApproverRoles(roles);
            } else if (k == "pending" && f.size() >= 2) {
                std::vector<std::string> roles = item->pendingApprovers();
                roles.push_back(f[1]);
                item->setPendingApprovers(roles);
            } else if (k == "signed" && f.size() >= 3) item->addSignature(f[1], f[2]);
            else if (k == "data" && f.size() >= 3) item->setData(f[1], f[2]);
            else throw std::invalid_argument("Line " + std::to_string(lineNo) + ": unexpected '" + k + "'");
            continue;
        }

        if (currentDefinition != nullptr) {
            if (k == "end") { currentDefinition = nullptr; continue; }
            if (!readDefinitionLine(f, *currentDefinition)) {
                throw std::invalid_argument("Line " + std::to_string(lineNo) + ": unexpected '" + k + "' in a definition");
            }
            continue;
        }

        if (k == "company" && f.size() >= 2) company = f[1];
        else if (k == "counters" && f.size() >= 3) { nextInstance = toInt(f[1]); nextTask = toInt(f[2]); }
        else if (k == "unit" && f.size() >= 4) units.push_back(f);
        else if (k == "person" && f.size() >= 5) people.push_back(f);
        else if (k == "definition" && f.size() >= 2) currentDefinition = &definitions[f[1]];
        else if (k == "instance" && f.size() >= 9) {
            std::unique_ptr<PendingInstance> p(new PendingInstance());
            p->id = f[1];
            p->name = f[2];
            p->policy = f[3];
            p->state = f[4];
            p->stageIndex = static_cast<std::size_t>(toInt(f[5]));
            p->reason = f[6];
            p->structured = f[7] == "1";
            p->signOff = f[8] == "1";
            current = p.get();
            pending.push_back(std::move(p));
        } else {
            throw std::invalid_argument("Line " + std::to_string(lineNo) + ": unexpected '" + k + "'");
        }
    }
    if (!versionSeen || company.empty()) {
        throw std::invalid_argument("Not a TIVIDY scenario file");
    }
    std::set<std::string> unitIds;
    unitIds.insert("org");
    for (std::size_t i = 0; i < units.size(); ++i) {
        const std::string& parent = units[i][3];
        if (!parent.empty() && unitIds.count(parent) == 0) {
            throw std::invalid_argument("Unit " + units[i][1] + " names an unknown parent " + parent);
        }
        if (units[i][1].empty() || !unitIds.insert(units[i][1]).second) {
            throw std::invalid_argument("Duplicate or empty unit id: " + units[i][1]);
        }
    }
    std::set<std::string> names;
    for (std::size_t i = 0; i < people.size(); ++i) {
        if (people[i][1].empty() || !names.insert(people[i][1]).second || unitIds.count(people[i][1]) != 0) {
            throw std::invalid_argument("Duplicate or clashing person: " + people[i][1]);
        }
        if (!people[i][3].empty() && unitIds.count(people[i][3]) == 0) {
            throw std::invalid_argument(people[i][1] + " is in an unknown unit " + people[i][3]);
        }
    }

    Company fresh(company);
    for (std::size_t i = 0; i < units.size(); ++i) {
        fresh.addUnit(units[i][3], units[i][1], units[i][2]);
    }
    for (std::size_t i = 0; i < people.size(); ++i) {
        Participant& p = fresh.addParticipant(people[i][1], people[i][2]);
        if (!people[i][3].empty()) fresh.placeMember(people[i][3], people[i][1]);
        p.setAvailable(people[i][4] == "1");
    }

    std::map<std::string, std::unique_ptr<Runtime> > runtimes;
    std::set<std::string> instanceIds;
    for (std::size_t i = 0; i < pending.size(); ++i) {
        PendingInstance& p = *pending[i];
        if (!p.root) throw std::invalid_argument("Project " + p.id + " has no work tree");
        if (!instanceIds.insert(p.id).second) throw std::invalid_argument("Duplicate project id: " + p.id);
        std::size_t stages = p.root->childCount();
        if (stages == 0 ? p.stageIndex != 0 : p.stageIndex >= stages) {
            throw std::invalid_argument("Project " + p.id + " points at a stage it does not have");
        }
        if (!p.policy.empty() && !PolicyCatalog::exists(p.policy)) {
            throw std::invalid_argument("Project " + p.id + " uses an unknown policy " + p.policy);
        }
        createInstanceState(p.state);
        runtimes[p.id] = makeRuntime(p.policy, p.structured, fresh.people());
    }

    organisation_.swap(fresh);
    definitions_ = definitions;

    std::map<std::string, std::unique_ptr<WorkflowInstance> > instances;
    std::vector<std::string> order;
    for (std::size_t i = 0; i < pending.size(); ++i) {
        PendingInstance& p = *pending[i];
        std::unique_ptr<WorkflowInstance> instance;
        std::map<std::string, std::unique_ptr<WorkflowInstance> >::iterator existing = instances_.find(p.id);
        if (existing != instances_.end()) {
            instance = std::move(existing->second);
        } else {
            instance.reset(new WorkflowInstance(p.id, p.definition));
        }
        instance->rebuild(p.definition, p.name, std::move(p.root), p.state, p.stageIndex, p.reason, p.signOff);
        instances[p.id] = std::move(instance);
        order.push_back(p.id);
    }
    instances_.swap(instances);
    runtimes_.swap(runtimes);
    instanceOrder_ = order;
    nextInstance_ = nextInstance;
    nextTask_ = nextTask;
    recomputeLoads();
}
