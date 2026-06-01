#pragma once

#include <cstdint>

namespace blocks {

constexpr uint8_t kAir = 0;
constexpr uint8_t kStone = 1;
constexpr uint8_t kDirt = 2;
constexpr uint8_t kGrass = 3;
constexpr uint8_t kCobblestone = 4;

inline bool isSolid(uint8_t id) { return id != kAir; }

} // namespace blocks
