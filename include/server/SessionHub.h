#ifndef SESSIONHUB_H
#define SESSIONHUB_H

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "server/ClientSession.h"
#include "server/CommandPipeline.h"

class Tividy;
class ActivityLog;

class SessionHub {
private:
    Tividy& engine_;
    CommandPipeline pipeline_;
    std::map<int, std::unique_ptr<ClientSession> > sessions_;
    int nextId_;
    std::mutex mutex_;
    ActivityLog* log_;
    std::function<void()> onChange_;
    unsigned long seenVersion_;

    void flush();
    void signOutRemoved();

public:
    explicit SessionHub(Tividy& engine, ActivityLog* log = nullptr,
                        std::function<void()> onChange = std::function<void()>());
    ~SessionHub();

    int open(const MessageSink& sink);
    void receive(int sessionId, const std::string& text);
    void close(int sessionId);
    std::size_t count();
};

#endif
