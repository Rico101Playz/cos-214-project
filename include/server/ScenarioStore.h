#ifndef SCENARIOSTORE_H
#define SCENARIOSTORE_H

#include <iosfwd>
#include <string>
#include <vector>

class Tividy;
class ActivityLog;

struct ScenarioInfo {
    std::string path;
    std::string file;
    std::string company;
    int people;
    int projects;
    bool readable;

    ScenarioInfo() : people(0), projects(0), readable(false) {}
};

class ScenarioStore {
public:
    static std::vector<ScenarioInfo> list(const std::string& directory);
    static void load(const std::string& path, Tividy& engine, ActivityLog* log);
    static void save(const std::string& path, const Tividy& engine, const ActivityLog* log);
    static std::string createBlank(const std::string& directory, const std::string& scenarioName,
                                   const std::string& company, const std::string& owner);
    static std::string slug(const std::string& name);
    static bool ensureDirectory(const std::string& directory);
    static bool exists(const std::string& path);
};

class ScenarioMenu {
public:
    static std::string choose(const std::string& directory, std::istream& in, std::ostream& out);
};

#endif
