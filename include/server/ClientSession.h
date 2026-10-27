#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <functional>
#include <memory>
#include <string>
#include "patterns/execution/WorkflowObserver.h"
#include "patterns/structural/SecuredTividy.h"

class Tividy;
class Json;

typedef std::function<void(const std::string&)> MessageSink;

class ClientSession : public WorkflowObserver {
private:
    int id_;
    Tividy& engine_;
    MessageSink sink_;
    std::unique_ptr<SecuredTividy> proxy_;
    bool dirty_;

    bool concernsMe(const WorkflowEvent& event) const;

public:
    ClientSession(int id, Tividy& engine, MessageSink sink);
    ~ClientSession();

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;

    int id() const;
    bool loggedIn() const;
    std::string participant() const;

    void login(const std::string& participant);
    void logout();
    SecuredTividy& proxy();

    void onEvent(const WorkflowEvent& event) override;

    void send(const Json& message);
    void markDirty();
    bool takeDirty();
};

#endif
