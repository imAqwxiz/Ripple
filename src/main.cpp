#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include "core/RippleMenu.hpp"
#include "replays/ReplayEngine.hpp"

using namespace geode::prelude;
using namespace ripple;

// Small "Ripple" button in the top-left that opens the menu (main menu + pause menu).
static CCMenu* makeRippleEntry(CCObject* target, SEL_MenuHandler handler) {
    auto win = CCDirector::get()->getWinSize();
    auto item = CCMenuItemSpriteExtra::create(ButtonSprite::create("Ripple"), target, handler);
    auto menu = CCMenu::create();
    menu->addChild(item);
    menu->setPosition({58.f, win.height - 28.f});
    return menu;
}

class $modify(RippleMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        this->addChild(makeRippleEntry(this, menu_selector(RippleMenuLayer::onRipple)));
        return true;
    }
    void onRipple(CCObject*) { RippleMenu::open(); }
};

class $modify(RipplePauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        this->addChild(makeRippleEntry(this, menu_selector(RipplePauseLayer::onRipple)));
    }
    void onRipple(CCObject*) { RippleMenu::open(); }
};

class $modify(RipplePlayLayer, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        ReplayEngine::get().onReset(this);
    }
    void storeCheckpoint(CheckpointObject* cp) {
        PlayLayer::storeCheckpoint(cp);
        ReplayEngine::get().onCheckpoint(m_checkpointArray ? static_cast<int>(m_checkpointArray->count()) : 0);
    }
    void levelComplete() {
        PlayLayer::levelComplete();
        ReplayEngine::get().onLevelEnd();
    }
    void onQuit() {
        ReplayEngine::get().onLevelEnd();
        PlayLayer::onQuit();
    }
};

class $modify(RippleBaseLayer, GJBaseGameLayer) {
    bool inPlayLayer() {
        return typeinfo_cast<PlayLayer*>(static_cast<GJBaseGameLayer*>(this)) != nullptr;
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        if (this->inPlayLayer() && !ReplayEngine::get().allowButton(down, button, isPlayer1)) return;
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        // count/inject once per full physics step, not on half ticks
        bool play = !isHalfTick && this->inPlayLayer();
        if (play) ReplayEngine::get().beforeStep(this);
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        if (play) ReplayEngine::get().afterStep();
    }
};
