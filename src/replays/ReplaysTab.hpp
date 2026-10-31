#pragma once
#include "../core/Tab.hpp"
#include <filesystem>
#include <vector>

namespace ripple {

class ReplaysTab : public RippleTab {
protected:
    static constexpr size_t kPerPage = 3;

    CCLabelBMFont* m_status = nullptr;
    CCLabelBMFont* m_info = nullptr;
    CCLabelBMFont* m_pageLabel = nullptr;
    CCLabelBMFont* m_empty = nullptr;
    std::vector<std::filesystem::path> m_files;
    std::vector<RippleButton*> m_rows;
    size_t m_page = 0;

    void rebuildList();
    void updateStatus();

public:
    bool build() override;
    void refresh() override;
    void tick(float dt);
};
}
