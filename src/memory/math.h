#pragma once

#include <cmath>
#include <array>

namespace memory {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
    Vec2 operator/(float scalar) const { return Vec2(x / scalar, y / scalar); }
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    Vec2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }

    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSquared() const { return x * x + y * y; }

    static const Vec2 ZERO;
};

inline const Vec2 Vec2::ZERO = Vec2(0.0f, 0.0f);

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    Vec3 operator/(float scalar) const { return Vec3(x / scalar, y / scalar, z / scalar); }

    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float lengthSquared() const { return x * x + y * y + z * z; }

    float distance(const Vec3& other) const {
        return (*this - other).length();
    }

    Vec3 normalize() const {
        float len = length();
        if (len == 0.0f) return Vec3(0, 0, 0);
        return Vec3(x / len, y / len, z / len);
    }

    static const Vec3 ZERO;
};

inline const Vec3 Vec3::ZERO = Vec3(0.0f, 0.0f, 0.0f);

struct IVec2 {
    int32_t x = 0;
    int32_t y = 0;

    Vec2 asVec2() const { return Vec2(static_cast<float>(x), static_cast<float>(y)); }
};

struct Mat4 {
    std::array<float, 4> x_axis;
    std::array<float, 4> y_axis;
    std::array<float, 4> z_axis;
    std::array<float, 4> w_axis;

    Mat4() : x_axis{0}, y_axis{0}, z_axis{0}, w_axis{0} {}
};

// Convert a forward vector to pitch/yaw angles
inline Vec2 anglesFromVector(const Vec3& forward) {
    float yaw, pitch;

    if (forward.x == 0.0f && forward.y == 0.0f) {
        yaw = 0.0f;
        pitch = (forward.z > 0.0f) ? 270.0f : 90.0f;
    } else {
        yaw = std::atan2(forward.y, forward.x) * (180.0f / 3.14159265358979323846f);
        if (yaw < 0.0f) {
            yaw += 360.0f;
        }

        float xyLength = Vec2(forward.x, forward.y).length();
        pitch = std::atan2(-forward.z, xyLength) * (180.0f / 3.14159265358979323846f);
        if (pitch < 0.0f) {
            pitch += 360.0f;
        }
    }

    return Vec2(pitch, yaw);
}

// Calculate the FOV (angle difference) between two angle pairs
inline float anglesToFov(const Vec2& viewAngles, const Vec2& aimAngles) {
    Vec2 delta = viewAngles - aimAngles;

    if (delta.x > 180.0f) {
        delta.x = 360.0f - delta.x;
    }
    delta.x = std::abs(delta.x);

    // Normalize yaw
    delta.y = std::fmod(delta.y + 180.0f, 360.0f) - 180.0f;
    delta.y = std::abs(delta.y);

    return delta.length();
}

// Clamp view angles to valid ranges
inline void vec2Clamp(Vec2& vec) {
    if (vec.x > 89.0f && vec.x <= 180.0f) {
        vec.x = 89.0f;
    }
    if (vec.x > 180.0f) {
        vec.x -= 360.0f;
    }
    if (vec.x < -89.0f) {
        vec.x = -89.0f;
    }
    vec.y = std::fmod(vec.y + 180.0f, 360.0f) - 180.0f;
}

} // namespace memory
