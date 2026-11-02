#ifndef GODOT_TERMINAL_COLOR_H
#define GODOT_TERMINAL_COLOR_H
#include <cmath>
#include <cstdint>

namespace color {
    inline uint32_t darkenColor(const uint32_t hex) {
        const uint32_t r = (hex >> 16) & 0xff;
        const uint32_t g = (hex >> 8) & 0xff;
        const uint32_t b = hex & 0xff;

        const int darkR = std::floor(r / 1.15);
        const int darkG = std::floor(g / 1.15);
        const int darkB = std::floor(b / 1.15);


        return static_cast<uint32_t>(darkR << 16 | darkG << 8 | darkB);
    }

    inline int32_t ansiToColor(const int32_t code) {
        switch (code) {
            case 0: return 0x000000;     // black
            case 1: return 0xff0000;     // red
            case 2: return 0x00ff00;     // green
            case 3: return 0xffff00;     // yellow
            case 4: return 0x0000ff;     // blue
            case 5: return 0xff00ff;     // magenta
            case 6: return 0x00ffff;     // cyan
            case 7: return 0xffffff;     // white
            case 8: return 0x888888;     // bright black / dark gray
            case 9: return 0xff8888;     // bright red
            case 10: return 0x88ff88;    // bright green
            case 11: return 0xffff88;    // bright yellow
            case 12: return 0x8888ff;    // bright blue
            case 13: return 0xff88ff;    // bright magenta
            case 14: return 0x88ffff;    // bright cyan
            default: return 0xffffff;    // case 15 included
        }
    }

    inline int32_t ansi256ToColor(const int32_t code) {
        if (code < 16) return ansiToColor(code);
        if (code <= 231) {
            const int idx = code - 16;

            const int r = idx / 36;
            const int g = (idx % 36) / 6;
            const int b = idx % 6;

            const int steps[6] = {0, 95, 135, 175, 215, 255};

            return steps[r] << 16 | steps[g] << 8 | steps[b];

        }
        const int gray = 8 + (code - 232) * 10;
        return gray << 16 | gray << 8 | gray;
    }

}


#endif //GODOT_TERMINAL_COLOR_H
