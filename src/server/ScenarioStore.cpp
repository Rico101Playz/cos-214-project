#include "server/ScenarioStore.h"
#include "server/ActivityLog.h"
#include "server/DemoSeed.h"
#include "patterns/structural/Tividy.h"
#include "core/TextRecord.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <sstream>
#include <stdexcept>
#include <dirent.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

namespace {

std::string readFile(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open " + path);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

std::string join(const std::string& directory, const std::string& file) {
    if (directory.empty()) return file;
    char last = directory[directory.size() - 1];
    if (last == '/' || last == '\\') return directory + file;
    return directory + (directory.find('\\') != std::string::npos ? "\\" : "/") + file;
}

bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string trim(const std::string& text) {
    std::size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

bool readLine(std::istream& in, std::ostream& out, const std::string& prompt, std::string& answer) {
    out << prompt << std::flush;
    if (!std::getline(in, answer)) return false;
    answer = trim(answer);
    return true;
}

}

bool ScenarioStore::exists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0;
}

bool ScenarioStore::ensureDirectory(const std::string& directory) {
    if (exists(directory)) return true;
#ifdef _WIN32
    return _mkdir(directory.c_str()) == 0;
#else
    return mkdir(directory.c_str(), 0755) == 0;
#endif
}

std::string ScenarioStore::slug(const std::string& name) {
    std::string out;
    for (std::size_t i = 0; i < name.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(name[i]);
        if (std::isalnum(c)) out += static_cast<char>(std::tolower(c));
        else if (!out.empty() && out[out.size() - 1] != '-') out += '-';
    }
    while (!out.empty() && out[out.size() - 1] == '-') out.erase(out.size() - 1);
    return out.empty() ? "scenario" : out;
}

std::vector<ScenarioInfo> ScenarioStore::list(const std::string& directory) {
    std::vector<ScenarioInfo> found;
    DIR* dir = opendir(directory.c_str());
    if (dir == nullptr) return found;
    for (dirent* entry = readdir(dir); entry != nullptr; entry = readdir(dir)) {
        std::string file = entry->d_name;
        if (!endsWith(file, ".txt")) continue;
        ScenarioInfo info;
        info.file = file;
        info.path = join(directory, file);
        try {
            std::istringstream in(readFile(info.path));
            std::string line;
            while (std::getline(in, line)) {
                std::vector<std::string> f = TextRecord::split(line);
                if (f[0] == "tividy") info.readable = true;
                else if (f[0] == "company" && f.size() >= 2) info.company = f[1];
                else if (f[0] == "person") ++info.people;
                else if (f[0] == "instance") ++info.projects;
            }
        } catch (const std::exception&) {
            info.readable = false;
        }
        if (info.company.empty()) info.readable = false;
        found.push_back(info);
    }
    closedir(dir);
    std::sort(found.begin(), found.end(), [](const ScenarioInfo& a, const ScenarioInfo& b) { return a.file < b.file; });
    return found;
}

void ScenarioStore::load(const std::string& path, Tividy& engine, ActivityLog* log) {
    std::string text = readFile(path);
    engine.restore(text);
    std::vector<std::string> wanted;
    wanted.push_back("Feature Delivery");
    wanted.push_back("Blank board");
    bool missing = false;
    for (std::size_t i = 0; i < wanted.size(); ++i) missing = missing || !engine.hasDefinition(wanted[i]);
    if (missing) DemoSeed::templates(engine);
    engine.clearHistory();

    if (log != nullptr) {
        std::vector<std::string> entries;
        std::istringstream in(text);
        std::string line;
        while (std::getline(in, line)) {
            std::vector<std::string> f = TextRecord::split(line);
            if (f[0] == "log" && f.size() >= 2) entries.push_back(f[1]);
        }
        log->replace(entries);
    }
}

void ScenarioStore::save(const std::string& path, const Tividy& engine, const ActivityLog* log) {
    std::string text = "# TIVIDY scenario. Saved automatically after every change.\n";
    text += engine.snapshot();
    if (log != nullptr) {
        const std::vector<std::string>& entries = log->entries();
        for (std::size_t i = 0; i < entries.size(); ++i) {
            std::vector<std::string> fields;
            fields.push_back("log");
            fields.push_back(entries[i]);
            text += TextRecord::line(fields);
        }
    }
    std::string temp = path + ".tmp";
    {
        std::ofstream out(temp.c_str(), std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("Cannot write " + temp);
        out << text;
        if (!out) throw std::runtime_error("Cannot write " + temp);
    }
#ifdef _WIN32
    if (!MoveFileExA(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
#else
    if (std::rename(temp.c_str(), path.c_str()) != 0) {
#endif
        throw std::runtime_error("Cannot replace " + path);
    }
}

std::string ScenarioStore::createBlank(const std::string& directory, const std::string& scenarioName,
                                       const std::string& company, const std::string& owner) {
    if (scenarioName.empty() || company.empty() || owner.empty()) {
        throw std::invalid_argument("A scenario needs a name, a company and an owner");
    }
    if (!ensureDirectory(directory)) {
        throw std::runtime_error("Cannot create the folder " + directory);
    }
    std::string base = slug(scenarioName);
    std::string path = join(directory, base + ".txt");
    for (int n = 2; exists(path); ++n) {
        path = join(directory, base + "-" + std::to_string(n) + ".txt");
    }
    Tividy engine(company);
    engine.addPerson(owner, "Owner", "org");
    DemoSeed::templates(engine);
    save(path, engine, nullptr);
    return path;
}

std::string ScenarioMenu::choose(const std::string& directory, std::istream& in, std::ostream& out) {
    while (true) {
        std::vector<ScenarioInfo> scenarios = ScenarioStore::list(directory);
        std::vector<ScenarioInfo> usable;
        for (std::size_t i = 0; i < scenarios.size(); ++i) {
            if (scenarios[i].readable) usable.push_back(scenarios[i]);
        }
        out << "\nTIVIDY scenarios in " << directory << "\n";
        for (std::size_t i = 0; i < usable.size(); ++i) {
            out << "  " << (i + 1) << ". " << usable[i].company << "  (" << usable[i].file << ", "
                << usable[i].people << (usable[i].people == 1 ? " person, " : " people, ")
                << usable[i].projects << (usable[i].projects == 1 ? " project)\n" : " projects)\n");
        }
        std::size_t createOption = usable.size() + 1;
        out << "  " << createOption << ". Create a new scenario (blank company)\n";
        out << "  q. Quit\n";

        std::string answer;
        std::ostringstream prompt;
        prompt << "Choose 1-" << createOption << ": ";
        if (!readLine(in, out, prompt.str(), answer)) return "";
        if (answer == "q" || answer == "Q") return "";
        int choice = std::atoi(answer.c_str());
        if (choice >= 1 && static_cast<std::size_t>(choice) <= usable.size()) {
            return usable[choice - 1].path;
        }
        if (choice >= 1 && static_cast<std::size_t>(choice) == createOption) {
            std::string name;
            std::string company;
            std::string owner;
            if (!readLine(in, out, "Scenario name: ", name)) return "";
            if (!readLine(in, out, "Company name [" + name + "]: ", company)) return "";
            if (company.empty()) company = name;
            if (!readLine(in, out, "Owner's name (the CEO who signs projects off): ", owner)) return "";
            try {
                std::string path = ScenarioStore::createBlank(directory, name, company, owner);
                out << "Created " << path << "\n";
                return path;
            } catch (const std::exception& e) {
                out << "Could not create it: " << e.what() << "\n";
                continue;
            }
        }
        out << "Please type a number from 1 to " << createOption << ", or q.\n";
    }
}
