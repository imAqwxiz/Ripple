#pragma once
#include "../core/Widgets.hpp"
#include <functional>
#include <vector>

namespace ripple {

// Sea-styled "Save Macro" window: name field (pre-filled with the level name)
// and a locked ".rpl" format checkbox at the bottom.
class SavePopup : public CCLayer {
protected:
    static inline SavePopup* s_open = nullptr;

    CCNode* m_panel = nullptr;
    TextInput* m_input = nullptr;
    CCLabelBMFont* m_msg = nullptr;
    std::vector<RippleButton*> m_buttons;
    RippleButton* m_pressed = nullptr;
    std::string m_confirmName;
    std::function<void()> m_onSaved;

    bool init() override;
    void onSave();
    RippleButton* findButton(CCPoint const& p);

    void registerWithTouchDispatcher() override;
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override;
    void ccTouchMoved(CCTouch* touch, CCEvent* event) override;
    void ccTouchEnded(CCTouch* touch, CCEvent* event) override;
    void ccTouchCancelled(CCTouch* touch, CCEvent* event) override;
    void keyBackClicked() override;

public:
    ~SavePopup() override;
    static SavePopup* open(std::function<void()> onSaved);
    void close();
};
}
