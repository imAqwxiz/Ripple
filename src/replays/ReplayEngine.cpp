#include "ReplayEngine.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>

namespace ripple {

static constexpr char const* kExt = ".rpl";

ReplayEngine& ReplayEngine::get() {
    static ReplayEngine instance;
    return instance;
}

std::filesystem::path ReplayEngine::saveDir() {
    auto dir = Mod::get()->getSaveDir() / "macros";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

void ReplayEngine::armRecord() {
    m_mode = ReplayMode::Idle;
    m_pending = ReplayMode::Recording;
    m_actions.clear();
    m_name.clear();
    m_message.clear();
}

void ReplayEngine::armReplay() {
    if (m_actions.empty()) {
        m_message = "No macro loaded";
        return;
    }
    m_mode = ReplayMode::Idle;
    m_pending = ReplayMode::Replaying;
    m_message.clear();
}

void ReplayEngine::stop() {
    m_mode = ReplayMode::Idle;
    m_pending = ReplayMode::Idle;
    m_message = "Stopped";
}

std::string ReplayEngine::sanitizeName(std::string name) {
    std::string out;
    for (char c : name) {
        auto u = static_cast<unsigned char>(c);
        if (u < 32 || std::string("\\/:*?\"<>|").find(c) != std::string::npos) continue;
        out += c;
    }
    // trim spaces and dots at both ends
    auto bad = [](char c) { return c == ' ' || c == '.'; };
    while (!out.empty() && bad(out.back())) out.pop_back();
    size_t start = 0;
    while (start < out.size() && bad(out[start])) ++start;
    out = out.substr(start);
    if (out.size() > 48) out.resize(48);
    while (!out.empty() && bad(out.back())) out.pop_back();
    if (out.empty()) out = "macro";
    return out;
}

SaveResult ReplayEngine::save(std::string const& rawName, bool overwrite) {
    if (m_actions.empty()) {
        m_message = "Nothing to save";
        return SaveResult::Failed;
    }
    auto name = sanitizeName(rawName);
    auto file = saveDir() / (name + kExt);

    std::error_code ec;
    if (!overwrite && std::filesystem::exists(file, ec)) return SaveResult::Exists;

    std::ofstream out(file);
    if (!out) {
        m_message = "Could not write file";
        return SaveResult::Failed;
    }
    out << "RIPPLE 1\n";
    out << "level " << m_levelId << "\n";
    out << "name " << (m_levelName.empty() ? std::string("unknown") : m_levelName) << "\n";
    out << "count " << m_actions.size() << "\n";
    for (auto const& a : m_actions) {
        out << a.frame << ' ' << a.button << ' ' << (a.down ? 1 : 0) << ' ' << (a.player1 ? 1 : 0) << "\n";
    }
    out.close();

    m_name = name;
    m_message = "Saved";
    return SaveResult::Saved;
}

bool ReplayEngine::load(std::filesystem::path const& path) {
    auto fail = [this](char const* msg) {
        m_message = msg;
        return false;
    };

    std::ifstream in(path);
    if (!in) return fail("Could not open file");

    std::string magic, key, name;
    int version = 0, level = 0;
    size_t count = 0;

    if (!(in >> magic >> version) || magic != "RIPPLE") return fail("Not a Ripple macro");
    if (!(in >> key >> level) || key != "level") return fail("Corrupt macro");
    if (!(in >> key) || key != "name") return fail("Corrupt macro");
    std::getline(in >> std::ws, name);
    if (!(in >> key >> count) || key != "count") return fail("Corrupt macro");
    if (count > 5000000) return fail("Corrupt macro");

    std::vector<ReplayAction> actions;
    actions.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        int f, b, d, p;
        if (!(in >> f >> b >> d >> p)) return fail("Corrupt macro");
        actions.push_back({f, b, d != 0, p != 0});
    }

    m_actions = std::move(actions);
    m_levelId = level;
    m_levelName = name;
    m_name = path.stem().string();
    m_mode = ReplayMode::Idle;
    m_pending = ReplayMode::Idle;
    m_message = "Loaded";
    return true;
}

std::vector<std::filesystem::path> ReplayEngine::listSaved() {
    std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> found;
    std::error_code ec;
    for (auto const& entry : std::filesystem::directory_iterator(saveDir(), ec)) {
        std::error_code e2;
        if (!entry.is_regular_file(e2)) continue;
        auto ext = entry.path().extension().string();
        if (ext != ".rpl" && ext != ".ripple") continue; // .ripple = older test saves
        found.emplace_back(entry.last_write_time(e2), entry.path());
    }
    std::sort(found.begin(), found.end(), [](auto const& a, auto const& b) { return a.first > b.first; });

    std::vector<std::filesystem::path> result;
    for (auto& f : found) result.push_back(std::move(f.second));
    return result;
}

void ReplayEngine::releaseAll(GJBaseGameLayer* layer) {
    m_injecting = true;
    layer->handleButton(false, 1, true);
    layer->handleButton(false, 1, false);
    m_injecting = false;
}

void ReplayEngine::onCheckpoint(int count) {
    if (count <= 0) {
        m_cpFrames.clear();
        return;
    }
    while (static_cast<int>(m_cpFrames.size()) >= count) m_cpFrames.pop_back();
    while (static_cast<int>(m_cpFrames.size()) < count - 1) m_cpFrames.push_back(0);
    m_cpFrames.push_back(m_frame);
}

void ReplayEngine::onReset(PlayLayer* pl) {
    int cps = (pl->m_isPracticeMode && pl->m_checkpointArray)
                  ? static_cast<int>(pl->m_checkpointArray->count())
                  : 0;

    // Armed modes only start from the very beginning of the level (no checkpoint respawn).
    if (m_pending != ReplayMode::Idle && cps == 0) {
        int id = pl->m_level->m_levelID.value();
        if (m_pending == ReplayMode::Replaying && m_levelId != 0 && m_levelId != id) {
            m_message = "This macro is for another level";
            m_pending = ReplayMode::Idle;
            m_mode = ReplayMode::Idle;
        } else {
            m_mode = m_pending;
            m_pending = ReplayMode::Idle;
            if (m_mode == ReplayMode::Recording) {
                m_levelId = id;
                m_levelName = std::string(pl->m_level->m_levelName);
                m_actions.clear();
            }
            m_message.clear();
            m_cpFrames.clear();
        }
    }

    // Practice mode: resume the frame counter from the checkpoint we respawned at.
    while (static_cast<int>(m_cpFrames.size()) > cps) m_cpFrames.pop_back();
    int start = (cps > 0 && !m_cpFrames.empty()) ? m_cpFrames.back() : 0;
    m_frame = start;

    auto it = std::lower_bound(m_actions.begin(), m_actions.end(), start,
                               [](ReplayAction const& a, int f) { return a.frame < f; });
    if (m_mode == ReplayMode::Recording) m_actions.erase(it, m_actions.end());
    m_index = static_cast<size_t>(std::distance(m_actions.begin(), it));

    if (m_mode == ReplayMode::Replaying) releaseAll(pl);
}

void ReplayEngine::onLevelEnd() {
    if (m_mode == ReplayMode::Recording) m_message = "Recording finished - press Save";
    else if (m_mode == ReplayMode::Replaying) m_message = "Replay finished";
    m_mode = ReplayMode::Idle;
    m_cpFrames.clear();
}

bool ReplayEngine::allowButton(bool down, int button, bool player1) {
    if (m_injecting) return true;
    if (m_mode == ReplayMode::Replaying) return false; // ignore real input during replay
    if (m_mode == ReplayMode::Recording) {
        m_actions.push_back({m_frame, button, down, player1});
    }
    return true;
}

void ReplayEngine::beforeStep(GJBaseGameLayer* layer) {
    if (m_mode != ReplayMode::Replaying) return;
    m_injecting = true;
    while (m_index < m_actions.size() && m_actions[m_index].frame <= m_frame) {
        auto const& a = m_actions[m_index++];
        layer->handleButton(a.down, a.button, a.player1);
    }
    m_injecting = false;
}

void ReplayEngine::afterStep() {
    if (m_mode != ReplayMode::Idle) ++m_frame;
}
}
