#include "SavePopup.hpp"
#include "ReplayEngine.hpp"

namespace ripple {

SavePopup::~SavePopup() {
    if (s_open == this) s_open = nullptr;
}

SavePopup* SavePopup::open(std::function<void()> onSaved) {
    if (s_open) return s_open;
    auto scene = CCDirector::get()->getRunningScene();
    if (!scene) return nullptr;

    auto popup = new SavePopup();
    popup->m_onSaved = std::move(onSaved);
    if (popup->init()) {
        popup->autorelease();
        scene->addChild(popup, 1100);
        s_open = popup;
        return popup;
    }
    delete popup;
    return nullptr;
}

void SavePopup::close() {
    this->setKeypadEnabled(false);
    this->setTouchEnabled(false);
    if (s_open == this) s_open = nullptr;
    this->removeFromParentAndCleanup(true);
}

bool SavePopup::init() {
    if (!CCLayer::init()) return false;

    auto win = CCDirector::get()->getWinSize();
    CCSize ps(300.f, 178.f);

    this->addChild(CCLayerColor::create(ccc4(0, 12, 22, 150), win.width, win.height));

    m_panel = CCNode::create();
    m_panel->setAnchorPoint({0.f, 0.f});
    m_panel->setContentSize(ps);
    m_panel->setPosition({(win.width - ps.width) / 2.f, (win.height - ps.height) / 2.f});
    this->addChild(m_panel);

    auto bg = CCLayerGradient::create(theme::kSurface, theme::kAbyss);
    bg->setContentSize(ps);
    m_panel->addChild(bg);

    auto frame = CCDrawNode::create();
    ccColor4F edge = ccc4f(0.18f, 0.77f, 0.71f, 0.9f);
    frame->drawSegment(CCPoint(0.f, 0.f), CCPoint(ps.width, 0.f), 1.f, edge);
    frame->drawSegment(CCPoint(ps.width, 0.f), CCPoint(ps.width, ps.height), 1.f, edge);
    frame->drawSegment(CCPoint(ps.width, ps.height), CCPoint(0.f, ps.height), 1.f, edge);
    frame->drawSegment(CCPoint(0.f, ps.height), CCPoint(0.f, 0.f), 1.f, edge);
    m_panel->addChild(frame, 2);

    auto title = CCLabelBMFont::create("Save Macro", "bigFont.fnt");
    title->setColor(theme::kAqua);
    title->setScale(0.65f);
    title->setPosition({ps.width / 2.f, ps.height - 22.f});
    m_panel->addChild(title, 3);

    // name field, pre-filled with the level name
    m_input = TextInput::create(240.f, "Macro name", "bigFont.fnt");
    m_input->setString(ReplayEngine::get().defaultName());
    m_input->setPosition({ps.width / 2.f, 112.f});
    m_panel->addChild(m_input, 4);

    m_msg = CCLabelBMFont::create("", "bigFont.fnt");
    m_msg->setScale(0.3f);
    m_msg->setColor(theme::kMist);
    m_msg->setPosition({ps.width / 2.f, 82.f});
    m_panel->addChild(m_msg, 3);

    // locked format checkbox (always checked)
    auto box = CCLayerColor::create(ccc4(46, 196, 182, 90), 16.f, 16.f);
    box->setPosition({20.f, 17.f});
    m_panel->addChild(box, 3);

    auto cb = CCDrawNode::create();
    ccColor4F c = ccc4f(0.88f, 0.98f, 0.99f, 0.95f);
    cb->drawSegment(CCPoint(20.f, 17.f), CCPoint(36.f, 17.f), 0.8f, c);
    cb->drawSegment(CCPoint(36.f, 17.f), CCPoint(36.f, 33.f), 0.8f, c);
    cb->drawSegment(CCPoint(36.f, 33.f), CCPoint(20.f, 33.f), 0.8f, c);
    cb->drawSegment(CCPoint(20.f, 33.f), CCPoint(20.f, 17.f), 0.8f, c);
    cb->drawSegment(CCPoint(23.f, 25.f), CCPoint(27.5f, 20.5f), 1.4f, c);
    cb->drawSegment(CCPoint(27.5f, 20.5f), CCPoint(34.f, 30.f), 1.4f, c);
    m_panel->addChild(cb, 4);

    auto fmtLabel = CCLabelBMFont::create(".rpl", "bigFont.fnt");
    fmtLabel->setAnchorPoint({0.f, 0.5f});
    fmtLabel->setScale(0.5f);
    fmtLabel->setColor(theme::kFoam);
    fmtLabel->setPosition({44.f, 25.f});
    m_panel->addChild(fmtLabel, 4);

    // buttons
    auto cancel = RippleButton::create("Cancel", CCSize(72.f, 26.f), theme::kSlate, [this] { close(); }, 0.4f);
    cancel->setPosition({140.f, 12.f});
    m_panel->addChild(cancel, 4);
    m_buttons.push_back(cancel);

    auto save = RippleButton::create("Save", CCSize(72.f, 26.f), theme::kAqua, [this] { onSave(); }, 0.4f);
    save->setPosition({218.f, 12.f});
    m_panel->addChild(save, 4);
    m_buttons.push_back(save);

    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);
    return true;
}

void SavePopup::onSave() {
    auto name = ReplayEngine::sanitizeName(m_input->getString());
    bool overwrite = (m_confirmName == name);

    auto result = ReplayEngine::get().save(name, overwrite);
    if (result == SaveResult::Saved) {
        auto cb = m_onSaved;
        close();
        if (cb) cb();
    } else if (result == SaveResult::Exists) {
        m_confirmName = name;
        m_msg->setColor(theme::kCoral);
        m_msg->setString("File exists - press Save again to overwrite");
    } else {
        m_msg->setColor(theme::kCoral);
        m_msg->setString("Could not save the macro");
    }
}

// ---- touch handling ----

void SavePopup::registerWithTouchDispatcher() {
    CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
}

RippleButton* SavePopup::findButton(CCPoint const& p) {
    for (auto b : m_buttons) {
        if (b->hit(p)) return b;
    }
    return nullptr;
}

bool SavePopup::ccTouchBegan(CCTouch* touch, CCEvent*) {
    m_pressed = findButton(touch->getLocation());
    if (m_pressed) m_pressed->press(true);
    return true;
}

void SavePopup::ccTouchMoved(CCTouch* touch, CCEvent*) {
    if (m_pressed && !m_pressed->hit(touch->getLocation())) {
        m_pressed->press(false);
        m_pressed = nullptr;
    }
}

void SavePopup::ccTouchEnded(CCTouch* touch, CCEvent*) {
    if (!m_pressed) return;
    auto btn = m_pressed;
    m_pressed = nullptr;
    btn->press(false);
    if (btn->hit(touch->getLocation())) btn->trigger();
}

void SavePopup::ccTouchCancelled(CCTouch*, CCEvent*) {
    if (m_pressed) {
        m_pressed->press(false);
        m_pressed = nullptr;
    }
}

void SavePopup::keyBackClicked() {
    close();
}
}
