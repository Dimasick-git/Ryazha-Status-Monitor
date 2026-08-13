#include "smd_parser.hpp"
#include <cstdio>
#include <string>

static void NoOpCb(smd::RenderCommand&, void*) {}

int main() {
    // Generate a file with 50 string VARs each having a nested fmt
    std::string smd = "Name = T\nEnableGame: true\nStart:\n";
    for (int i = 0; i < 50; ++i) {
        char tmp[256];
        std::snprintf(tmp, sizeof(tmp),
            "VAR{x%d, \"\"}\n"
            "VAR{x%d, x%d + {\"v=%%d\", %d}}\n", i, i, i, i*10);
        smd += tmp;
    }
    smd += "TEXT{0,0,18,0xFFFF,true,x49}\n";

    // Repeat: load/compile/eval/free 30 times
    for (int rep = 0; rep < 30; ++rep) {
        smd::Document doc;
        if (!doc.LoadFromMemory(smd.data(), smd.size())) {
            std::printf("Load %d: %s\n", rep, doc.LastError()); return 1;
        }
        if (!doc.Compile()) {
            std::printf("Compile %d: %s\n", rep, doc.LastError()); return 1;
        }
        for (int f = 0; f < 5; ++f) {
            if (!doc.Evaluate(NoOpCb, nullptr)) {
                std::printf("Eval %d.%d: %s\n", rep, f, doc.LastError()); return 1;
            }
        }
    }
    std::printf("OK: 30 reps of 50-format script\n");
    return 0;
}
