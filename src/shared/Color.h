#ifndef GODOT_TERMINAL_COLOR_H
#define GODOT_TERMINAL_COLOR_H
#include <cmath>
#include "Segment.h"

namespace color {
    inline uint32_t darkenColor(const uint32_t hex, const float darkeningAmount) {
        const uint32_t r = (hex >> 16) & 0xff;
        const uint32_t g = (hex >> 8) & 0xff;
        const uint32_t b = hex & 0xff;

        const int darkR = std::floor(static_cast<float>(r) / darkeningAmount);
        const int darkG = std::floor(static_cast<float>(g)/ darkeningAmount);
        const int darkB = std::floor(static_cast<float>(b) / darkeningAmount);


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


namespace AnsiParseHelpers {
    inline void applyAnsiColorCode(const int code, Segment &seg){
        switch(code){
            case 0:
                seg.bold = false;
                seg.hasBg = false;
                seg.bg_color = 0x000000;
                seg.color = 0xffffff;
                break;

            case 1: seg.bold = true; break;
            case 22: seg.bold = false; break;

            case 30 ... 37: seg.color = color::ansiToColor(code - 30); break;
            case 40 ... 47: seg.bg_color = color::ansiToColor(code - 40); seg.hasBg = true; break;
            case 90 ... 97: seg.color = color::ansiToColor(code - 90 + 8); break;
            case 100 ... 107: seg.bg_color = color::ansiToColor(code - 100 + 8); seg.hasBg = true; break;
            default: break;
        }
    }


    inline void applyAnsiColorMode2(std::array<short, 3> rgb, bool isFG, Segment &seg) {
        if (isFG) seg.color = rgb[0] << 16 | rgb[1] << 8 | rgb[2];
        else {
            seg.bg_color = rgb[0] << 16 | rgb[1] << 8 | rgb[2];
            seg.hasBg = true;
        }
    }

    inline void applyAnsiColorMode5(const int code, bool isFG, Segment &seg) {
        const int rgb = color::ansi256ToColor(code);

        if (isFG) seg.color = rgb;
        else {
            seg.bg_color = rgb;
            seg.hasBg = true;
        }

    }

    inline void parseAndApplyArgs(Segment &seg, const godot::String &args) {
        auto params = args.split(";");
        for (ssize_t i = 0; i < params.size();){
            const int code = static_cast<int>(params[i].to_int());

            if (code == 38 || code == 48){
                const bool isFG = code == 38;
                const int mode = static_cast<int>(params[i+1].to_int());
                if (mode == 5){

                    applyAnsiColorMode5(static_cast<int>(params[i+2].to_int()), isFG, seg);

                    i += 3;
                    continue;

                }
                if (mode == 2) {
                    const auto r = static_cast<short>(params[i+2].to_int());
                    const auto g = static_cast<short>(params[i+3].to_int());
                    const auto b = static_cast<short>(params[i+4].to_int());

                    applyAnsiColorMode2({r, g, b}, isFG, seg);

                    i += 5;
                    continue;
                }
                i++;
                continue;
            }
            applyAnsiColorCode(code, seg);
            i++;
        }
        if (!seg.bold) seg.color = color::darkenColor(seg.color, 1.15);
        if (seg.hasBg && !seg.bold) seg.bg_color = color::darkenColor(seg.bg_color, 1.15);
    }

}

#endif //GODOT_TERMINAL_COLOR_H
