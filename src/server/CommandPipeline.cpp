#include "server/CommandPipeline.h"
#include "server/ClientSession.h"
#include "server/ViewBuilder.h"
#include "patterns/structural/Tividy.h"
#include "patterns/structural/SecuredTividy.h"
#include <vector>

namespace {

const std::size_t MAX_FIELD = 200;

std::string text(const Json& args, const std::string& key, bool required = true) {
    const Json& value = args.get(key);
    if (value.isNull() && !required) return "";
    if (!value.isString()) {
        throw std::invalid_argument("Field '" + key + "' must be text");
    }
    const std::string& s = value.asString();
    if (required && s.empty()) {
        throw std::invalid_argument("Field '" + key + "' is required");
    }
    if (s.size() > MAX_FIELD) {
        throw std::invalid_argument("Field '" + key + "' is too long");
    }
    return s;
}

std::vector<std::string> textList(const Json& args, const std::string& key, std::size_t max) {
    std::vector<std::string> out;
    const Json& value = args.get(key);
    if (value.isNull()) return out;
    if (!value.isArray()) throw std::invalid_argument("Field '" + key + "' must be a list");
    if (value.size() > max) throw std::invalid_argument("Too many entries in '" + key + "'");
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (!value.at(i).isString() || value.at(i).asString().size() > MAX_FIELD) {
            throw std::invalid_argument("Entries in '" + key + "' must be short text");
        }
        if (!value.at(i).asString().empty()) out.push_back(value.at(i).asString());
    }
    return out;
}

TaskSpec taskSpec(const Json& args) {
    TaskSpec spec;
    spec.name = text(args, "name");
    spec.stageId = text(args, "stage", false);
    spec.unit = text(args, "unit", false);
    spec.role = text(args, "role", false);
    spec.approverRole = text(args, "approver", false);
    spec.approverRoles = textList(args, "approvers", 10);
    spec.priority = text(args, "priority", false);
    spec.assignee = text(args, "assignee", false);
    spec.autoAssign = args.get("autoAssign").asBool(true);
    spec.blockedBy = textList(args, "blockedBy", 20);
    return spec;
}

Json ok() {
    return Json::object();
}

Json login(Tividy& engine, ClientSession& session, const Json& args) {
    std::string name = text(args, "as");
    if (engine.organisation().find(name) == nullptr) {
        throw std::invalid_argument("Unknown person: " + name);
    }
    session.logout();
    session.login(name);
    return ok();
}

Json logout(Tividy& engine, ClientSession& session, const Json&) {
    session.logout();
    session.send(ViewBuilder::hello(engine));
    return ok();
}

Json startProject(Tividy&, ClientSession& session, const Json& args) {
    std::string rule = text(args, "signOff", false);
    SignOffRule signOff = SignOffRule::FromTemplate;
    if (rule == "required") signOff = SignOffRule::Required;
    else if (rule == "skip") signOff = SignOffRule::NotRequired;
    else if (!rule.empty() && rule != "template") throw std::invalid_argument("signOff must be template, required or skip");
    std::string id = session.proxy().startProject(text(args, "template"), text(args, "title", false),
                                                 text(args, "policy", false), signOff);
    Json out = Json::object();
    out.set("project", id);
    return out;
}

Json addTask(Tividy&, ClientSession& session, const Json& args) {
    std::string id = session.proxy().addTask(text(args, "project"), taskSpec(args));
    Json out = Json::object();
    out.set("item", id);
    return out;
}

Json addSubtask(Tividy&, ClientSession& session, const Json& args) {
    std::string id = session.proxy().addSubtask(text(args, "project"), text(args, "item"), taskSpec(args));
    Json out = Json::object();
    out.set("item", id);
    return out;
}

Json withdraw(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().withdraw(text(args, "project"), text(args, "item"));
    return ok();
}

Json setPriority(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().setPriority(text(args, "project"), text(args, "item"), text(args, "priority"));
    return ok();
}

Json signOff(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().signOff(text(args, "project"));
    return ok();
}

Json flagIssue(Tividy&, ClientSession& session, const Json& args) {
    IssueSpec spec;
    spec.name = text(args, "name");
    spec.units = textList(args, "units", 10);
    spec.priority = text(args, "priority", false);
    std::vector<std::string> ids = session.proxy().flagIssue(text(args, "project"), spec);
    Json list = Json::array();
    for (std::size_t i = 0; i < ids.size(); ++i) list.push(ids[i]);
    Json out = Json::object();
    out.set("items", list);
    return out;
}

Json redo(Tividy&, ClientSession& session, const Json&) {
    session.proxy().redo();
    return ok();
}

Json assign(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().assign(text(args, "project"), text(args, "item"), text(args, "to"));
    return ok();
}

Json claim(Tividy&, ClientSession& session, const Json& args) {
    SecuredTividy& proxy = session.proxy();
    proxy.assign(text(args, "project"), text(args, "item"), proxy.actor());
    return ok();
}

Json autoAssign(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().autoAssign(text(args, "project"), text(args, "item"));
    return ok();
}

Json begin(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().start(text(args, "project"), text(args, "item"));
    return ok();
}

Json done(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().complete(text(args, "project"), text(args, "item"));
    return ok();
}

Json approve(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().approve(text(args, "project"), text(args, "item"));
    return ok();
}

Json reject(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().reject(text(args, "project"), text(args, "item"), text(args, "reason"));
    return ok();
}

Json escalate(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().escalate(text(args, "project"), text(args, "item"));
    return ok();
}

Json cancel(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().cancel(text(args, "project"), text(args, "reason", false));
    return ok();
}

Json undo(Tividy&, ClientSession& session, const Json&) {
    session.proxy().undo();
    return ok();
}

Json addUnit(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().addUnit(text(args, "parent", false), text(args, "id"), text(args, "name"));
    return ok();
}

Json addPerson(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().addPerson(text(args, "name"), text(args, "role"), text(args, "unit"));
    return ok();
}

Json moveMember(Tividy&, ClientSession& session, const Json& args) {
    session.proxy().moveMember(text(args, "unit"), text(args, "name"));
    return ok();
}

}

CommandPipeline::CommandPipeline(Tividy& engine) : engine_(engine) {
    handlers_["login"] = &login;
    handlers_["logout"] = &logout;
    handlers_["startProject"] = &startProject;
    handlers_["addTask"] = &addTask;
    handlers_["assign"] = &assign;
    handlers_["claim"] = &claim;
    handlers_["autoAssign"] = &autoAssign;
    handlers_["begin"] = &begin;
    handlers_["done"] = &done;
    handlers_["approve"] = &approve;
    handlers_["reject"] = &reject;
    handlers_["escalate"] = &escalate;
    handlers_["cancel"] = &cancel;
    handlers_["undo"] = &undo;
    handlers_["addUnit"] = &addUnit;
    handlers_["addPerson"] = &addPerson;
    handlers_["moveMember"] = &moveMember;
    handlers_["addSubtask"] = &addSubtask;
    handlers_["withdraw"] = &withdraw;
    handlers_["setPriority"] = &setPriority;
    handlers_["signOff"] = &signOff;
    handlers_["flagIssue"] = &flagIssue;
    handlers_["redo"] = &redo;
}

bool CommandPipeline::knows(const std::string& command) const {
    return handlers_.find(command) != handlers_.end();
}

Json CommandPipeline::handle(ClientSession& session, const std::string& raw) {
    Json reply = Json::object();
    reply.set("type", "result");
    try {
        if (raw.size() > MAX_MESSAGE) {
            throw JsonError("Message too large");
        }
        Json message = Json::parse(raw);
        if (!message.isObject()) {
            throw JsonError("A command must be an object");
        }
        const Json& ref = message.get("ref");
        if (ref.type() == Json::NUMBER || ref.isString()) {
            reply.set("ref", ref);
        }
        std::string command = message.str("cmd");
        reply.set("cmd", command);
        std::map<std::string, Handler>::const_iterator it = handlers_.find(command);
        if (it == handlers_.end()) {
            throw JsonError("Unknown command: " + command);
        }
        if (command != "login" && !session.loggedIn()) {
            throw AccessDenied("Choose who you are first");
        }
        const Json& args = message.get("args");
        Json result = it->second(engine_, session, args.isObject() ? args : Json::object());
        reply.set("ok", true);
        reply.set("data", result);
    } catch (const JsonError& e) {
        reply.set("ok", false);
        reply.set("code", "bad_request");
        reply.set("error", e.what());
    } catch (const AccessDenied& e) {
        reply.set("ok", false);
        reply.set("code", "forbidden");
        reply.set("error", e.what());
    } catch (const std::invalid_argument& e) {
        reply.set("ok", false);
        reply.set("code", "invalid");
        reply.set("error", e.what());
    } catch (const std::logic_error& e) {
        reply.set("ok", false);
        reply.set("code", "not_allowed_now");
        reply.set("error", e.what());
    } catch (const std::exception& e) {
        reply.set("ok", false);
        reply.set("code", "error");
        reply.set("error", e.what());
    }
    return reply;
}
