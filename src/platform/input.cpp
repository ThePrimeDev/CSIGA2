#include "input.h"

#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <cstring>
#include <cstdio>
#include <sys/ioctl.h>
#include <cmath>
#include <memory>

namespace platform {

namespace {
    static std::unique_ptr<Mouse> g_mouse_instance;
    static std::unique_ptr<Keyboard> g_keyboard_instance;
}

Mouse::Mouse() {
    // Open uinput device
    fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_ < 0) {
        perror("Failed to open /dev/uinput");
        return;
    }

    // Configure device
    ioctl(fd_, UI_SET_EVBIT, EV_KEY);
    ioctl(fd_, UI_SET_KEYBIT, BTN_LEFT);
    
    ioctl(fd_, UI_SET_EVBIT, EV_REL);
    ioctl(fd_, UI_SET_RELBIT, REL_X);
    ioctl(fd_, UI_SET_RELBIT, REL_Y);

    // Setup device structure
    struct uinput_setup usetup;
    std::memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x0451; // Texas Instruments
    usetup.id.product = 0xe008; // TI-84 Silver (from Rust implementation)
    std::strcpy(usetup.name, "TI-84 Plus Silver Calculator");

    ioctl(fd_, UI_DEV_SETUP, &usetup);
    ioctl(fd_, UI_DEV_CREATE);
}

Mouse::~Mouse() {
    if (fd_ >= 0) {
        ioctl(fd_, UI_DEV_DESTROY);
        close(fd_);
        fd_ = -1;
    }
}

Mouse::Mouse(Mouse&& other) noexcept {
    fd_ = other.fd_;
    other.fd_ = -1;
}

Mouse& Mouse::operator=(Mouse&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) {
            ioctl(fd_, UI_DEV_DESTROY);
            close(fd_);
        }
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

Mouse* Mouse::Get() {
    if (!g_mouse_instance) {
        g_mouse_instance = std::make_unique<Mouse>();
    }
    return g_mouse_instance.get();
}

void Mouse::MoveRel(const Vec2& coords) {
    if (fd_ < 0) return;

    struct input_event ie[2];
    std::memset(ie, 0, sizeof(ie));

    ie[0].type = EV_REL;
    ie[0].code = REL_X;
    ie[0].value = static_cast<int32_t>(coords.x);

    ie[1].type = EV_REL;
    ie[1].code = REL_Y;
    ie[1].value = static_cast<int32_t>(coords.y);

    write(fd_, ie, sizeof(ie));

    struct input_event syn;
    std::memset(&syn, 0, sizeof(syn));
    syn.type = EV_SYN;
    syn.code = SYN_REPORT;
    syn.value = 0;
    write(fd_, &syn, sizeof(syn));
}

void Mouse::LeftPress() {
    SendKey(1);
}

void Mouse::LeftRelease() {
    SendKey(0);
}

void Mouse::SendKey(int32_t pressed) {
    if (fd_ < 0) return;

    struct input_event ie;
    std::memset(&ie, 0, sizeof(ie));

    ie.type = EV_KEY;
    ie.code = BTN_LEFT;
    ie.value = pressed;

    write(fd_, &ie, sizeof(ie));

    struct input_event syn;
    std::memset(&syn, 0, sizeof(syn));
    syn.type = EV_SYN;
    syn.code = SYN_REPORT;
    syn.value = 0;
    write(fd_, &syn, sizeof(syn));
}

} // namespace platform

namespace platform {

Keyboard::Keyboard() {
    fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_ < 0) {
        perror("Failed to open /dev/uinput");
        return;
    }

    ioctl(fd_, UI_SET_EVBIT, EV_KEY);
    ioctl(fd_, UI_SET_KEYBIT, KEY_GRAVE);

    struct uinput_setup usetup;
    std::memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1209;
    usetup.id.product = 0x0001;
    std::strcpy(usetup.name, "CSIGA2 Virtual Keyboard");

    ioctl(fd_, UI_DEV_SETUP, &usetup);
    ioctl(fd_, UI_DEV_CREATE);
}

Keyboard::~Keyboard() {
    if (fd_ >= 0) {
        ioctl(fd_, UI_DEV_DESTROY);
        close(fd_);
        fd_ = -1;
    }
}

Keyboard::Keyboard(Keyboard&& other) noexcept {
    fd_ = other.fd_;
    other.fd_ = -1;
}

Keyboard& Keyboard::operator=(Keyboard&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) {
            ioctl(fd_, UI_DEV_DESTROY);
            close(fd_);
        }
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

Keyboard* Keyboard::Get() {
    if (!g_keyboard_instance) {
        g_keyboard_instance = std::make_unique<Keyboard>();
    }
    return g_keyboard_instance.get();
}

void Keyboard::GravePress() {
    SendKey(1, KEY_GRAVE);
}

void Keyboard::GraveRelease() {
    SendKey(0, KEY_GRAVE);
}

void Keyboard::SendKey(int32_t pressed, int key_code) {
    if (fd_ < 0) return;

    struct input_event ie;
    std::memset(&ie, 0, sizeof(ie));
    ie.type = EV_KEY;
    ie.code = key_code;
    ie.value = pressed;
    write(fd_, &ie, sizeof(ie));

    struct input_event syn;
    std::memset(&syn, 0, sizeof(syn));
    syn.type = EV_SYN;
    syn.code = SYN_REPORT;
    syn.value = 0;
    write(fd_, &syn, sizeof(syn));
}

} // namespace platform

namespace platform {

InputReader::InputReader() {
    char path[64];
    char name[256];
    for (int i = 0; i < 32; ++i) {
        std::snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        // Skip our own virtual devices
        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
            std::string dev_name(name);
            if (dev_name.find("TI-84") != std::string::npos ||
                dev_name.find("CSIGA2") != std::string::npos) {
                close(fd);
                continue;
            }
        }

        // Check if this device supports EV_KEY
        unsigned long evbits[(EV_MAX + 7) / 8] = {};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits) < 0) {
            close(fd);
            continue;
        }
        if (!((evbits[EV_KEY / 8] >> (EV_KEY % 8)) & 1)) {
            close(fd);
            continue;
        }

        fds_[fd_count_++] = fd;
        if (fd_count_ >= kMaxDevices) break;
    }
}

InputReader::~InputReader() {
    for (int i = 0; i < fd_count_; ++i) {
        close(fds_[i]);
    }
}

InputReader& InputReader::Get() {
    static InputReader instance;
    return instance;
}

bool InputReader::IsMouseButtonDown(int button) const {
    return IsAnyDeviceKeyDown(button);
}

bool InputReader::IsKeyDown(int key) const {
    return IsAnyDeviceKeyDown(key);
}

bool InputReader::IsAnyDeviceKeyDown(int code) const {
    for (int i = 0; i < fd_count_; ++i) {
        unsigned char keys[(KEY_MAX + 7) / 8] = {};
        if (ioctl(fds_[i], EVIOCGKEY(sizeof(keys)), keys) < 0) continue;
        if ((keys[code / 8] >> (code % 8)) & 1) return true;
    }
    return false;
}

} // namespace platform
