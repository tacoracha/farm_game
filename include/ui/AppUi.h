#pragma once

#include "farm/Game.h"

#include <deque>
#include <string>

namespace farm::ui {

// Owns the simulation Game instance and a bounded log for the desktop UI.
// Logic stays in farm::*; this class only orchestrates ImGui panels.
class AppUi {
public:
    AppUi() = default;

    Game& game() { return game_; }
    const Game& game() const { return game_; }

    const std::deque<std::string>& log_lines() const { return log_lines_; }

    void push_log(std::string line);
    void render_frame();

    // Auto tick is UI-only: accumulates real time and calls Game::AdvanceTick().
    void update_auto_tick(float delta_seconds);
    void set_auto_tick_running(bool running);
    bool auto_tick_running() const { return auto_tick_running_; }
    void set_auto_tick_speed(int multiplier);
    int auto_tick_speed() const { return auto_tick_speed_; }

private:
    Game game_;
    std::deque<std::string> log_lines_;
    static constexpr std::size_t kMaxLogLines = 200;

    bool auto_tick_running_ = false;
    int auto_tick_speed_ = 1;
    float auto_tick_accumulator_ = 0.0F;
};

}  // namespace farm::ui
