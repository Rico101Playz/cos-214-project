#ifndef WORKCOMMAND_H
#define WORKCOMMAND_H

#include <string>

class WorkCommand {
private:
    int executedAt;

protected:
    virtual void perform() = 0;

public:
    virtual ~WorkCommand() = default;
    
    void execute();
    void undo();
    virtual std::string describe() const = 0;
};

#endif // WORKCOMMAND_H