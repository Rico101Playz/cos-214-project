#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>

#include "patterns/structural/Tividy.h"
#include "server/ClientSession.h"
#include "server/CommandPipeline.h"
#include "server/DemoSeed.h"
#include "server/Json.h"
#include "server/SessionHub.h"
#include "server/ActivityLog.h"

namespace {

bool parses(const std::string& text) {
    try { Json::parse(text); } catch (const JsonError&) { return false; }
    return true;
}

TEST(Server, Json) {
    Json v = Json::parse(" {\"a\": [1, 2.5, -3e2, true, false, null], \"s\": \"x\\n\\u00e9\\ud83d\\ude00\"} ");
    EXPECT_TRUE(v.isObject() && v.get("a").size() == 6) << "Json: parses objects and arrays";
    EXPECT_TRUE(v.get("a").at(1).asNumber() == 2.5 && v.get("a").at(2).asNumber() == -300) << "Json: parses numbers";
    EXPECT_TRUE(v.get("a").at(3).asBool() && !v.get("a").at(4).asBool(true) && v.get("a").at(5).isNull()) << "Json: parses literals";
    EXPECT_EQ(v.get("s").asString(), "x\n\xc3\xa9\xf0\x9f\x98\x80") << "Json: decodes escapes and surrogate pairs to UTF-8";
    EXPECT_TRUE(v.get("missing").isNull() && v.str("missing", "d") == "d") << "Json: missing keys read as null";
    EXPECT_EQ(Json::parse(v.dump()).dump(), v.dump()) << "Json: dump() round-trips";
    EXPECT_EQ(Json(0.1 + 0.2).dump(), Json::parse(Json(0.1 + 0.2).dump()).dump()) << "Json: non-integers keep full precision";
    EXPECT_EQ(Json("</script>").dump(), "\"\\u003c/script>\"") << "Json: '<' is escaped in output";

    EXPECT_FALSE(parses("{")) << "Json: rejects unterminated objects";
    EXPECT_FALSE(parses("[1,]")) << "Json: rejects trailing commas";
    EXPECT_FALSE(parses("{\"a\":1} x")) << "Json: rejects trailing characters";
    EXPECT_FALSE(parses("\"\\x\"")) << "Json: rejects bad escapes";
    EXPECT_FALSE(parses("\"\\ud83d\"")) << "Json: rejects lone surrogates";
    EXPECT_FALSE(parses("01")) << "Json: rejects leading zeros";
    EXPECT_FALSE(parses("\"a\nb\"")) << "Json: rejects raw control characters";
    EXPECT_FALSE(parses(std::string(40, '[') + std::string(40, ']'))) << "Json: rejects very deep nesting";
}

struct Inbox {
    std::vector<Json> messages;
    MessageSink sink() {
        return [this](const std::string& text) { messages.push_back(Json::parse(text)); };
    }
    const Json* last(const std::string& type) const {
        for (std::size_t i = messages.size(); i > 0; --i) {
            if (messages[i - 1].str("type") == type) return &messages[i - 1];
        }
        return nullptr;
    }
    int count(const std::string& type) const {
        int n = 0;
        for (std::size_t i = 0; i < messages.size(); ++i) if (messages[i].str("type") == type) ++n;
        return n;
    }
};

std::string command(const std::string& cmd, const std::string& args) {
    return "{\"cmd\":\"" + cmd + "\",\"ref\":7,\"args\":" + args + "}";
}

TEST(Server, Pipeline) {
    Tividy engine("Acme");
    DemoSeed::company(engine);
    DemoSeed::templates(engine);
    CommandPipeline pipeline(engine);
    Inbox inbox;
    ClientSession session(1, engine, inbox.sink());

    Json r = pipeline.handle(session, "nonsense");
    EXPECT_TRUE(!r.get("ok").asBool(true) && r.str("code") == "bad_request") << "Pipeline: invalid JSON is a bad request";
    r = pipeline.handle(session, command("explode", "{}"));
    EXPECT_EQ(r.str("code"), "bad_request") << "Pipeline: unknown commands are refused";
    r = pipeline.handle(session, command("begin", "{\"project\":\"p\",\"item\":\"i\"}"));
    EXPECT_EQ(r.str("code"), "forbidden") << "Pipeline: nothing but login works before choosing a person";
    EXPECT_EQ(r.get("ref").asNumber(), 7) << "Pipeline: replies carry the request ref";
    r = pipeline.handle(session, command("login", "{\"as\":\"Nobody\"}"));
    EXPECT_EQ(r.str("code"), "invalid") << "Pipeline: logging in as an unknown person fails";
    r = pipeline.handle(session, command("login", "{\"as\":\"Bob\"}"));
    EXPECT_TRUE(r.get("ok").asBool() && session.participant() == "Bob") << "Pipeline: login binds the session to a person";
    r = pipeline.handle(session, command("startProject", "{\"template\":\"Feature Delivery\"}"));
    EXPECT_EQ(r.str("code"), "forbidden") << "Pipeline: the proxy stops a worker starting a project";
    r = pipeline.handle(session, command("claim", "{\"project\":42,\"item\":\"x\"}"));
    EXPECT_EQ(r.str("code"), "invalid") << "Pipeline: fields must have the right type";
    r = pipeline.handle(session, command("claim", "{\"project\":\"" + std::string(300, 'a') + "\",\"item\":\"x\"}"));
    EXPECT_EQ(r.str("code"), "invalid") << "Pipeline: over-long fields are refused";
    r = pipeline.handle(session, std::string(CommandPipeline::MAX_MESSAGE + 1, ' '));
    EXPECT_EQ(r.str("code"), "bad_request") << "Pipeline: oversized messages are refused";
    r = pipeline.handle(session, command("logout", "{}"));
    EXPECT_TRUE(r.get("ok").asBool() && !session.loggedIn()) << "Pipeline: logout unbinds the session";
    EXPECT_NE(inbox.last("hello"), nullptr) << "Pipeline: logout sends the people list again";
}

TEST(Server, Hub) {
    Tividy engine("Acme");
    DemoSeed::company(engine);
    DemoSeed::templates(engine);
    SessionHub hub(engine);
    Inbox managerBox;
    Inbox workerBox;
    int manager = hub.open(managerBox.sink());
    int worker = hub.open(workerBox.sink());
    EXPECT_EQ(hub.count(), 2u) << "Hub: two clients are connected";
    EXPECT_TRUE(managerBox.last("hello") != nullptr && managerBox.last("hello")->get("people").size() == 11) << "Hub: a new client gets the people grid";

    hub.receive(manager, command("login", "{\"as\":\"Maya\"}"));
    hub.receive(worker, command("login", "{\"as\":\"Tom\"}"));
    EXPECT_TRUE(managerBox.last("state") != nullptr && managerBox.last("state")->get("me").str("name") == "Maya") << "Hub: login sends a view for that person";
    EXPECT_EQ(engine.bus().observerCount(), 2u) << "Hub: each logged-in session observes the engine";

    hub.receive(manager, command("startProject", "{\"template\":\"Feature Delivery\",\"title\":\"Dark mode\",\"policy\":\"BusyTeam\"}"));
    const Json* tomState = workerBox.last("state");
    EXPECT_TRUE(tomState != nullptr && tomState->get("projects").size() == 1) << "Hub: the worker's view updates when the manager acts";

    const Json& spec = tomState->get("projects").at(0).get("items").at(0);
    bool canClaim = false;
    for (std::size_t i = 0; i < spec.get("actions").size(); ++i) {
        if (spec.get("actions").at(i).asString() == "claim") canClaim = true;
    }
    EXPECT_TRUE(spec.str("id") == "spec" && canClaim) << "Hub: the view tells Tom he can take the spec";

    hub.receive(worker, command("claim", "{\"project\":\"instance-1\",\"item\":\"spec\"}"));
    bool mine = false;
    for (std::size_t i = 0; i < workerBox.messages.size(); ++i) {
        const Json& m = workerBox.messages[i];
        if (m.str("type") == "event" && m.str("event") == "ItemAssigned" && m.get("mine").asBool()) mine = true;
    }
    EXPECT_TRUE(mine) << "Hub: Tom is told the assignment is his";
    hub.receive(worker, command("begin", "{\"project\":\"instance-1\",\"item\":\"spec\"}"));
    hub.receive(worker, command("done", "{\"project\":\"instance-1\",\"item\":\"spec\"}"));
    EXPECT_TRUE(managerBox.last("state")->get("approvals").size() == 1) << "Hub: Maya's approvals list gains the spec";

    bool routed = false;
    for (std::size_t i = 0; i < managerBox.messages.size(); ++i) {
        const Json& m = managerBox.messages[i];
        if (m.str("event") == "ApprovalRouted" && m.get("mine").asBool()) routed = true;
    }
    EXPECT_TRUE(routed) << "Hub: Maya is told the approval is hers";

    hub.receive(worker, command("approve", "{\"project\":\"instance-1\",\"item\":\"spec\"}"));
    EXPECT_TRUE(workerBox.last("result")->str("code") == "forbidden") << "Hub: Tom cannot approve his own work";
    hub.receive(manager, command("approve", "{\"project\":\"instance-1\",\"item\":\"spec\"}"));
    EXPECT_TRUE(managerBox.last("result")->get("ok").asBool()) << "Hub: Maya approves it";
    EXPECT_TRUE(managerBox.last("state")->get("approvals").size() == 0) << "Hub: and it leaves her approvals list";

    int before = workerBox.count("state");
    hub.close(worker);
    EXPECT_TRUE(hub.count() == 1 && engine.bus().observerCount() == 1) << "Hub: closing a client detaches its observer";
    hub.receive(manager, command("undo", "{}"));
    EXPECT_EQ(workerBox.count("state"), before) << "Hub: closed clients get nothing more";
    hub.receive(99, command("undo", "{}"));
    EXPECT_EQ(hub.count(), 1u) << "Hub: messages for unknown sessions are ignored";
}

TEST(Server, NewCommands) {
    Tividy engine("Acme");
    DemoSeed::company(engine);
    DemoSeed::templates(engine);
    ActivityLog log(engine);
    engine.bus().attach(&log);
    int saves = 0;
    SessionHub hub(engine, &log, [&saves]() { ++saves; });
    Inbox mayaBox;
    Inbox tomBox;
    int maya = hub.open(mayaBox.sink());
    int tom = hub.open(tomBox.sink());
    hub.receive(maya, command("login", "{\"as\":\"Maya\"}"));
    hub.receive(tom, command("login", "{\"as\":\"Tom\"}"));
    EXPECT_EQ(saves, 0) << "Autosave: logging in changes nothing, so nothing is saved";

    hub.receive(maya, command("startProject", "{\"template\":\"Feature Delivery\",\"policy\":\"BusyTeam\"}"));
    EXPECT_EQ(saves, 1) << "Autosave: a change triggers one save";
    hub.receive(maya, command("addTask", "{\"project\":\"instance-1\",\"name\":\"Hotfix\",\"priority\":\"Urgent\",\"approvers\":[\"TechLead\",\"Manager\"]}"));
    EXPECT_TRUE(mayaBox.last("result")->get("ok").asBool()) << "Pipeline: addTask takes a priority and several approvers";
    hub.receive(maya, command("addTask", "{\"project\":\"instance-1\",\"name\":\"Bad\",\"priority\":\"Whenever\"}"));
    EXPECT_TRUE(mayaBox.last("result")->str("code") == "invalid") << "Pipeline: a bad priority is refused";
    hub.receive(maya, command("addTask", "{\"project\":\"instance-1\",\"name\":\"Bad\",\"approvers\":\"Manager\"}"));
    EXPECT_TRUE(mayaBox.last("result")->str("code") == "invalid") << "Pipeline: approvers must be a list";

    hub.receive(tom, command("claim", "{\"project\":\"instance-1\",\"item\":\"spec\"}"));
    hub.receive(tom, command("begin", "{\"project\":\"instance-1\",\"item\":\"spec\"}"));
    hub.receive(tom, command("done", "{\"project\":\"instance-1\",\"item\":\"spec\"}"));
    hub.receive(tom, command("withdraw", "{\"project\":\"instance-1\",\"item\":\"spec\"}"));
    EXPECT_TRUE(tomBox.last("result")->get("ok").asBool()) << "Pipeline: the assignee can take work back";
    hub.receive(tom, command("done", "{\"project\":\"instance-1\",\"item\":\"spec\"}"));
    hub.receive(maya, command("addSubtask", "{\"project\":\"instance-1\",\"item\":\"spec\",\"name\":\"More detail\"}"));
    const Json* result = mayaBox.last("result");
    EXPECT_TRUE(result->get("ok").asBool() && !result->get("data").str("item").empty()) << "Pipeline: addSubtask returns the new id";
    bool told = false;
    for (std::size_t i = 0; i < tomBox.messages.size(); ++i) {
        const Json& m = tomBox.messages[i];
        if (m.str("event") == "SubtaskAdded" && m.get("mine").asBool()) told = true;
    }
    EXPECT_TRUE(told) << "Observer: Tom is told a sub-task was added to his work";
    hub.receive(tom, command("setPriority", "{\"project\":\"instance-1\",\"item\":\"spec\",\"priority\":\"Low\"}"));
    EXPECT_TRUE(tomBox.last("result")->str("code") == "forbidden") << "Pipeline: workers cannot change priority";
    hub.receive(maya, command("setPriority", "{\"project\":\"instance-1\",\"item\":\"spec\",\"priority\":\"Low\"}"));
    EXPECT_TRUE(mayaBox.last("result")->get("ok").asBool()) << "Pipeline: managers can";

    hub.receive(maya, command("undo", "{}"));
    const Json* state = mayaBox.last("state");
    EXPECT_TRUE(state->get("history").get("redo").size() == 1) << "View: the redo list shows what can be redone";
    hub.receive(maya, command("redo", "{}"));
    EXPECT_TRUE(mayaBox.last("result")->get("ok").asBool()) << "Pipeline: redo works";
    EXPECT_TRUE(mayaBox.last("state")->get("activity").size() > 5) << "View: managers see the activity log";
    hub.receive(maya, command("signOff", "{\"project\":\"instance-1\"}"));
    EXPECT_TRUE(mayaBox.last("result")->str("code") == "forbidden") << "Pipeline: a manager cannot sign off when there is an owner";
    hub.receive(maya, command("flagIssue", "{\"project\":\"instance-1\",\"name\":\"x\",\"units\":[\"backend\"]}"));
    EXPECT_TRUE(mayaBox.last("result")->str("code") == "forbidden") << "Pipeline: or flag issues";
    engine.bus().detach(&log);
}

}


TEST(Server, RemovedPersonIsSignedOut) {
    Tividy engine("Acme");
    DemoSeed::company(engine);
    DemoSeed::templates(engine);
    engine.clearHistory();
    SessionHub hub(engine);
    Inbox mayaBox;
    Inbox zedBox;
    int maya = hub.open(mayaBox.sink());
    int zed = hub.open(zedBox.sink());
    hub.receive(maya, command("login", "{\"as\":\"Maya\"}"));
    hub.receive(maya, command("addPerson", "{\"name\":\"Zed\",\"role\":\"Developer\",\"unit\":\"backend\"}"));
    hub.receive(zed, command("login", "{\"as\":\"Zed\"}"));
    EXPECT_TRUE(zedBox.last("state") != nullptr) << "Hub: Zed can log in once he is added";
    int hellos = zedBox.count("hello");
    hub.receive(maya, command("undo", "{}"));
    EXPECT_EQ(engine.organisation().find("Zed"), nullptr) << "Hub: undo takes Zed out again";
    EXPECT_EQ(zedBox.count("hello"), hellos + 1) << "Hub: Zed's tab goes back to the people grid";
    hub.receive(zed, command("claim", "{\"project\":\"instance-1\",\"item\":\"spec\"}"));
    EXPECT_FALSE(zedBox.last("result")->get("ok").asBool()) << "Hub: and Zed can do nothing more";
    EXPECT_EQ(engine.bus().observerCount(), 1u) << "Hub: his session stopped observing the engine";
}
