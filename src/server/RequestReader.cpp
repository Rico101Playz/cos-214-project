#include "server/RequestReader.h"
#include "server/ClientSession.h"
#include "server/ViewBuilder.h"
#include "mediator/TividyMediator.h"
#include "proxy/SecuredMediator.h"
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

TaskRef item(const Json& args) { return TaskRef(text(args, "project"), text(args, "item")); }

// The task fields shared by addTask and addSubtask.
Request& withTask(Request& request, const Json& args) {
    request.with("name", text(args, "name"))
        .with("stage", text(args, "stage", false))
        .with("unit", text(args, "unit", false))
        .with("role", text(args, "role", false))
        .with("approver", text(args, "approver", false))
        .with("priority", text(args, "priority", false))
        .with("assignee", text(args, "assignee", false))
        .with("autoAssign", args.get("autoAssign").asBool(true) ? "true" : "false")
        .withList("approvers", textList(args, "approvers", 10))
        .withList("blockedBy", textList(args, "blockedBy", 20));
    return request;
}

Json ok() {
    return Json::object();
}

Json login(TividyMediator& engine, ClientSession& session, const Json& args) {
    std::string name = text(args, "as");
    if (engine.organisation().find(name) == nullptr) {
        throw std::invalid_argument("Unknown person: " + name);
    }
    session.logout();
    session.login(name);
    return ok();
}

Json logout(TividyMediator& engine, ClientSession& session, const Json&) {
    session.logout();
    session.send(ViewBuilder::hello(engine));
    return ok();
}

Request startProject(const Json& args, const std::string& actor) {
    return Request(Action::StartProject, TaskRef(), actor)
        .with("definition", text(args, "template"))
        .with("title", text(args, "title", false))
        .with("policy", text(args, "policy", false))
        .with("signOff", text(args, "signOff", false));
}

Request addTask(const Json& args, const std::string& actor) {
    Request request(Action::AddTask, TaskRef(text(args, "project")), actor);
    return withTask(request, args);
}

Request addSubtask(const Json& args, const std::string& actor) {
    Request request(Action::AddSubtask, item(args), actor);
    return withTask(request, args);
}

Request withdraw(const Json& args, const std::string& actor) { return Request(Action::Withdraw, item(args), actor); }

Request setPriority(const Json& args, const std::string& actor) {
    return Request(Action::SetPriority, item(args), actor).with("priority", text(args, "priority"));
}

Request signOff(const Json& args, const std::string& actor) {
    return Request(Action::SignOff, TaskRef(text(args, "project")), actor);
}

Request flagIssue(const Json& args, const std::string& actor) {
    return Request(Action::FlagIssue, TaskRef(text(args, "project")), actor)
        .with("name", text(args, "name"))
        .with("priority", text(args, "priority", false))
        .withList("units", textList(args, "units", 10));
}

Request assign(const Json& args, const std::string& actor) {
    return Request(Action::Assign, item(args), actor).with("participant", text(args, "to"));
}

// Taking a task is assigning it to yourself.
Request claim(const Json& args, const std::string& actor) {
    return Request(Action::Assign, item(args), actor).with("participant", actor);
}

Request autoAssign(const Json& args, const std::string& actor) { return Request(Action::AutoAssign, item(args), actor); }
Request begin(const Json& args, const std::string& actor) { return Request(Action::Start, item(args), actor); }
Request done(const Json& args, const std::string& actor) { return Request(Action::Complete, item(args), actor); }
Request approve(const Json& args, const std::string& actor) { return Request(Action::Approve, item(args), actor); }
Request escalate(const Json& args, const std::string& actor) { return Request(Action::Escalate, item(args), actor); }

Request reject(const Json& args, const std::string& actor) {
    return Request(Action::Reject, item(args), actor).with("reason", text(args, "reason"));
}

Request cancel(const Json& args, const std::string& actor) {
    return Request(Action::CancelProject, TaskRef(text(args, "project")), actor).with("reason", text(args, "reason", false));
}

Request undo(const Json&, const std::string& actor) { return Request(Action::Undo, TaskRef(), actor); }
Request redo(const Json&, const std::string& actor) { return Request(Action::Redo, TaskRef(), actor); }

Request addUnit(const Json& args, const std::string& actor) {
    return Request(Action::AddUnit, TaskRef(), actor)
        .with("parent", text(args, "parent", false))
        .with("id", text(args, "id"))
        .with("name", text(args, "name"));
}

Request addPerson(const Json& args, const std::string& actor) {
    return Request(Action::AddPerson, TaskRef(), actor)
        .with("name", text(args, "name"))
        .with("role", text(args, "role"))
        .with("unit", text(args, "unit"));
}

Request moveMember(const Json& args, const std::string& actor) {
    return Request(Action::MoveMember, TaskRef(), actor)
        .with("unit", text(args, "unit"))
        .with("name", text(args, "name"));
}

// The Response's results as the reply's data object: values as text, lists as arrays.
Json dataOf(const Response& response) {
    Json out = Json::object();
    for (std::map<std::string, std::string>::const_iterator it = response.data().begin(); it != response.data().end(); ++it) {
        out.set(it->first, it->second);
    }
    typedef std::map<std::string, std::vector<std::string> > Lists;
    for (Lists::const_iterator it = response.lists().begin(); it != response.lists().end(); ++it) {
        Json list = Json::array();
        for (std::size_t i = 0; i < it->second.size(); ++i) list.push(it->second[i]);
        out.set(it->first, list);
    }
    return out;
}

}

RequestReader::RequestReader(TividyMediator& engine) : engine_(engine) {
    sessionHandlers_["login"] = &login;
    sessionHandlers_["logout"] = &logout;
    readers_["startProject"] = &startProject;
    readers_["addTask"] = &addTask;
    readers_["assign"] = &assign;
    readers_["claim"] = &claim;
    readers_["autoAssign"] = &autoAssign;
    readers_["begin"] = &begin;
    readers_["done"] = &done;
    readers_["approve"] = &approve;
    readers_["reject"] = &reject;
    readers_["escalate"] = &escalate;
    readers_["cancel"] = &cancel;
    readers_["undo"] = &undo;
    readers_["addUnit"] = &addUnit;
    readers_["addPerson"] = &addPerson;
    readers_["moveMember"] = &moveMember;
    readers_["addSubtask"] = &addSubtask;
    readers_["withdraw"] = &withdraw;
    readers_["setPriority"] = &setPriority;
    readers_["signOff"] = &signOff;
    readers_["flagIssue"] = &flagIssue;
    readers_["redo"] = &redo;
}

bool RequestReader::knows(const std::string& command) const {
    return readers_.find(command) != readers_.end() || sessionHandlers_.find(command) != sessionHandlers_.end();
}

Json RequestReader::handle(ClientSession& session, const std::string& raw) {
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
        if (!knows(command)) {
            throw JsonError("Unknown command: " + command);
        }
        if (command != "login" && !session.loggedIn()) {
            throw AccessDenied("Choose who you are first");
        }
        const Json& given = message.get("args");
        const Json& args = given.isObject() ? given : Json::object();
        std::map<std::string, SessionHandler>::const_iterator own = sessionHandlers_.find(command);
        if (own != sessionHandlers_.end()) {
            reply.set("ok", true);
            reply.set("data", own->second(engine_, session, args));
            return reply;
        }
        SecuredMediator& proxy = session.proxy();
        Response response = proxy.send(readers_.find(command)->second(args, proxy.actor()));
        reply.set("ok", response.ok());
        if (response.ok()) {
            reply.set("data", dataOf(response));
        } else {
            reply.set("code", response.code());
            reply.set("error", response.message());
        }
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
    } catch (const std::exception& e) {
        reply.set("ok", false);
        reply.set("code", "error");
        reply.set("error", e.what());
    }
    return reply;
}
