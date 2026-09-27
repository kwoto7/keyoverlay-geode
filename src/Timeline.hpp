#pragma once
#include <array>
#include <deque>
#include <cstdint>

struct Hold { double start; double end; bool active; };
struct Lane {
    bool down = false;
    std::uint64_t count = 0;
    std::deque<Hold> holds;
    std::deque<double> presses;
    void input(bool pressed, double now) {
        if (pressed == down) return;
        down = pressed;
        if (pressed) {
            ++count;
            holds.push_back({now, now, true});
            presses.push_back(now);
            if (holds.size() > 2048) holds.pop_front();
            if (presses.size() > 2048) presses.pop_front();
        } else if (!holds.empty() && holds.back().active) {
            holds.back().end = now;
            holds.back().active = false;
        }
    }
    void prune(double now, double history) {
        while (!holds.empty() && !holds.front().active && now-holds.front().end > history)
            holds.pop_front();
        while (!presses.empty() && now-presses.front() >= 1.0) presses.pop_front();
    }
};
struct Timeline {
    double now = 0;
    std::array<Lane, 2> lanes;
    void reset() { now = 0; lanes = {}; }
    void release() { for (auto& lane : lanes) lane.input(false, now); }
};
