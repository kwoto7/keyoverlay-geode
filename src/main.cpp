#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <algorithm>
#include "Timeline.hpp"
using namespace geode::prelude;

namespace {
constexpr char OverlayID[] = "kwoto.hold_overlay/overlay";
float setting(char const* name) { return static_cast<float>(Mod::get()->getSettingValue<double>(name)); }
bool enabled(char const* name) { return Mod::get()->getSettingValue<bool>(name); }

class HoldOverlay : public CCNode {
    Timeline timeline;
    CCDrawNode* drawing = nullptr;
    CCLabelBMFont* labels[2] = {};
    void rectangle(float x, float y, float w, float h, ccColor4F color) {
        CCPoint points[] = {{x,y},{x+w,y},{x+w,y+h},{x,y+h}};
        drawing->drawPolygon(points, 4, color, 0, color);
    }
public:
    static HoldOverlay* create() {
        auto result = new HoldOverlay;
        if (result->init()) { result->autorelease(); return result; }
        delete result;
        return nullptr;
    }
    bool init() override {
        if (!CCNode::init()) return false;
        setID(OverlayID);
        drawing = CCDrawNode::create();
        addChild(drawing);
        for (int i=0; i<2; ++i) {
            labels[i] = CCLabelBMFont::create("0", "bigFont.fnt");
            addChild(labels[i]);
        }
        scheduleUpdate();
        return true;
    }
    void input(bool down, bool player1) {
        timeline.lanes[player1 ? 0 : 1].input(down, timeline.now);
    }
    void reset() { timeline.reset(); }
    void release() { timeline.release(); }
    void update(float dt) override {
        timeline.now += std::max(0.f, dt);
        auto size = CCDirector::sharedDirector()->getWinSize();
        float width = setting("width"), height = setting("height");
        float speed = setting("speed"), opacity = setting("opacity") / 100.f;
        float scale = std::min({setting("scale"), size.width/(2*width+10), size.height/(height+width+4)});
        setScale(scale);
        int lanes = enabled("show-p2") ? 2 : 1;
        float totalWidth = lanes*width+(lanes-1)*10;
        setPosition({std::clamp(setting("x"), 0.f, std::max(0.f,size.width-totalWidth*scale)),
                     std::clamp(setting("y"), 0.f, std::max(0.f,size.height-(height+width+4)*scale))});
        setVisible(enabled("enabled"));
        drawing->clear();
        for (int i=0; i<2; ++i) {
            auto& lane = timeline.lanes[i];
            lane.prune(timeline.now, height/speed);
            labels[i]->setVisible(i < lanes);
            if (i >= lanes) continue;
            float x = i*(width+10), base = width+4;
            ccColor4F border = {0.90f, 0.76f, 0.48f, opacity};
            rectangle(x,0,width,width,{0.f,0.f,0.f,opacity*0.35f});
            if (lane.down) rectangle(x,0,width,width,{1.f,1.f,1.f,opacity*0.4f});
            rectangle(x,0,width,1,border); rectangle(x,width-1,width,1,border);
            rectangle(x,0,1,width,border); rectangle(x+width-1,0,1,width,border);
            for (auto const& hold : lane.holds) {
                float bottom = hold.active ? 0.f : static_cast<float>((timeline.now-hold.end)*speed);
                float top = static_cast<float>((timeline.now-hold.start)*speed);
                bottom = std::clamp(bottom,0.f,height);
                top = std::clamp(std::max(top,bottom+1.f),0.f,height);
                if (top <= bottom) continue;
                // Small bands keep the fade spatially smooth even on a long hold.
                for (float y=bottom; y<top; y+=3.f) {
                    float h=std::min(3.f,top-y);
                    float fade=enabled("fade") ? std::clamp((height-y-h/2)/(height*0.25f),0.f,1.f) : 1.f;
                    rectangle(x,base+y,width,h,{1.f,1.f,1.f,opacity*fade});
                }
            }
            std::string text = enabled("show-cps") ? std::to_string(lane.presses.size()) : std::to_string(lane.count);
            labels[i]->setString(text.c_str());
            labels[i]->setScale(std::min(0.38f,(width-4)/std::max(1.f,labels[i]->getContentSize().width)));
            labels[i]->setPosition({x+width/2,width/2});
            labels[i]->setOpacity(static_cast<GLubyte>(255*opacity));
        }
    }
};
HoldOverlay* overlay(PlayLayer* layer) {
    return layer ? static_cast<HoldOverlay*>(layer->getChildByID(OverlayID)) : nullptr;
}
}
class $modify(HoldPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level,useReplay,dontCreateObjects)) return false;
        if (auto hud=HoldOverlay::create()) addChild(hud,10000);
        return true;
    }
    void resetLevel() {
        PlayLayer::resetLevel();
        if (auto hud=overlay(this)) hud->reset();
    }
    void pauseGame(bool unknown) {
        if (auto hud=overlay(this)) hud->release();
        PlayLayer::pauseGame(unknown);
    }
};
class $modify(HoldInputLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool player1) {
        GJBaseGameLayer::handleButton(down,button,player1);
        auto play=PlayLayer::get();
        if (play && static_cast<GJBaseGameLayer*>(play)==this && button==1)
            if (auto hud=overlay(play)) hud->input(down,player1);
    }
};
