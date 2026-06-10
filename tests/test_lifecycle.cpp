#include "smd_parser.hpp"
#include <cstdio>
#include <cstring>

int main() {
    const char* smd =
        "Name = T\n"
        "EnableGame: true\n"
        "Start:\n"
        "VAR{R, Game_ResolutionRenderCalls_int[0]}\n"
        "VAR{Out, {\"%dx%d_%d\", R.width, R.height, R.calls}}\n"
        "TEXT{0, 0, 18, 0xFFFF, true, Out}\n";
    smd::Document doc;
    if (!doc.LoadFromMemory(smd, std::strlen(smd))) {
        std::printf("Load fail: %s\n", doc.LastError()); return 1;
    }
    smd::ResolutionEntry r[8] = {};
    r[0] = { 1920, 1080, 100 };
    doc.BindResolutionArray("Game_ResolutionRenderCalls_int", r);
    if (!doc.Compile()) { std::printf("Compile fail: %s\n", doc.LastError()); return 1; }
    auto cb = +[](smd::RenderCommand& cmd, void*) {
        if (cmd.type == smd::RenderCmdType::Text) std::printf("  '%s'\n", cmd.text.c_str());
    };
    // Frame 1
    std::printf("Frame 1 (1920x1080, 100):\n");
    doc.Evaluate(cb, nullptr);
    // Frame 2: change values
    r[0] = { 1280, 720, 60 };
    std::printf("Frame 2 (1280x720, 60):\n");
    doc.Evaluate(cb, nullptr);
    // Frame 3: change again
    r[0] = { 800, 600, 30 };
    doc.Reset();
    std::printf("Frame 3 (800x600, 30) post-Reset:\n");
    doc.Evaluate(cb, nullptr);
    // Defaults survive Free?
    std::printf("LayerWidth before Free: %lld\n", (long long)doc.GetConfigInt("LayerWidth", 0));
    doc.Free();
    std::printf("LayerWidth after Free:  %lld\n", (long long)doc.GetConfigInt("LayerWidth", 0));
    std::printf("Movable after Free:     %d\n", (int)doc.GetConfigBool("Movable", true));
    std::printf("COMMON_MARGIN after Free: %lld\n", (long long)doc.GetConfigInt("COMMON_MARGIN", 0));
    return 0;
}
