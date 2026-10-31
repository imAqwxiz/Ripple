#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <functional>
#include <string>
#include "Theme.hpp"

namespace ripple {
using namespace geode::prelude;

// Flat sea-styled button. Touches are handled by RippleMenu (hit-testing), not by CCMenu.
class RippleButton : public CCNode {
protected:
    CCLayerColor* m_bg = nullptr;
    CCLabelBMFont* m_label = nullptr;
    std::function<void()> m_callback;
    ccColor3B m_color = {255, 255, 255};
    float m_maxScale = 0.5f;
    bool m_enabled = true;
    bool m_active = false;
    bool m_pressed = false;

    void fit() {
        m_label->setScale(1.f);
        float w = std::max(m_label->getContentSize().width, 1.f);
        float s = std::min(m_maxScale, (this->getContentSize().width - 10.f) / w);
        m_label->setScale(s);
    }

    void restyle() {
        m_bg->setColor(m_active ? theme::kAqua : m_color);
        GLubyte o = !m_enabled ? 70 : m_pressed ? 130 : m_active ? 255 : 200;
        m_bg->setOpacity(o);
        m_label->setColor(m_active ? ccc3(2, 24, 42) : ccc3(255, 255, 255));
        m_label->setOpacity(m_enabled ? 255 : 120);
    }

    bool init(std::string const& text, CCSize size, ccColor3B color, std::function<void()> cb, float textScale) {
        if (!CCNode::init()) return false;
        m_color = color;
        m_callback = std::move(cb);
        m_maxScale = textScale;
        this->setAnchorPoint({0.f, 0.f});
        this->setContentSize(size);

        m_bg = CCLayerColor::create(ccc4(color.r, color.g, color.b, 200), size.width, size.height);
        this->addChild(m_bg);

        auto edge = CCLayerColor::create(ccc4(255, 255, 255, 60), size.width, 1.f);
        edge->setPosition({0.f, size.height - 1.f});
        this->addChild(edge, 1);

        m_label = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
        m_label->setPosition({size.width / 2.f, size.height / 2.f});
        this->addChild(m_label, 2);

        fit();
        restyle();
        return true;
    }

public:
    static RippleButton* create(std::string const& text, CCSize size, ccColor3B color,
                                std::function<void()> cb, float textScale = 0.45f) {
        auto ret = new RippleButton();
        if (ret->init(text, size, color, std::move(cb), textScale)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool hit(CCPoint const& world) {
        if (!m_enabled || !this->isVisible()) return false;
        auto p = this->convertToNodeSpace(world);
        auto s = this->getContentSize();
        return p.x >= 0.f && p.y >= 0.f && p.x <= s.width && p.y <= s.height;
    }

    void press(bool down) { m_pressed = down; restyle(); }
    void setActive(bool a) { m_active = a; restyle(); }
    void setEnabled(bool e) { m_enabled = e; restyle(); }
    void setText(std::string const& t) { m_label->setString(t.c_str()); fit(); }

    void trigger() {
        if (!m_enabled || !m_callback) return;
        auto cb = m_callback; // callback may remove this button
        this->retain();
        cb();
        this->release();
    }
};
}
