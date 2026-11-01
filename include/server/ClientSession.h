#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <functional>
#include <memory>
#include <string>
#include "observer/WorkflowObserver.h"
#include "proxy/SecuredMediator.h"

class TividyMediator;
class Json;

typedef std::function<void(const std::string&)> MessageSink;

class ClientSession : public WorkflowObserver {
private:
    int id_;
    TividyMediator& engine_;
    MessageSink sink_;
    std::unique_ptr<SecuredMediator> proxy_;
    bool dirty_;

    bool concernsMe(const WorkflowEvent& event) const;

public:
    ClientSession(int id, TividyMediator& engine, MessageSink sink);
    ~ClientSession();

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;

    int id() const;
    bool loggedIn() const;
    std::string participant() const;

    void login(const std::string& participant);
    void logout();
    SecuredMediator& proxy();

    void onEvent(const WorkflowEvent& event) override;

    void send(const Json& message);
    void markDirty();
    bool takeDirty();
};

#endif
