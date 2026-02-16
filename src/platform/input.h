#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace platform {

struct Vec2 {
    float x;
    float y;
};

class Mouse {
public:
    Mouse();
    ~Mouse();

    // Prevent copying
    Mouse(const Mouse&) = delete;
    Mouse& operator=(const Mouse&) = delete;

    // Allow moving
    Mouse(Mouse&& other) noexcept;
    Mouse& operator=(Mouse&& other) noexcept;

    // Static method to create/open the mouse device
    static Mouse* Get();

    // Mouse operations
    void MoveRel(const Vec2& coords);
    void LeftPress();
    void LeftRelease();
    
    bool IsValid() const { return fd_ >= 0; }

private:
    void SendKey(int32_t pressed);
    
    int fd_ = -1;
};

class Keyboard {
public:
    Keyboard();
    ~Keyboard();

    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;

    Keyboard(Keyboard&& other) noexcept;
    Keyboard& operator=(Keyboard&& other) noexcept;

    static Keyboard* Get();

    void GravePress();
    void GraveRelease();

    bool IsValid() const { return fd_ >= 0; }

private:
    void SendKey(int32_t pressed, int key_code);

    int fd_ = -1;
};

// Reads raw button state from physical input devices via evdev.
// Opens ALL /dev/input/event* devices (skipping our virtual ones)
// and checks button state across all of them.
class InputReader {
public:
    InputReader();
    ~InputReader();

    InputReader(const InputReader&) = delete;
    InputReader& operator=(const InputReader&) = delete;

    static InputReader& Get();

    // Check if a mouse button is currently held.
    // button: linux evdev code (BTN_LEFT, BTN_EXTRA, etc.)
    bool IsMouseButtonDown(int button) const;

    // Check if a keyboard key is currently held.
    // key: linux evdev code (KEY_SPACE, KEY_C, etc.)
    bool IsKeyDown(int key) const;

private:
    bool IsAnyDeviceKeyDown(int code) const;

    static constexpr int kMaxDevices = 32;
    int fds_[kMaxDevices] = {};
    int fd_count_ = 0;
};

} // namespace platform
