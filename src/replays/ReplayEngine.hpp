#pragma once
#include <Geode/Geode.hpp>
#include <filesystem>
#include <string>
#include <vector>

namespace ripple {
using namespace geode::prelude;

struct ReplayAction {
    int frame;
    int button;
    bool down;
    bool player1;
};

enum class ReplayMode { Idle, Recording, Replaying };
enum class SaveResult { Saved, Exists, Failed };

class ReplayEngine {
public:
    static ReplayEngine& get();

    ReplayMode mode() const { return m_mode; }
    ReplayMode pending() const { return m_pending; }
    size_t actionCount() const { return m_actions.size(); }
    int frame() const { return m_frame; }
    std::string const& loadedName() const { return m_name; }
    std::string const& message() const { return m_message; }
    void setMessage(std::string m) { m_message = std::move(m); }

    // Armed modes start on the next restart from the very beginning of a level.
    void armRecord();
    void armReplay();
    void stop();

    static std::string sanitizeName(std::string name);
    std::string defaultName() const { return sanitizeName(m_levelName); }
    SaveResult save(std::string const& name, bool overwrite);
    bool load(std::filesystem::path const& path);
    std::vector<std::filesystem::path> listSaved();
    std::filesystem::path saveDir();

    // Called from the hooks in main.cpp
    void onReset(PlayLayer* pl);
    void onCheckpoint(int count);
    void onLevelEnd();
    bool allowButton(bool down, int button, bool player1);
    void beforeStep(GJBaseGameLayer* layer);
    void afterStep();

private:
    void releaseAll(GJBaseGameLayer* layer);

    ReplayMode m_mode = ReplayMode::Idle;
    ReplayMode m_pending = ReplayMode::Idle;
    std::vector<ReplayAction> m_actions;
    std::vector<int> m_cpFrames; // frame number at each practice checkpoint
    size_t m_index = 0;
    int m_frame = 0;
    int m_levelId = 0;
    std::string m_levelName;
    std::string m_name;
    std::string m_message;
    bool m_injecting = false;
};
}
