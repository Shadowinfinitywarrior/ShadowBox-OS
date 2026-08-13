#pragma once

#include <cstdint>

// 16×16 standard arrow cursor (1 = fg, 0 = transparent)
// Stored bottom-up so that the tip is at (0,0).
namespace CursorBitmap {
static constexpr int W = 16;
static constexpr int H = 16;
static constexpr uint32_t FG = 0xFF000000u; // black
static constexpr uint32_t BG = 0xFFFFFFFFu; // white (optional outline)

// Each row is 16 bits: LSB = leftmost pixel.
static const uint16_t rows[H] = {
    0b0000000000000000u,
    0b0000000001000000u,
    0b0000000001100000u,
    0b0000000001110000u,
    0b0000000011111000u,
    0b0000000011111000u,
    0b0000000011111000u,
    0b0000000011111000u,
    0b0000000001110000u,
    0b0000000001100000u,
    0b0000000001100000u,
    0b0000000001100000u,
    0b0000000000000000u,
    0b0000000000000000u,
    0b0000000000000000u,
    0b0000000000000000u,
};

inline bool pixel(int x, int y) {
    if (x < 0 || x >= W || y < 0 || y >= H) return false;
    return (rows[y] >> (15 - x)) & 1;
}
}
