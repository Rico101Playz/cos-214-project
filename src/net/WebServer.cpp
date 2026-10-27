#include "net/WebServer.h"
#include "server/SessionHub.h"
#include "civetweb.h"
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <sstream>

namespace {

struct Outbox {
    mg_connection* conn;
    std::mutex lock;
    std::deque<std::string> queue;
    bool open;
    std::mutex writing;

    explicit Outbox(mg_connection* c) : conn(c), open(true) {}
};

struct Link {
    int session;
    std::string partial;
    std::shared_ptr<Outbox> outbox;
};

std::vector<std::shared_ptr<Outbox> >& touched() {
    static thread_local std::vector<std::shared_ptr<Outbox> > list;
    return list;
}

void drain(const std::shared_ptr<Outbox>& box) {
    for (;;) {
        std::unique_lock<std::mutex> writer(box->writing, std::try_to_lock);
        if (!writer.owns_lock()) return;
        for (;;) {
            std::string message;
            {
                std::lock_guard<std::mutex> guard(box->lock);
                if (!box->open || box->queue.empty()) break;
                message.swap(box->queue.front());
                box->queue.pop_front();
            }
            mg_websocket_write(box->conn, MG_WEBSOCKET_OPCODE_TEXT, message.data(), message.size());
        }
        writer.unlock();
        std::lock_guard<std::mutex> guard(box->lock);
        if (!box->open || box->queue.empty()) return;
    }
}

void drainTouched() {
    std::vector<std::shared_ptr<Outbox> > boxes;
    boxes.swap(touched());
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        bool seen = false;
        for (std::size_t j = 0; j < i && !seen; ++j) seen = boxes[j] == boxes[i];
        if (!seen) drain(boxes[i]);
    }
}

bool localOrigin(const char* origin) {
    if (origin == nullptr) return true;
    std::string o(origin);
    const char* allowed[] = { "http://localhost", "http://127.0.0.1", "http://[::1]" };
    for (std::size_t i = 0; i < 3; ++i) {
        std::string prefix(allowed[i]);
        if (o == prefix || o.compare(0, prefix.size() + 1, prefix + ":") == 0) return true;
    }
    return false;
}

}

WebServer::WebServer(SessionHub& hub, const std::string& webRoot, const std::vector<int>& ports)
    : hub_(hub), webRoot_(webRoot), ports_(ports), context_(nullptr) {}

WebServer::~WebServer() { stop(); }

bool WebServer::start(std::string& error) {
    mg_init_library(MG_FEATURES_WEBSOCKET);

    std::ostringstream listen;
    for (std::size_t i = 0; i < ports_.size(); ++i) {
        if (i) listen << ",";
        listen << "127.0.0.1:" << ports_[i];
    }
    std::string ports = listen.str();

    const char* options[] = {
        "listening_ports", ports.c_str(),
        "document_root", webRoot_.c_str(),
        "enable_directory_listing", "no",
        "num_threads", "64",
        "websocket_timeout_ms", "3600000",
        "enable_websocket_ping_pong", "yes",
        nullptr
    };

    mg_callbacks callbacks;
    std::memset(&callbacks, 0, sizeof callbacks);
    context_ = mg_start(&callbacks, this, options);
    if (context_ == nullptr) {
        error = "Could not listen on " + ports + " (is another server already running?)";
        mg_exit_library();
        return false;
    }
    mg_set_websocket_handler(context_, "/ws", &WebServer::onConnect, &WebServer::onReady,
                             &WebServer::onData, &WebServer::onClose, this);
    return true;
}

void WebServer::stop() {
    if (context_ != nullptr) {
        mg_stop(context_);
        context_ = nullptr;
        mg_exit_library();
    }
}

int WebServer::onConnect(const mg_connection* conn, void*) {
    const char* origin = mg_get_header(conn, "Origin");
    return localOrigin(origin) ? 0 : 1;
}

void WebServer::onReady(mg_connection* conn, void* self) {
    WebServer* server = static_cast<WebServer*>(self);
    Link* link = new Link();
    link->session = 0;
    link->outbox = std::make_shared<Outbox>(conn);
    mg_set_user_connection_data(conn, link);
    std::shared_ptr<Outbox> box = link->outbox;
    link->session = server->hub_.open([box](const std::string& message) {
        {
            std::lock_guard<std::mutex> guard(box->lock);
            if (!box->open) return;
            box->queue.push_back(message);
        }
        touched().push_back(box);
    });
    drainTouched();
}

int WebServer::onData(mg_connection* conn, int bits, char* data, size_t length, void* self) {
    WebServer* server = static_cast<WebServer*>(self);
    Link* link = static_cast<Link*>(mg_get_user_connection_data(conn));
    if (link == nullptr) return 0;

    int opcode = bits & 0x0F;
    bool final = (bits & 0x80) != 0;
    if (opcode == MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE) return 0;
    if (opcode == MG_WEBSOCKET_OPCODE_PING || opcode == MG_WEBSOCKET_OPCODE_PONG) return 1;
    if (opcode == MG_WEBSOCKET_OPCODE_BINARY) return 0;

    if (link->partial.size() + length > CommandPipeline::MAX_MESSAGE) return 0;
    link->partial.append(data, length);
    if (!final) return 1;

    std::string message;
    message.swap(link->partial);
    server->hub_.receive(link->session, message);
    drainTouched();
    return 1;
}

void WebServer::onClose(const mg_connection* conn, void* self) {
    WebServer* server = static_cast<WebServer*>(self);
    Link* link = static_cast<Link*>(mg_get_user_connection_data(conn));
    if (link == nullptr) return;
    {
        std::lock_guard<std::mutex> guard(link->outbox->lock);
        link->outbox->open = false;
        link->outbox->queue.clear();
    }
    { std::lock_guard<std::mutex> wait(link->outbox->writing); }
    server->hub_.close(link->session);
    delete link;
}
