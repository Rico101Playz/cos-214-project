#ifndef COMMANDPIPELINE_H
#define COMMANDPIPELINE_H

#include <map>
#include <string>
#include "server/Json.h"

class ClientSession;
class Tividy;

class CommandPipeline {
public:
    typedef Json (*Handler)(Tividy& engine, ClientSession& session, const Json& args);

private:
    Tividy& engine_;
    std::map<std::string, Handler> handlers_;

public:
    static const std::size_t MAX_MESSAGE = 16384;

    explicit CommandPipeline(Tividy& engine);

    Json handle(ClientSession& session, const std::string& raw);
    bool knows(const std::string& command) const;
};

#endif
