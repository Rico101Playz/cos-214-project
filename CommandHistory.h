#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include <vector>
#include <string>
#include <memory> 
#include <cstddef>

class WorkCommand;

class CommandHistory {
private:
    std::vector<std::unique_ptr<WorkCommand>> executed_;
    std::vector<std::string> log_;

public:
    CommandHistory() = default;
    ~CommandHistory() = default;

    size_t log(std::string line);

    void relabel(size_t index, std::string line);

    void push(std::unique_ptr<WorkCommand> command);

    std::unique_ptr<WorkCommand> pop();

    void truncate(size_t commandCount, size_t logCount);

    std::vector<std::string> entries();

    size_t logSize();

    size_t size();

    bool canUndo();
};

#endif // COMMANDHISTORY_H