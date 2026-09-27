#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <algorithm>
#include <filesystem>
#include "Timeline.hpp"

using namespace geode::prelude;

namespace {
constexpr char OverlayID[] = "kwoto.hold_overlay/overlay";

float setting(char const* name) {
    return static_cast<float>(
        Mod::get()->getSettingValue<double>(name)
    );
}

bool enabled(char const* name) {
    return Mod::get()->getSettingValue<bool>(name);
}

class HoldOverlay : public CCNode {
    Timeline timeline;
    CCDrawNode* drawing = nullptr;
    CCLabelBMFont* labels[2] = {};
    CCSprite* backgrounds[2] = {};

    std::filesystem::path lastImage;
    bool imageLoaded = false;
    float imageCheck = 0.f;

    void rectangle(
        float x, float y, float w, float h, ccColor4F color
    ) {
        CCPoint points[] = {
            {x, y}, {x + w, y},
            {x + w, y + h}, {x, y + h}
        };
        drawing->drawPolygon(points, 4, color, 0, color);
    }

    void refreshBackground() {
        auto path =
            Mod::get()->getSettingValue<std::filesystem::path>(
                "button-image"
            );

        if (imageLoaded && path == lastImage) return;

        imageLoaded = true;
        lastImage = path;

        std::error_code error;
        bool custom = !path.empty() &&
            std::filesystem::is_regular_file(path, error);

        auto utf8 = path.u8string();
        std::string filename(utf8.begin(), utf8.end());

        for (int i = 0; i < 2; ++i) {
            if (backgrounds[i]) {
                backgrounds[i]->removeFromParentAndCleanup(true);
                backgrounds[i] = nullptr;
            }

            CCSprite* sprite = custom
                ? CCSprite::create(filename.c_str())
                : nullptr;

            if (!sprite) {
                sprite = CCSprite::create("duck.png"_spr);
            }

            if (sprite &&
                sprite->getContentSize().width > 0 &&
                sprite->getContentSize().height > 0) {
                backgrounds[i] = sprite;
                addChild(sprite, -1);
            }
        }
    }

public:
    static HoldOverlay* create() {
        auto result = new HoldOverlay;

        if (result->init()) {
            result->autorelease();
            return result;
        }

        delete result;
        return nullptr;
    }

    bool init() override {
        if (!CCNode::init()) return false;

        setID(OverlayID);
        drawing = CCDrawNode::create();
        addChild(drawing);

        for (int i = 0; i < 2; ++i) {
            labels[i] = CCLabelBMFont::create("0", "bigFont.fnt");
            addChild(labels[i], 1);
        }

        refreshBackground();
        scheduleUpdate();
        return true;
    }

    void input(bool down, bool player1) {
        timeline.lanes[player1 ? 0 : 1].input(
            down, timeline.now
        );
    }

    void reset() {
        timeline.reset();
    }

    void release() {
        timeline.release();
    }

    void update(float dt) override {
        timeline.now += std::max(0.f, dt);

        imageCheck -= std::max(0.f, dt);
        if (imageCheck <= 0.f) {
            refreshBackground();
            imageCheck = 0.25f;
        }

        auto size = CCDirector::sharedDirector()->getWinSize();

        float width = setting("width");
        float height = setting("height");
        float speed = setting("speed");
        float opacity = setting("opacity") / 100.f;

        int lanes = enabled("show-p2") ? 2 : 1;
        float totalWidth = lanes * width + (lanes - 1) * 10;

        float scale = std::min({
            setting("scale"),
            size.width / totalWidth,
            size.height / (height + width + 4)
        });

        setScale(scale);
        setPosition({
            std::clamp(
                setting("x"), 0.f,
                std::max(0.f, size.width - totalWidth * scale)
            ),
            std::clamp(
                setting("y"), 0.f,
                std::max(
                    0.f, size.height - (height + width + 4) * scale
                )
            )
        });

        setVisible(enabled("enabled"));
        drawing->clear();

        for (int i = 0; i < 2; ++i) {
            auto& lane = timeline.lanes[i];
            lane.prune(timeline.now, height / speed);

            labels[i]->setVisible(
                i < lanes && enabled("show-counter")
            );

            if (backgrounds[i]) {
                backgrounds[i]->setVisible(
                    i < lanes && enabled("show-image")
                );
            }

            if (i >= lanes) continue;

            float x = i * (width + 10);
            float base = width + 4;
            ccColor4F border = {0.90f, 0.76f, 0.48f, opacity};

            bool hasImage =
                backgrounds[i] && enabled("show-image");

            if (hasImage) {
                auto sprite = backgrounds[i];
                auto imageSize = sprite->getContentSize();

                float fit = std::min(
                    (width - 2) / imageSize.width,
                    (width - 2) / imageSize.height
                );

                sprite->setScale(fit);
                sprite->setPosition({
                    x + width / 2, width / 2
                });
                sprite->setOpacity(
                    static_cast<GLubyte>(255 * opacity)
                );
            } else {
                rectangle(
                    x, 0, width, width,
                    {0.f, 0.f, 0.f, opacity * 0.35f}
                );
            }

            if (lane.down) {
                rectangle(
                    x, 0, width, width,
                    {1.f, 1.f, 1.f, opacity * 0.4f}
                );
            }

            rectangle(x, 0, width, 1, border);
            rectangle(x, width - 1, width, 1, border);
            rectangle(x, 0, 1, width, border);
            rectangle(x + width - 1, 0, 1, width, border);

            for (auto const& hold : lane.holds) {
                float bottom = hold.active ? 0.f :
                    static_cast<float>(
                        (timeline.now - hold.end) * speed
                    );

                float top = static_cast<float>(
                    (timeline.now - hold.start) * speed
                );

                bottom = std::clamp(bottom, 0.f, height);
                top = std::clamp(
                    std::max(top, bottom + 1.f), 0.f, height
                );

                if (top <= bottom) continue;

                for (float y = bottom; y < top; y += 3.f) {
                    float h = std::min(3.f, top - y);

                    float fade = enabled("fade")
                        ? std::clamp(
                            (height - y - h / 2) /
                                (height * 0.25f),
                            0.f, 1.f
                        )
                        : 1.f;

                    rectangle(
                        x, base + y, width, h,
                        {1.f, 1.f, 1.f, opacity * fade}
                    );
                }
            }

            std::string text = enabled("show-cps")
                ? std::to_string(lane.presses.size())
                : std::to_string(lane.count);

            labels[i]->setString(text.c_str());
            labels[i]->setScale(std::min(
                0.38f,
                (width - 4) / std::max(
                    1.f, labels[i]->getContentSize().width
                )
            ));
            labels[i]->setPosition({
                x + width / 2, width / 2
            });
            labels[i]->setOpacity(
                static_cast<GLubyte>(255 * opacity)
            );
        }
    }
};

HoldOverlay* overlay(PlayLayer* layer) {
    return layer
        ? static_cast<HoldOverlay*>(
            layer->getChildByID(OverlayID)
        )
        : nullptr;
}
}

class $modify(HoldPlayLayer, PlayLayer) {
    bool init(
        GJGameLevel* level, bool useReplay, bool dontCreateObjects
    ) {
        if (!PlayLayer::init(
            level, useReplay, dontCreateObjects
        )) {
            return false;
        }

        if (auto hud = HoldOverlay::create()) {
            addChild(hud, 10000);
        }

        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (auto hud = overlay(this)) hud->reset();
    }

    void pauseGame(bool unknown) {
        if (auto hud = overlay(this)) hud->release();
        PlayLayer::pauseGame(unknown);
    }
};

class $modify(HoldPlayerInputs, PlayerObject) {
    void reportJump(bool down) {
        auto play = PlayLayer::get();
        if (!play) return;

        bool player1 = this == play->m_player1;
        if (!player1 && this != play->m_player2) return;

        if (auto hud = overlay(play)) {
            hud->input(down, player1);
        }
    }

    bool pushButton(PlayerButton button) {
        bool result = PlayerObject::pushButton(button);
        if (static_cast<int>(button) == 1) reportJump(true);
        return result;
    }

    bool releaseButton(PlayerButton button) {
        bool result = PlayerObject::releaseButton(button);
        if (static_cast<int>(button) == 1) reportJump(false);
        return result;
    }

    void releaseAllButtons() {
        PlayerObject::releaseAllButtons();
        reportJump(false);
    }
};
