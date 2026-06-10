#include "smd_parser.hpp"
#include <cstdio>

#include <cstring>
#include <string>
static int g_seen = 0;
static void Cb(smd::RenderCommand& cmd, void*) {
    g_seen++;
    if (cmd.type == smd::RenderCmdType::GraphLineChart) {
        // Verify all sample/cond pointers are non-null when count > 0
        if (cmd.sampleCount > 0 && !cmd.samples) std::abort();
        if (cmd.condCount > 0 && !cmd.conditions) std::abort();
    }
}

int main() {
    const char* smd =
        "Name = T\n"
        "hist: HISTORY{int, 32}\n"
        "conds: GRAPH_CONDITIONS{{x < 30, 0xF00F, 0xF005}, {x >= 30, 0x0F0F, 0x0F05}}\n"
        "Start:\n"
        "HISTORY_UPDATE{hist, 25}\n"
        "HISTORY_UPDATE{hist, 50}\n"
        "GRAPH_LINE_CHART{0, 0, 100, 60, 0, 100, LEFT_TO_RIGHT, 0xFFFF, 0xFFF8, conds, hist}\n";
    smd::Document doc;
    if (!doc.LoadFromMemory(smd, std::strlen(smd))) {
        std::printf("Load: %s\n", doc.LastError()); return 1;
    }
    if (!doc.Compile()) {
        std::printf("Compile: %s\n", doc.LastError()); return 1;
    }
    // Run 100 frames to fill / wrap the ring
    for (int f = 0; f < 100; ++f) {
        if (!doc.Evaluate(Cb, nullptr)) return 1;
    }
    // Reset shouldn't break anything
    doc.Reset();
    if (!doc.Evaluate(Cb, nullptr)) return 1;
    // Free + re-Load + Compile + Eval
    std::string s(smd);
    doc.LoadFromMemory(s.data(), s.size());
    doc.Compile();
    doc.Evaluate(Cb, nullptr);
    std::printf("OK: %d render cmds seen\n", g_seen);
    return 0;
}
