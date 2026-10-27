#include "server/SessionHub.h"
#include "server/ViewBuilder.h"
#include "server/ActivityLog.h"
#include <iostream>
#include "patterns/structural/Tividy.h"

SessionHub::SessionHub(Tividy& engine, ActivityLog* log, std::function<void()> onChange)
    : engine_(engine), pipeline_(engine), nextId_(1), log_(log), onChange_(onChange),
      seenVersion_(log != nullptr ? log->version() : 0) {}

SessionHub::~SessionHub() {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.clear();
}

int SessionHub::open(const MessageSink& sink) {
    std::lock_guard<std::mutex> lock(mutex_);
    int id = nextId_++;
    std::unique_ptr<ClientSession> session(new ClientSession(id, engine_, sink));
    session->send(ViewBuilder::hello(engine_));
    sessions_[id] = std::move(session);
    return id;
}

void SessionHub::receive(int sessionId, const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<int, std::unique_ptr<ClientSession> >::iterator it = sessions_.find(sessionId);
    if (it == sessions_.end()) return;
    ClientSession& session = *it->second;
    Json reply = pipeline_.handle(session, text);
    session.send(reply);
    if (log_ != nullptr && log_->version() != seenVersion_) {
        seenVersion_ = log_->version();
        if (onChange_) {
            try {
                onChange_();
            } catch (const std::exception& e) {
                std::cerr << "Autosave failed: " << e.what() << "\n";
            }
        }
    }
    signOutRemoved();
    flush();
}

void SessionHub::signOutRemoved() {
    for (std::map<int, std::unique_ptr<ClientSession> >::iterator it = sessions_.begin();
         it != sessions_.end(); ++it) {
        ClientSession& session = *it->second;
        if (session.loggedIn() && engine_.organisation().find(session.participant()) == nullptr) {
            session.logout();
            session.send(ViewBuilder::hello(engine_));
        }
    }
}

void SessionHub::flush() {
    for (std::map<int, std::unique_ptr<ClientSession> >::iterator it = sessions_.begin();
         it != sessions_.end(); ++it) {
        ClientSession& session = *it->second;
        if (session.takeDirty() && session.loggedIn()) {
            session.send(ViewBuilder::state(engine_, session.proxy(), log_));
        }
    }
}

void SessionHub::close(int sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(sessionId);
}

std::size_t SessionHub::count() {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}
