#ifndef TUI_CONTROLLER_H
#define TUI_CONTROLLER_H

#include <string>
#include <utility>
#include <vector>

#include "patterns/structural/Tividy.h"
#include "patterns/execution/WorkflowEvent.h"
#include "patterns/execution/WorkflowObserver.h"

// The Controller is everything the TUI needs from the engine, with no FTXUI
// and no std::cout. Every action returns a Result instead of printing, and
// every query returns plain data the view can render.
namespace tui {

struct Result {
    bool ok;
    std::string message;
};

// One line of the flattened work tree (stage or item).
struct Row {
    std::string id;
    int depth = 0;
    bool isItem = false;
    std::string status;
    std::string assignee;
    std::string reason;
    std::string role;
    bool ready = false;
};

struct Person {
    std::string name;
    std::string role;
    int load = 0;
};

// Which actions make sense for an item status. UI hint only: the engine is
// still the authority and throws std::logic_error on an illegal action.
struct Allowed {
    bool assign = false;
    bool begin = false;
    bool done = false;
    bool approve = false;
    bool reject = false;
};
Allowed allowedFor(const std::string& status);

// OBSERVER: collects every engine event as a text line for the activity log.
class LogObserver : public WorkflowObserver {
public:
    std::vector<std::string> lines;
    void onEvent(const WorkflowEvent& e) override;
};

class Controller {
private:
    LogObserver log_;       // declared before engine_ so it outlives it
    Tividy engine_;
    std::vector<std::pair<std::string, std::string> > people_;  // name, role
    std::vector<std::string> instances_;
    std::string current_;

public:
    Controller();
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    // ---- queries ----
    const std::string& organisationName();
    bool hasInstance() const { return !current_.empty(); }
    const std::string& currentInstance() const { return current_; }
    std::string instanceSummary();              // "instance-1 (title)  state: Running"
    double progress();                          // 0.0 .. 1.0
    std::vector<Row> rows();                    // flattened tree of current instance
    std::vector<Person> team();
    const std::vector<std::string>& log() const { return log_.lines; }

    // ---- actions ----
    Result newInstance();                       // loads "delivery" on first use
    void nextInstance();                        // cycle through started instances
    Result assign(const std::string& item, const std::string& person);
    Result begin(const std::string& item);
    Result done(const std::string& item);
    Result approve(const std::string& item);
    Result reject(const std::string& item, const std::string& reason);
    Result cancel(const std::string& reason);
    Result undo();
};

}  // namespace tui

#endif  // TUI_CONTROLLER_H
