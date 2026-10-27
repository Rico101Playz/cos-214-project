#include "server/ClientSession.h"
#include "server/Json.h"
#include "server/EventText.h"
#include "patterns/structural/Tividy.h"
#include "patterns/structural/LeafWorkItem.h"

ClientSession::ClientSession(int id, Tividy& engine, MessageSink sink)
    : id_(id), engine_(engine), sink_(sink), dirty_(false) {}

ClientSession::~ClientSession() {
    engine_.bus().detach(this);
}

int ClientSession::id() const { return id_; }

bool ClientSession::loggedIn() const { return proxy_ != nullptr; }

std::string ClientSession::participant() const { return proxy_ ? proxy_->actor() : std::string(); }

void ClientSession::login(const std::string& participant) {
    std::unique_ptr<SecuredTividy> proxy(new SecuredTividy(engine_, participant));
    proxy_ = std::move(proxy);
    engine_.bus().attach(this);
    dirty_ = true;
}

void ClientSession::logout() {
    engine_.bus().detach(this);
    proxy_.reset();
    dirty_ = false;
}

SecuredTividy& ClientSession::proxy() {
    if (!proxy_) {
        throw AccessDenied("Choose who you are first");
    }
    return *proxy_;
}

bool ClientSession::concernsMe(const WorkflowEvent& event) const {
    if (!proxy_) return false;
    const std::string me = proxy_->actor();
    const Company& org = engine_.organisation();
    const Participant* person = org.find(me);
    if (person == nullptr) return false;

    const std::string& t = event.type;
    if (t == WorkflowEvent::ITEM_ASSIGNED) return event.detail == me;
    if (t == WorkflowEvent::APPROVAL_ROUTED) {
        return event.detail != "Auto" && engine_.canApprove(me, event.instanceId, event.itemId);
    }
    if (t == WorkflowEvent::SIGNOFF_REQUESTED) return engine_.canSignOff(me, event.instanceId);
    if (t == WorkflowEvent::ITEM_ESCALATED) return org.isManager(me);
    if (t == WorkflowEvent::ITEM_REJECTED || t == WorkflowEvent::ITEM_APPROVED ||
        t == WorkflowEvent::ITEM_SIGNED || t == WorkflowEvent::PRIORITY_CHANGED) {
        try {
            const WorkItem* item = engine_.instance(event.instanceId).item(event.itemId);
            return item != nullptr && item->assignee() == me && event.detail != me;
        } catch (const std::exception&) {
            return false;
        }
    }
    if (t == WorkflowEvent::SUBTASK_ADDED) {
        try {
            const WorkItem* parent = engine_.instance(event.instanceId).item(event.detail);
            return parent != nullptr && parent->assignee() == me;
        } catch (const std::exception&) {
            return false;
        }
    }
    return false;
}

void ClientSession::onEvent(const WorkflowEvent& event) {
    if (!proxy_) return;

    Json message = Json::object();
    message.set("type", "event");
    message.set("event", event.type);
    message.set("project", event.instanceId);
    message.set("item", event.itemId);
    message.set("text", EventText::describe(engine_, event));
    message.set("mine", concernsMe(event));
    send(message);
    dirty_ = true;
}

void ClientSession::send(const Json& message) {
    if (sink_) sink_(message.dump());
}

void ClientSession::markDirty() { dirty_ = true; }

bool ClientSession::takeDirty() {
    bool was = dirty_;
    dirty_ = false;
    return was;
}
