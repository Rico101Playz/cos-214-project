#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include <vector>

struct mg_context;
struct mg_connection;
class SessionHub;

class WebServer {
private:
    SessionHub& hub_;
    std::string webRoot_;
    std::vector<int> ports_;
    mg_context* context_;

    static int onConnect(const mg_connection* conn, void* self);
    static void onReady(mg_connection* conn, void* self);
    static int onData(mg_connection* conn, int bits, char* data, size_t length, void* self);
    static void onClose(const mg_connection* conn, void* self);

public:
    WebServer(SessionHub& hub, const std::string& webRoot, const std::vector<int>& ports);
    ~WebServer();

    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;

    bool start(std::string& error);
    void stop();
};

#endif
