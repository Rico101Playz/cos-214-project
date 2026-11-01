#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "net/WebServer.h"
#include "server/ActivityLog.h"
#include "server/ScenarioStore.h"
#include "server/SessionHub.h"
#include "patterns/structural/Tividy.h"

#ifndef TIVIDY_WEB_ROOT
#define TIVIDY_WEB_ROOT "web"
#endif

#ifndef TIVIDY_SCENARIOS
#define TIVIDY_SCENARIOS "scenarios"
#endif

namespace {

std::atomic<bool> running(true);

void onSignal(int) { running = false; }

std::vector<int> parsePorts(const std::string& text) {
    std::vector<int> ports;
    std::stringstream in(text);
    std::string part;
    while (std::getline(in, part, ',')) {
        int port = std::atoi(part.c_str());
        if (port <= 0 || port > 65535) {
            throw std::invalid_argument("Bad port: " + part);
        }
        ports.push_back(port);
    }
    if (ports.empty()) throw std::invalid_argument("No ports given");
    return ports;
}

}

int main(int argc, char** argv) {
    std::string webRoot = TIVIDY_WEB_ROOT;
    std::string scenarios = TIVIDY_SCENARIOS;
    std::string scenario;
    std::vector<int> ports;
    ports.push_back(8080);
    ports.push_back(8081);
    ports.push_back(8082);

    try {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--ports" && i + 1 < argc) {
                ports = parsePorts(argv[++i]);
            } else if (arg == "--web" && i + 1 < argc) {
                webRoot = argv[++i];
            } else if (arg == "--scenarios" && i + 1 < argc) {
                scenarios = argv[++i];
            } else if (arg == "--scenario" && i + 1 < argc) {
                scenario = argv[++i];
            } else {
                std::cout << "Usage: TIVIDY_Server [--ports 8080,8081,8082] [--web <folder>]\n"
                             "                     [--scenarios <folder>] [--scenario <file.txt>]\n";
                return arg == "--help" ? 0 : 1;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    if (scenario.empty()) {
        ScenarioStore::ensureDirectory(scenarios);
        scenario = ScenarioMenu::choose(scenarios, std::cin, std::cout);
        if (scenario.empty()) {
            std::cout << "No scenario chosen.\n";
            return 0;
        }
    }

    Tividy engine("Company");
    ActivityLog log(engine);
    engine.bus().attach(&log);
    try {
        ScenarioStore::load(scenario, engine, &log);
    } catch (const std::exception& e) {
        std::cerr << "Could not load " << scenario << ": " << e.what() << "\n";
        return 1;
    }
    log.add("Server started with " + scenario);
    ScenarioStore::save(scenario, engine, &log);

    SessionHub hub(engine, &log, [&engine, &log, scenario]() {
        ScenarioStore::save(scenario, engine, &log);
    });
    WebServer server(hub, webRoot, ports);
    std::string error;
    if (!server.start(error)) {
        std::cerr << error << "\n";
        return 1;
    }

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    std::cout << "\nLoaded " << engine.organisation().name() << " from " << scenario
              << ". Every change is saved back to that file.\n";
    std::cout << "TIVIDY server is running. Open one tab per person:\n";
    for (std::size_t i = 0; i < ports.size(); ++i) {
        std::cout << "  http://localhost:" << ports[i] << "\n";
    }
    std::cout << "Serving " << webRoot << ". Press Ctrl+C to stop.\n" << std::flush;

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    server.stop();
    engine.bus().detach(&log);
    ScenarioStore::save(scenario, engine, &log);
    std::cout << "Stopped. Saved " << scenario << "\n";
    return 0;
}
