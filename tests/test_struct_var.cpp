#include "smd_parser.hpp"
#include <cstdio>
#include <cstring>

int main() {
    const char* smd =
        "Name = T\n"
        "Start:\n"
        "VAR{R, Game_ResolutionRenderCalls_int[0]}\n"
        // Test 1: read R sub-fields via tinyexpr (no nestedFmt indirection)
        "VAR{x_width, R.width}\n"
        "VAR{x_height, R.height}\n"
        "VAR{Out, {\"%dx%d\", x_width, x_height}}\n"
        "TEXT{0, 0, 18, 0xFFFF, true, Out}\n";
    smd::Document doc;
    if (!doc.LoadFromMemory(smd, std::strlen(smd))) { std::printf("Load: %s\n", doc.LastError()); return 1; }
    smd::ResolutionEntry r[8] = {};
    r[0] = { 1920, 1080, 100 };
    doc.BindResolutionArray("Game_ResolutionRenderCalls_int", r);
    if (!doc.Compile()) { std::printf("Compile: %s\n", doc.LastError()); return 1; }
    auto cb = +[](smd::RenderCommand& cmd, void*) {
        if (cmd.type == smd::RenderCmdType::Text) std::printf("TEXT: '%s'\n", cmd.text.c_str());
    };
    doc.Evaluate(cb, nullptr);
}
