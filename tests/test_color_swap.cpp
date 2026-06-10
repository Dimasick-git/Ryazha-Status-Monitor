// Tests for the COLOR{} pseudo-function that nibble-swaps 16-bit values
// (so .smd authors can write RGBA-ordered constants and the hardware
// receives them as ABGR).
//
// Coverage:
//   - Literal form `COLOR{0xHHHH}` rewritten at parse time
//   - Expression form `COLOR{expr}` evaluated each frame
//   - Used in config values and inline render-command args
//   - Edge cases: 0x0000, 0xFFFF (palindromes -- value unchanged),
//     0xABCD -> 0xDCBA

#include "test_support.hpp"

#include <cstdio>
#include <cstdint>
#include <cstring>

using namespace test_support;

namespace {

int g_failures = 0;

struct CapColor {
    uint16_t lastTextColor = 0;
    uint16_t lastBoxColor  = 0;
};

void Cb(smd::RenderCommand& cmd, void* user) {
    auto* c = static_cast<CapColor*>(user);
    if (cmd.type == smd::RenderCmdType::Text) c->lastTextColor = cmd.color;
    else if (cmd.type == smd::RenderCmdType::Box) c->lastBoxColor = cmd.color;
    else if (cmd.type == smd::RenderCmdType::GetDimensions && cmd.outDims) {
        cmd.outDims->x = (int64_t)cmd.text.size() * (cmd.fontSize / 2);
        cmd.outDims->y = cmd.fontSize;
    }
}

void Expect16(uint16_t actual, uint16_t expected, const char* label) {
    if (actual == expected)
        std::printf("  PASS %s: 0x%04X\n", label, actual);
    else {
        std::printf("  FAIL %s: got 0x%04X, expected 0x%04X\n",
            label, actual, expected);
        g_failures++;
    }
}

} // namespace

int main() {
    // ---- 1. Literal config form ----
    std::printf("[literal in config values]\n");
    {
        const char* smd =
            "Name = T\n"
            "C1: COLOR{0x0000}\n"
            "C2: COLOR{0xFFFF}\n"
            "C3: COLOR{0xABCD}\n"
            "C4: COLOR{0x1234}\n"
            "Start:\nTEXT{0,0,18,0xFFFF,true,\"x\"}\n";
        smd::Document doc;
        if (!doc.LoadFromMemory(smd, std::strlen(smd))) {
            std::printf("FAIL Load: %s\n", doc.LastError()); return 1;
        }
        Expect16((uint16_t)doc.GetConfigInt("C1", 1), 0x0000, "0x0000");
        Expect16((uint16_t)doc.GetConfigInt("C2", 0), 0xFFFF, "0xFFFF (palindrome)");
        Expect16((uint16_t)doc.GetConfigInt("C3", 0), 0xDCBA, "0xABCD -> 0xDCBA");
        Expect16((uint16_t)doc.GetConfigInt("C4", 0), 0x4321, "0x1234 -> 0x4321");
    }

    // ---- 2. Literal in inline render-command arg ----
    std::printf("\n[literal in inline render-command args]\n");
    {
        const char* smd =
            "Name = T\n"
            "Start:\n"
            "TEXT{0, 0, 18, COLOR{0xF00F}, true, \"x\"}\n"   // 0xF00F (palindrome)
            "BOX{0, 0, 10, 10, COLOR{0x12AB}}\n";       // -> 0xBA21
        smd::Document doc;
        doc.LoadFromMemory(smd, std::strlen(smd));
        if (!doc.Compile()) {
            std::printf("FAIL Compile: %s\n", doc.LastError()); return 1;
        }
        CapColor cap;
        doc.Evaluate(Cb, &cap);
        Expect16(cap.lastTextColor, 0xF00F, "TEXT COLOR{0xF00F}");
        Expect16(cap.lastBoxColor,  0xBA21, "BOX COLOR{0x12AB}");
    }

    // ---- 3. Expression form ----
    std::printf("\n[expression form]\n");
    {
        const char* smd =
            "Name = T\n"
            "base: 0x1230\n"
            "Start:\n"
            "TEXT{0, 0, 18, COLOR{base + 4}, true, \"x\"}\n";   // base+4 = 0x1234 -> 0x4321
        smd::Document doc;
        doc.LoadFromMemory(smd, std::strlen(smd));
        if (!doc.Compile()) {
            std::printf("FAIL Compile: %s\n", doc.LastError()); return 1;
        }
        CapColor cap;
        doc.Evaluate(Cb, &cap);
        Expect16(cap.lastTextColor, 0x4321, "COLOR{base + 4}");
    }

    // ---- 4. New defaults: HeaderText / FooterText / UseCustomExitCombo ----
    std::printf("\n[new config defaults]\n");
    {
        smd::Document doc;
        if (doc.GetConfigBool("HeaderText", false) != true) {
            std::printf("  FAIL HeaderText default\n"); g_failures++;
        } else std::printf("  PASS HeaderText=true\n");
        if (doc.GetConfigBool("FooterText", false) != true) {
            std::printf("  FAIL FooterText default\n"); g_failures++;
        } else std::printf("  PASS FooterText=true\n");
        if (doc.GetConfigBool("UseCustomExitCombo", false) != false) {
            std::printf("  FAIL UseCustomExitCombo default\n"); g_failures++;
        } else std::printf("  PASS UseCustomExitCombo=true\n");
    }

    std::printf("\n%d failures total\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
