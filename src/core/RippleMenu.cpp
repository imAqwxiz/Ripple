#include "RippleMenu.hpp"
#include "../replays/ReplaysTab.hpp"
#include "../render/RenderTab.hpp"
#include "../assists/AssistsTab.hpp"
#include "../visuals/VisualsTab.hpp"
#include "../settings/SettingsTab.hpp"
#include <cmath>
#include <cstdlib>

namespace ripple {

static char const* const kTabNames[5] = {"Ripple Replays", "Render", "Assists", "Visuals", "Settings"};

static float rnd(float a, float b) {
    return a + (b - a) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
}

RippleMenu::~RippleMenu() {
    if (s_open == this) s_open = nullptr;
}

RippleMenu* RippleMenu::open() {
    if (s_open) return s_open;
    auto scene = CCDirector::get()->getRunningScene();
    if (!scene) return nullptr;

    auto menu = new RippleMenu();
    if (menu->init()) {
        menu->autorelease();
        scene->addChild(menu, 1000);
        s_open = menu;
        return menu;
    }
    delete menu;
    return nullptr;
}

void RippleMenu::close() {
    this->setKeypadEnabled(false);
    this->setTouchEnabled(false);
    if (s_open == this) s_open = nullptr;
    this->removeFromParentAndCleanup(true);
}

bool RippleMenu::init() {
    if (!CCLayer::init()) return false;

    auto win = CCDirector::get()->getWinSize();
    auto const ps = m_size;

    // dim background
    this->addChild(CCLayerColor::create(ccc4(0, 12, 22, 175), win.width, win.height));

    // panel
    m_panel = CCNode::create();
    m_panel->setAnchorPoint({0.f, 0.f});
    m_panel->setContentSize(ps);
    m_panel->setPosition({(win.width - ps.width) / 2.f, (win.height - ps.height) / 2.f});
    this->addChild(m_panel);

    auto bg = CCLayerGradient::create(theme::kSurface, theme::kAbyss);
    bg->setContentSize(ps);
    m_panel->addChild(bg);
    m_panel->addChild(CCLayerColor::create(theme::kSide, 110.f, ps.height), 1);

    // animated bubbles + wave
    m_fx = CCDrawNode::create();
    m_panel->addChild(m_fx, 2);

    // static frame
    auto frame = CCDrawNode::create();
    ccColor4F edge = ccc4f(0.18f, 0.77f, 0.71f, 0.9f);
    ccColor4F soft = ccc4f(0.18f, 0.77f, 0.71f, 0.35f);
    frame->drawSegment(CCPoint(0.f, 0.f), CCPoint(ps.width, 0.f), 1.f, edge);
    frame->drawSegment(CCPoint(ps.width, 0.f), CCPoint(ps.width, ps.height), 1.f, edge);
    frame->drawSegment(CCPoint(ps.width, ps.height), CCPoint(0.f, ps.height), 1.f, edge);
    frame->drawSegment(CCPoint(0.f, ps.height), CCPoint(0.f, 0.f), 1.f, edge);
    frame->drawSegment(CCPoint(110.f, 8.f), CCPoint(110.f, ps.height - 8.f), 0.7f, soft);
    m_panel->addChild(frame, 3);

    // titles
    auto title = CCLabelBMFont::create("Ripple", "bigFont.fnt");
    title->setColor(theme::kAqua);
    title->setScale(0.8f);
    title->setPosition({55.f, ps.height - 20.f});
    m_panel->addChild(title, 5);

    m_header = CCLabelBMFont::create("", "bigFont.fnt");
    m_header->setAnchorPoint({0.f, 0.5f});
    m_header->setScale(0.6f);
    m_header->setColor(theme::kFoam);
    m_header->setPosition({122.f, ps.height - 20.f});
    m_panel->addChild(m_header, 5);

    // tabs
    CCSize cs(294.f, 202.f);
    m_tabs = {
        createTab<ReplaysTab>(cs),
        createTab<RenderTab>(cs),
        createTab<AssistsTab>(cs),
        createTab<VisualsTab>(cs),
        createTab<SettingsTab>(cs),
    };

    for (int i = 0; i < 5; ++i) {
        auto tab = m_tabs[i];
        tab->setPosition({118.f, 8.f});
        tab->setVisible(false);
        m_panel->addChild(tab, 4);

        auto btn = RippleButton::create(kTabNames[i], CCSize(94.f, 30.f), theme::kDeep,
                                        [this, i] { selectTab(i); }, 0.42f);
        btn->setPosition({8.f, ps.height - 66.f - static_cast<float>(i) * 38.f});
        m_panel->addChild(btn, 4);
        m_tabButtons.push_back(btn);
    }

    // close
    m_close = RippleButton::create("X", CCSize(26.f, 22.f), theme::kCoral, [this] { close(); }, 0.5f);
    m_close->setPosition({ps.width - 34.f, ps.height - 32.f});
    m_panel->addChild(m_close, 6);

    // bubbles
    for (int i = 0; i < 14; ++i) {
        m_bubbles.push_back({rnd(0.f, ps.width), rnd(0.f, ps.height), rnd(1.5f, 4.f), rnd(8.f, 22.f), rnd(0.f, 6.28f)});
    }

    selectTab(0);
    this->scheduleUpdate();

    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);
    return true;
}

void RippleMenu::selectTab(int idx) {
    m_current = idx;
    for (size_t j = 0; j < m_tabs.size(); ++j) {
        m_tabs[j]->setVisible(static_cast<int>(j) == idx);
        m_tabButtons[j]->setActive(static_cast<int>(j) == idx);
    }
    m_header->setString(kTabNames[idx]);
    m_tabs[idx]->refresh();
}

void RippleMenu::update(float dt) {
    m_time += dt;
    m_fx->clear();
    auto const ps = m_size;

    for (auto& b : m_bubbles) {
        b.y += b.speed * dt;
        if (b.y > ps.height + 6.f) {
            b.y = -6.f;
            b.x = rnd(0.f, ps.width);
        }
        float x = b.x + std::sin(m_time * 1.5f + b.phase) * 4.f;
        m_fx->drawDot(CCPoint(x, b.y), b.r, ccc4f(0.88f, 0.98f, 0.99f, 0.16f));
    }

    // wave under the header
    float baseY = ps.height - 38.f;
    CCPoint prev(118.f, baseY);
    for (float x = 124.f; x <= ps.width - 8.f; x += 6.f) {
        CCPoint cur(x, baseY + std::sin(x * 0.06f + m_time * 2.f) * 2.5f);
        m_fx->drawSegment(prev, cur, 0.9f, ccc4f(0.18f, 0.77f, 0.71f, 0.8f));
        prev = cur;
    }
}

// ---- touch handling ----

void RippleMenu::registerWithTouchDispatcher() {
    CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
}

RippleButton* RippleMenu::findButton(CCPoint const& p) {
    if (m_close && m_close->hit(p)) return m_close;
    for (auto b : m_tabButtons) {
        if (b->hit(p)) return b;
    }
    if (m_current >= 0) {
        for (auto b : m_tabs[m_current]->buttons()) {
            if (b->hit(p)) return b;
        }
    }
    return nullptr;
}

bool RippleMenu::ccTouchBegan(CCTouch* touch, CCEvent*) {
    m_pressed = findButton(touch->getLocation());
    if (m_pressed) m_pressed->press(true);
    return true; // swallow everything while the menu is open
}

void RippleMenu::ccTouchMoved(CCTouch* touch, CCEvent*) {
    if (m_pressed && !m_pressed->hit(touch->getLocation())) {
        m_pressed->press(false);
        m_pressed = nullptr;
    }
}

void RippleMenu::ccTouchEnded(CCTouch* touch, CCEvent*) {
    if (!m_pressed) return;
    auto btn = m_pressed;
    m_pressed = nullptr;
    btn->press(false);
    if (btn->hit(touch->getLocation())) btn->trigger();
}

void RippleMenu::ccTouchCancelled(CCTouch*, CCEvent*) {
    if (m_pressed) {
        m_pressed->press(false);
        m_pressed = nullptr;
    }
}

void RippleMenu::keyBackClicked() {
    close();
}
}
