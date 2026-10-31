#pragma once
#include "Widgets.hpp"
#include <vector>

namespace ripple {

// Base class for every menu tab. Each tab lives in its own folder under src/.
class RippleTab : public CCNode {
protected:
    std::vector<RippleButton*> m_buttons;

public:
    virtual bool build() { return true; }  // create the tab's widgets
    virtual void refresh() {}              // called whenever the tab is opened

    bool initTab(CCSize size) {
        if (!CCNode::init()) return false;
        this->setAnchorPoint({0.f, 0.f});
        this->setContentSize(size);
        return true;
    }

    std::vector<RippleButton*> const& buttons() const { return m_buttons; }

    RippleButton* addButton(std::string const& text, CCPoint pos, CCSize size, ccColor3B color,
                            std::function<void()> cb, float scale = 0.45f) {
        auto b = RippleButton::create(text, size, color, std::move(cb), scale);
        b->setPosition(pos);
        this->addChild(b);
        m_buttons.push_back(b);
        return b;
    }

    void removeButton(RippleButton* b) {
        m_buttons.erase(std::remove(m_buttons.begin(), m_buttons.end(), b), m_buttons.end());
        b->removeFromParent();
    }

    CCLabelBMFont* addLabel(std::string const& text, CCPoint pos, float scale, ccColor3B color) {
        auto l = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
        l->setPosition(pos);
        l->setScale(scale);
        l->setColor(color);
        this->addChild(l);
        return l;
    }
};

template <class T>
T* createTab(CCSize size) {
    auto t = new T();
    if (t->initTab(size) && t->build()) {
        t->autorelease();
        return t;
    }
    delete t;
    return nullptr;
}
}
