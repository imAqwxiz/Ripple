#include "ReplaysTab.hpp"
#include "ReplayEngine.hpp"

namespace ripple {

bool ReplaysTab::build() {
    auto s = this->getContentSize();

    m_status = addLabel("", {s.width / 2.f, s.height - 14.f}, 0.5f, theme::kFoam);
    m_info = addLabel("", {s.width / 2.f, s.height - 34.f}, 0.35f, theme::kAqua);

    float const bw = 66.f, bh = 30.f, y = 122.f;
    addButton("Record", {6.f, y}, {bw, bh}, theme::kCoral, [this] {
        ReplayEngine::get().armRecord();
        updateStatus();
    });
    addButton("Replay", {78.f, y}, {bw, bh}, theme::kAqua, [this] {
        ReplayEngine::get().armReplay();
        updateStatus();
    });
    addButton("Stop", {150.f, y}, {bw, bh}, theme::kSlate, [this] {
        ReplayEngine::get().stop();
        updateStatus();
    });
    addButton("Save", {222.f, y}, {bw, bh}, theme::kDeep, [this] {
        if (ReplayEngine::get().save()) refresh();
        else updateStatus();
    });

    addLabel("Saved macros", {s.width / 2.f, 111.f}, 0.35f, theme::kFoam);
    m_empty = addLabel("No saved macros yet", {s.width / 2.f, 52.f}, 0.35f, theme::kMist);

    addButton("<", {6.f, 4.f}, {30.f, 18.f}, theme::kDeep, [this] {
        if (m_page > 0) {
            --m_page;
            rebuildList();
        }
    }, 0.4f);
    addButton(">", {s.width - 36.f, 4.f}, {30.f, 18.f}, theme::kDeep, [this] {
        if ((m_page + 1) * kPerPage < m_files.size()) {
            ++m_page;
            rebuildList();
        }
    }, 0.4f);
    m_pageLabel = addLabel("1/1", {s.width / 2.f, 13.f}, 0.3f, theme::kMist);

    this->schedule(schedule_selector(ReplaysTab::tick), 0.2f);
    refresh();
    return true;
}

void ReplaysTab::refresh() {
    m_files = ReplayEngine::get().listSaved();
    rebuildList();
    updateStatus();
}

void ReplaysTab::tick(float) {
    updateStatus();
}

void ReplaysTab::rebuildList() {
    for (auto row : m_rows) this->removeButton(row);
    m_rows.clear();

    size_t pages = std::max<size_t>(1, (m_files.size() + kPerPage - 1) / kPerPage);
    if (m_page >= pages) m_page = pages - 1;

    auto const& loaded = ReplayEngine::get().loadedName();
    size_t start = m_page * kPerPage;
    for (size_t i = 0; i < kPerPage && start + i < m_files.size(); ++i) {
        auto path = m_files[start + i];
        auto name = path.stem().string();
        bool selected = (name == loaded);
        auto row = addButton(name, {6.f, 80.f - 26.f * static_cast<float>(i)}, {282.f, 22.f},
                             selected ? theme::kAqua : theme::kDeep,
                             [this, path] {
                                 ReplayEngine::get().load(path);
                                 refresh();
                             }, 0.32f);
        m_rows.push_back(row);
    }

    m_empty->setVisible(m_files.empty());
    m_pageLabel->setString(fmt::format("{}/{}", m_page + 1, pages).c_str());
}

void ReplaysTab::updateStatus() {
    auto& e = ReplayEngine::get();
    std::string status;

    if (e.mode() == ReplayMode::Recording) {
        status = fmt::format("Recording... {} inputs", e.actionCount());
    } else if (e.mode() == ReplayMode::Replaying) {
        status = fmt::format("Replaying... frame {}", e.frame());
    } else if (e.pending() == ReplayMode::Recording) {
        status = "Armed: start or restart a level to record";
    } else if (e.pending() == ReplayMode::Replaying) {
        status = "Armed: start or restart a level to replay";
    } else {
        status = e.message().empty() ? "Idle" : e.message();
    }
    m_status->setString(status.c_str());

    std::string info;
    if (e.actionCount() == 0) info = "No macro loaded";
    else if (e.loadedName().empty()) info = fmt::format("Unsaved recording - {} inputs", e.actionCount());
    else info = fmt::format("Loaded: {} - {} inputs", e.loadedName(), e.actionCount());
    m_info->setString(info.c_str());

    // keep long text inside the content area
    for (auto l : {m_status, m_info}) {
        float base = (l == m_status) ? 0.5f : 0.35f;
        l->setScale(base);
        float w = l->getContentSize().width * base;
        float maxW = this->getContentSize().width - 8.f;
        if (w > maxW) l->setScale(base * maxW / w);
    }
}
}
