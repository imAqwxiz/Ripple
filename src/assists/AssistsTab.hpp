#pragma once
#include "../core/Tab.hpp"

namespace ripple {
class AssistsTab : public RippleTab {
public:
    bool build() override {
        auto s = this->getContentSize();
        addLabel("Assists", {s.width / 2.f, s.height / 2.f + 14.f}, 0.55f, theme::kAqua);
        addLabel("Coming soon", {s.width / 2.f, s.height / 2.f - 12.f}, 0.32f, theme::kMist);
        return true;
    }
};
}
