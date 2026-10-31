#pragma once
#include "Tab.hpp"
#include <vector>

namespace ripple {

// The custom sea-themed Ripple menu (not a GD popup). Handles its own touches.
class RippleMenu : public CCLayer {
protected:
    struct Bubble {
        float x, y, r, speed, phase;
    };

    static inline RippleMenu* s_open = nullptr;

    CCNode* m_panel = nullptr;
    CCDrawNode* m_fx = nullptr;
    CCLabelBMFont* m_header = nullptr;
    CCSize m_size = CCSize(420.f, 250.f);
    std::vector<RippleTab*> m_tabs;
    std::vector<RippleButton*> m_tabButtons;
    std::vector<Bubble> m_bubbles;
    RippleButton* m_close = nullptr;
    RippleButton* m_pressed = nullptr;
    int m_current = -1;
    float m_time = 0.f;
    bool m_prioHeld = false;

    bool init() override;
    void selectTab(int idx);
    void releasePrio();
    RippleButton* findButton(CCPoint const& p);

    void update(float dt) override;
    void registerWithTouchDispatcher() override;
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override;
    void ccTouchMoved(CCTouch* touch, CCEvent* event) override;
    void ccTouchEnded(CCTouch* touch, CCEvent* event) override;
    void ccTouchCancelled(CCTouch* touch, CCEvent* event) override;
    void keyBackClicked() override;

public:
    ~RippleMenu() override;
    static RippleMenu* open();
    void close();
};
}
