// Regression test for the "stale formatted string" bug.
//
// Previously: Reset() seeded string VARs from config Format defaults by
// calling MaterializeText -- but it did so BEFORE RefreshScratches(),
// which only runs at the top of Evaluate(). So format args evaluated
// against last frame's host scratches, and any string VAR not reassigned
// inside an active `#if` branch carried last frame's data forward.
//
// Symptom on Switch: live FPS / temperature / freq strings appeared frozen
// after a single frame had been rendered, only updating when something
// else also changed in the script flow.
//
// Fix: Reset() calls RefreshScratches() at its entry. The cost is one extra
// pass over ~50 doubles per frame; the gain is that string-VAR seeds
// always reflect current host data. Evaluate() refreshes again at its own
// entry; the second refresh is essentially free.

#include "test_support.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace test_support;

namespace {

int g_failures = 0;

struct Cap {
    std::vector<std::string> texts;
};

void Cb(smd::RenderCommand& cmd, void* user) {
    auto* cap = static_cast<Cap*>(user);
    if (cmd.type == smd::RenderCmdType::Text) {
        cap->texts.push_back(cmd.text);
    } else if (cmd.type == smd::RenderCmdType::GetDimensions && cmd.outDims) {
        cmd.outDims->x = (int64_t)cmd.text.size() * (cmd.fontSize / 2);
        cmd.outDims->y = cmd.fontSize;
    }
}

void ExpectEq(const std::string& actual, const char* expected,
              const char* label) {
    if (actual == expected) {
        std::printf("  PASS %s: '%s'\n", label, actual.c_str());
    } else {
        std::printf("  FAIL %s: got '%s', expected '%s'\n",
            label, actual.c_str(), expected);
        g_failures++;
    }
}

} // namespace

int main() {
    // The script defines `FpsLine` as a config Format that references the
    // host int Game_FPS_int. Reset() will re-seed the FpsLine string-VAR
    // from this Format default each frame. With the bug, Reset()
    // materialised against stale scratches -- so frame 2 would still
    // render frame 1's FPS even after the host changed Game_FPS_int.
    const char* smd =
        "Name = T\n"
        "FpsLine: {\"FPS=%d\", Game_FPS_int}\n"
        "Start:\n"
        "TEXT{0, 0, 18, 0xFFFF, true, FpsLine}\n";

    smd::Document doc;
    if (!doc.LoadFromMemory(smd, std::strlen(smd))) {
        std::printf("FAIL Load: %s\n", doc.LastError()); return 1;
    }
    int64_t fps = 60;
    doc.BindInt64("Game_FPS_int", &fps);
    if (!doc.Compile()) {
        std::printf("FAIL Compile: %s\n", doc.LastError()); return 1;
    }

    // Frame 1: fps=60
    {
        Cap cap;
        doc.Reset();
        doc.Evaluate(Cb, &cap);
        if (cap.texts.empty()) { std::printf("FAIL: no TEXT emitted\n"); return 1; }
        ExpectEq(cap.texts[0], "FPS=60", "frame 1 (fps=60)");
    }

    // Frame 2: fps=30 -- this is where the old code emitted "FPS=60"
    fps = 30;
    {
        Cap cap;
        doc.Reset();
        doc.Evaluate(Cb, &cap);
        if (cap.texts.empty()) { std::printf("FAIL: no TEXT emitted\n"); return 1; }
        ExpectEq(cap.texts[0], "FPS=30", "frame 2 (fps=30)");
    }

    // Frame 3: fps=120 -- confirm propagation continues
    fps = 120;
    {
        Cap cap;
        doc.Reset();
        doc.Evaluate(Cb, &cap);
        if (cap.texts.empty()) { std::printf("FAIL: no TEXT emitted\n"); return 1; }
        ExpectEq(cap.texts[0], "FPS=120", "frame 3 (fps=120)");
    }

    std::printf("\n%d failures total\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
