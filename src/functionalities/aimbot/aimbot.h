#pragma once

#include <array>
#include <cstdint>
#include "../../memory/math.h"

namespace functionalities {

class Aimbot {
public:
    static Aimbot& Get();

    void Update();
    bool isActive() const { return active_; }

private:
    bool previous_button_state_ = false;
    bool active_ = false;
    float previous_aim_punch_x_ = 0.0f;
    float previous_aim_punch_y_ = 0.0f;

    // Target locking
    uint64_t locked_pawn_ = 0;

    // Fractional mouse accumulation
    float mouse_remainder_x_ = 0.0f;
    float mouse_remainder_y_ = 0.0f;
};

enum class AimbotKey : uint64_t {
    Num0 = 1,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Space = 66,
    Backspace,
    Tab,
    Escape = 71,
    Insert = 73,
    Delete,
    Home,
    End,
    MouseLeft = 317,
    MouseRight,
    MouseMiddle,
    Mouse4,
    Mouse5,
    MouseWheelUp,
    MouseWheelDown,
};

struct AimbotHotkeyOption {
    const char* label;
    AimbotKey key;
};

constexpr std::array<AimbotHotkeyOption, 10> kAimbotHotkeys = {{
    {"Mouse4", AimbotKey::Mouse4},
    {"Mouse5", AimbotKey::Mouse5},
    {"Space", AimbotKey::Space},
    {"C", AimbotKey::C},
    {"V", AimbotKey::V},
    {"X", AimbotKey::X},
    {"Z", AimbotKey::Z},
    {"F", AimbotKey::F},
    {"E", AimbotKey::E},
    {"Q", AimbotKey::Q},
}};

} // namespace functionalities
