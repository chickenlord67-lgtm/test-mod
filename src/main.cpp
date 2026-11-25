// SussyMod | GD 2.208 Android | Geode | C++20
// features: noclip, hitboxes, speedhack, jumphack, startpos switcher

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/DebugDrawNode.hpp>
#include <Geode/binding/StartPosObject.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/ui/Slider.hpp>
#include <cocos2d.h>

using namespace geode::prelude;
using namespace cocos2d;

// ── global state ────────────────────────────────────────────────────────────
static bool  s_noclip    = false;
static bool  s_hitboxes  = false;
static float s_speed     = 1.0f;
static float s_jumpForce = 1.0f;
static int   s_startpos  = 0;    // 0 = default start

// ── noclip — swallow hits and death ─────────────────────────────────────────
class $modify(MyPlayer, PlayerObject) {
    bool playerHit(GameObject* obj) {
        if (s_noclip) return false;
        return PlayerObject::playerHit(obj);
    }

    bool playerDestroyed(bool p0) {
        if (s_noclip) return false;
        return PlayerObject::playerDestroyed(p0);
    }

    // jump force — multiply y velocity right after the jump is applied
    void pushButton(PlayerButton btn) {
        PlayerObject::pushButton(btn);
        if (btn == PlayerButton::Jump && s_jumpForce != 1.0f) {
            m_yVel *= s_jumpForce;
        }
    }
};

// ── hitboxes — gate the debug draw ──────────────────────────────────────────
class $modify(MyDebug, DebugDrawNode) {
    void draw() {
        if (!s_hitboxes) return;
        DebugDrawNode::draw();
    }
};

// ── UI layer ─────────────────────────────────────────────────────────────────
class SussyModUI : public CCLayer {
public:
    CCLabelBMFont* m_spLabel     = nullptr;
    CCLabelBMFont* m_speedLabel  = nullptr;
    CCLabelBMFont* m_jumpLabel   = nullptr;

    static SussyModUI* create() {
        auto* ret = new SussyModUI();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float cx = winSize.width  * 0.5f;
        float cy = winSize.height * 0.5f;

        // dim background
        auto* dim = CCLayerColor::create({0, 0, 0, 140});
        this->addChild(dim, 0);

        // panel
        auto* panel = CCScale9Sprite::create("GJ_square02.png");
        panel->setContentSize({340, 440});
        panel->setPosition({cx, cy});
        this->addChild(panel, 1);

        // title
        auto* title = CCLabelBMFont::create("SussyMod", "goldFont.fnt");
        title->setScale(0.9f);
        title->setPosition({cx, cy + 195});
        this->addChild(title, 2);

        // separator line
        auto* line = CCSprite::createWithSpriteFrameName("floorLine_001.png");
        line->setPosition({cx, cy + 175});
        line->setScaleX(1.4f);
        line->setOpacity(100);
        this->addChild(line, 2);

        float y = cy + 140;
        float gap = 48.0f;

        // ── noclip toggle ──
        this->addLabel("Noclip", cx - 80, y);
        auto* noclipBtn = this->makeToggle(0, s_noclip, {cx + 90, y});
        this->addChild(noclipBtn, 2);
        y -= gap;

        // ── hitboxes toggle ──
        this->addLabel("Hitboxes", cx - 80, y);
        auto* hitboxBtn = this->makeToggle(1, s_hitboxes, {cx + 90, y});
        this->addChild(hitboxBtn, 2);
        y -= gap;

        // ── speed slider ──
        this->addLabel("Speed", cx - 100, y);
        m_speedLabel = CCLabelBMFont::create(
            fmt::format("{:.1f}x", s_speed).c_str(), "bigFont.fnt"
        );
        m_speedLabel->setScale(0.4f);
        m_speedLabel->setPosition({cx + 120, y});
        this->addChild(m_speedLabel, 2);

        auto* speedSlider = Slider::create(
            this, menu_selector(SussyModUI::onSpeedSlider), 0.75f
        );
        speedSlider->setPosition({cx + 20, y});
        speedSlider->setValue((s_speed - 0.1f) / 4.9f);
        speedSlider->setTag(10);
        this->addChild(speedSlider, 2);
        y -= gap;

        // ── jump slider ──
        this->addLabel("Jump", cx - 100, y);
        m_jumpLabel = CCLabelBMFont::create(
            fmt::format("{:.1f}x", s_jumpForce).c_str(), "bigFont.fnt"
        );
        m_jumpLabel->setScale(0.4f);
        m_jumpLabel->setPosition({cx + 120, y});
        this->addChild(m_jumpLabel, 2);

        auto* jumpSlider = Slider::create(
            this, menu_selector(SussyModUI::onJumpSlider), 0.75f
        );
        jumpSlider->setPosition({cx + 20, y});
        jumpSlider->setValue((s_jumpForce - 0.5f) / 4.5f);
        jumpSlider->setTag(11);
        this->addChild(jumpSlider, 2);
        y -= gap;

        // ── startpos stepper ──
        this->addLabel("StartPos", cx - 100, y);

        auto* spDown = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png"),
            this,
            menu_selector(SussyModUI::onSpDown)
        );
        spDown->setScale(0.55f);

        m_spLabel = CCLabelBMFont::create("Default", "bigFont.fnt");
        m_spLabel->setScale(0.42f);

        auto* spUp = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png"),
            this,
            menu_selector(SussyModUI::onSpUp)
        );
        spUp->setScale(0.55f);
        spUp->setFlipX(true);

        auto* spMenu = CCMenu::create(spDown, m_spLabel, spUp, nullptr);
        spMenu->setLayout(
            RowLayout::create()
                ->setGap(10.0f)
                ->setAxisAlignment(AxisAlignment::Center)
        );
        spMenu->setContentSize({160, 30});
        spMenu->setPosition({cx + 40, y});
        spMenu->updateLayout();
        this->addChild(spMenu, 2);

        // ── close button ──
        auto* closeBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"),
            this,
            menu_selector(SussyModUI::onClose)
        );
        closeBtn->setScale(0.9f);
        auto* closeMenu = CCMenu::create(closeBtn, nullptr);
        closeMenu->setPosition({cx + 155, cy + 200});
        this->addChild(closeMenu, 2);

        this->setTouchEnabled(true);
        this->setTouchMode(kCCTouchesOneByOne);

        return true;
    }

    // ── helpers ──────────────────────────────────────────────────────────────

    void addLabel(const char* text, float x, float y) {
        auto* lbl = CCLabelBMFont::create(text, "bigFont.fnt");
        lbl->setScale(0.48f);
        lbl->setPosition({x, y});
        lbl->setAnchorPoint({0.5f, 0.5f});
        this->addChild(lbl, 2);
    }

    CCMenu* makeToggle(int tag, bool state, CCPoint pos) {
        auto* tog = CCMenuItemToggler::createWithStandardSprites(
            this,
            menu_selector(SussyModUI::onToggle),
            0.7f
        );
        tog->setTag(tag);
        tog->toggle(state);

        auto* menu = CCMenu::create(tog, nullptr);
        menu->setPosition(pos);
        return menu;
    }

    // ── callbacks ─────────────────────────────────────────────────────────────

    void onToggle(CCObject* sender) {
        auto* tog = static_cast<CCMenuItemToggler*>(sender);
        bool val = !tog->isToggled();
        switch (tog->getTag()) {
            case 0: s_noclip   = val; break;
            case 1: s_hitboxes = val; break;
        }
    }

    void onSpeedSlider(CCObject* sender) {
        auto* sl = static_cast<Slider*>(sender);
        s_speed = 0.1f + sl->getValue() * 4.9f;
        m_speedLabel->setString(fmt::format("{:.1f}x", s_speed).c_str());
        CCDirector::sharedDirector()->getScheduler()->setTimeScale(s_speed);
    }

    void onJumpSlider(CCObject* sender) {
        auto* sl = static_cast<Slider*>(sender);
        s_jumpForce = 0.5f + sl->getValue() * 4.5f;
        m_jumpLabel->setString(fmt::format("{:.1f}x", s_jumpForce).c_str());
    }

    void onSpDown(CCObject*) {
        if (s_startpos > 0) s_startpos--;
        updateSpLabel();
    }

    void onSpUp(CCObject*) {
        s_startpos++;
        updateSpLabel();
    }

    void updateSpLabel() {
        if (s_startpos == 0)
            m_spLabel->setString("Default");
        else
            m_spLabel->setString(fmt::format("{}", s_startpos).c_str());
    }

    void onClose(CCObject*) {
        // restore timescale to current speed value on close
        CCDirector::sharedDirector()->getScheduler()->setTimeScale(s_speed);
        this->removeFromParent();
    }

    bool ccTouchBegan(CCTouch* touch, CCTouchEvent*) override {
        return true; // swallow touches so level doesn't get them
    }
};

// ── PlayLayer hooks ───────────────────────────────────────────────────────────
class $modify(MyPlayLayer, PlayLayer) {

    bool init(GJGameLevel* level, bool useReplay, bool dontCreate) {
        if (!PlayLayer::init(level, useReplay, dontCreate)) return false;

        // apply current speed on level load
        CCDirector::sharedDirector()->getScheduler()->setTimeScale(s_speed);

        // floating mod button — top left
        auto* icon = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        icon->setScale(0.65f);

        auto* btn = CCMenuItemSpriteExtra::create(
            icon,
            this,
            menu_selector(MyPlayLayer::onOpenMod)
        );

        auto* menu = CCMenu::create(btn, nullptr);
        menu->setPosition({28.0f, CCDirector::sharedDirector()->getWinSize().height - 28.0f});
        menu->setZOrder(999);
        this->addChild(menu);

        return true;
    }

    void startGame() {
        PlayLayer::startGame();

        // startpos switcher
        if (s_startpos > 0) {
            std::vector<StartPosObject*> sps;
            auto* objs = this->m_objects;

            for (int i = 0; i < static_cast<int>(objs->count()); i++) {
                if (auto* sp = typeinfo_cast<StartPosObject*>(objs->objectAtIndex(i)))
                    sps.push_back(sp);
            }

            if (!sps.empty()) {
                // sort by x position so index 1 = first startpos in level
                std::sort(sps.begin(), sps.end(), [](StartPosObject* a, StartPosObject* b) {
                    return a->getPositionX() < b->getPositionX();
                });

                int idx = std::clamp(s_startpos - 1, 0, static_cast<int>(sps.size()) - 1);
                this->setStartPos(sps[idx]);
                this->resetLevel();
            }
        }
    }

    void update(float dt) {
        // keep timescale locked to our value every frame
        // in case GD internally resets it
        CCDirector::sharedDirector()->getScheduler()->setTimeScale(s_speed);
        PlayLayer::update(dt);
    }

    void onQuit() {
        // reset timescale when leaving level so menus aren't sped up
        CCDirector::sharedDirector()->getScheduler()->setTimeScale(1.0f);
        PlayLayer::onQuit();
    }

    void onOpenMod(CCObject*) {
        auto* ui = SussyModUI::create();
        this->addChild(ui, 1000);
    }
};

// ── mod entry ─────────────────────────────────────────────────────────────────
$on_mod(Loaded) {
    log::info("SussyMod v1.0.0 loaded — GD 2.208 android");
}
