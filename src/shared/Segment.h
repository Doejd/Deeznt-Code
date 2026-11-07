#ifndef SEGMENT_HPP_
#define SEGMENT_HPP_
#include <cstdint>

struct Segment {
    uint32_t color{0xffffff};
    uint32_t bg_color{0x000000};
    uint32_t length{0};
    int32_t starting_column{0};
    bool hasBg{false};
    bool bold{false};
};
#endif