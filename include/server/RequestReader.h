#ifndef REQUEST_READER_H
#define REQUEST_READER_H

#include <map>
#include <string>
#include "mediator/Request.h"
#include "server/Json.h"

class ClientSession;
class TividyMediator;

// The only place that reads the browser's JSON. Each message
// {cmd, args, ref} becomes one Request, which is sent through the session's
// SecuredMediator; the Response becomes the reply {type: "result", ok, data |
// code, error}. Choosing who you are (login) and leaving (logout) belong to the
// session, not the engine, so they are handled here directly.
class RequestReader {
public:
    // Builds the Request for one command from its arguments; throws
    // std::invalid_argument for a missing or malformed field.
    typedef Request (*Reader)(const Json& args, const std::string& actor);
    typedef Json (*SessionHandler)(TividyMediator& engine, ClientSession& session, const Json& args);

private:
    TividyMediator& engine_;
    std::map<std::string, Reader> readers_;
    std::map<std::string, SessionHandler> sessionHandlers_;

public:
    static const std::size_t MAX_MESSAGE = 16384;

    explicit RequestReader(TividyMediator& engine);

    Json handle(ClientSession& session, const std::string& raw);
    bool knows(const std::string& command) const;
};

#endif // REQUEST_READER_H
