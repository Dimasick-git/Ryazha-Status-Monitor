// Smoke test: drives PreprocessExpr from inside the parser by parsing
// a tiny .smd snippet and inspecting the produced expression source.
#include "smd_parser.hpp"
#include <cstdio>
#include <cstring>

int main() {
    const char* smd =
        "Name = T\n"
        "EnableGame: true\n"
        "Start:\n"
        "#if $Game_ResolutionRenderCalls_int[0].calls != 0xFFFF\n"
        "TEXT{Game_ResolutionRenderCalls_int[0].width, Game_ResolutionViewportCalls_int[3].height, 15, 0xFFFF, true, \"x\"}\n"
        "#endif\n";
    smd::Document doc;
    if (!doc.LoadFromMemory(smd, std::strlen(smd))) {
        std::printf("Load failed: %s\n", doc.LastError());
        return 1;
    }
    // Compile will fail because we haven't bound anything, but it should not
    // fail with a tinyexpr parse error from leftover `[` characters.
    // Bind dummies so Compile finds the names.
    int64_t a = 1920, b = 1080, c = 100;
    doc.BindInt64("Game_ResolutionRenderCalls_int_0_width",  &a);
    doc.BindInt64("Game_ResolutionRenderCalls_int_0_height", &b);
    doc.BindInt64("Game_ResolutionRenderCalls_int_0_calls",  &c);
    doc.BindInt64("Game_ResolutionViewportCalls_int_3_height", &b);
    if (!doc.Compile()) {
        std::printf("Compile failed: %s\n", doc.LastError());
        return 2;
    }
    std::printf("Phase 1 OK: array indexing mangled correctly.\n");
    return 0;
}
