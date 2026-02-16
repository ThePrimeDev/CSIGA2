#pragma once

#include <cstdint>
#include <array>
#include <utility>

namespace memory {

enum class Bones : uint64_t {
    Hip = 0,
    Spine1 = 1,
    Spine2 = 2,
    Spine3 = 3,
    Spine4 = 4,
    Neck = 5,
    Head = 6,
    LeftShoulder = 8,
    LeftElbow = 9,
    LeftHand = 10,
    RightShoulder = 13,
    RightElbow = 14,
    RightHand = 15,
    LeftHip = 22,
    LeftKnee = 23,
    LeftFoot = 24,
    RightHip = 25,
    RightKnee = 26,
    RightFoot = 27
};

// Bone connections for skeleton rendering
constexpr std::array<std::pair<Bones, Bones>, 18> BONE_CONNECTIONS = {{
    // Spine
    {Bones::Hip, Bones::Spine1},
    {Bones::Spine1, Bones::Spine2},
    {Bones::Spine2, Bones::Spine3},
    {Bones::Spine3, Bones::Spine4},
    {Bones::Spine4, Bones::Neck},
    {Bones::Neck, Bones::Head},
    // Left arm
    {Bones::Neck, Bones::LeftShoulder},
    {Bones::LeftShoulder, Bones::LeftElbow},
    {Bones::LeftElbow, Bones::LeftHand},
    // Right arm
    {Bones::Neck, Bones::RightShoulder},
    {Bones::RightShoulder, Bones::RightElbow},
    {Bones::RightElbow, Bones::RightHand},
    // Left leg
    {Bones::Hip, Bones::LeftHip},
    {Bones::LeftHip, Bones::LeftKnee},
    {Bones::LeftKnee, Bones::LeftFoot},
    // Right leg
    {Bones::Hip, Bones::RightHip},
    {Bones::RightHip, Bones::RightKnee},
    {Bones::RightKnee, Bones::RightFoot}
}};

// All bones for iteration
constexpr std::array<Bones, 19> ALL_BONES = {
    Bones::Hip, Bones::Spine1, Bones::Spine2, Bones::Spine3, Bones::Spine4,
    Bones::Neck, Bones::Head, Bones::LeftShoulder, Bones::LeftElbow, Bones::LeftHand,
    Bones::RightShoulder, Bones::RightElbow, Bones::RightHand, Bones::LeftHip,
    Bones::LeftKnee, Bones::LeftFoot, Bones::RightHip, Bones::RightKnee, Bones::RightFoot
};

inline uint64_t boneToIndex(Bones bone) {
    return static_cast<uint64_t>(bone);
}

} // namespace memory
