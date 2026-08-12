#include "smd_parser.hpp"
#include <cstdio>
#include <string>

static void NoOpCb(smd::RenderCommand&, void*) {}

static void CaptureText(smd::RenderCommand& cmd, void* user) {
    if (cmd.type == smd::RenderCmdType::Text)
        *static_cast<std::string*>(user) = cmd.text;
}

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
    // A user-controlled field width may legitimately exceed a small stack
    // buffer. The formatter must produce the full value without reading past
    // its temporary storage.
    const std::string wideSmd =
        "Name = WideFormat\n"
        "Wide: {\"%1000d\", 7}\n"
        "Start:\n"
        "TEXT{0,0,18,0xFFFF,true,Wide}\n";
    smd::Document wideDoc;
    if (!wideDoc.LoadFromMemory(wideSmd.data(), wideSmd.size()) || !wideDoc.Compile()) {
        std::printf("Wide format setup failed: %s\n", wideDoc.LastError());
        return 1;
    }
    std::string rendered;
    if (!wideDoc.Evaluate(CaptureText, &rendered)) {
        std::printf("Wide format evaluation failed: %s\n", wideDoc.LastError());
        return 1;
    }
    if (rendered.size() != 1000 || rendered.back() != '7') {
        std::printf("Wide numeric format result is invalid: size=%zu\n", rendered.size());
        return 1;
    }

    const std::string wideStringSmd =
        "Name = WideStringFormat\n"
        "Wide: {\"%1000s\", \"ok\"}\n"
        "Start:\n"
        "TEXT{0,0,18,0xFFFF,true,Wide}\n";
    smd::Document wideStringDoc;
    if (!wideStringDoc.LoadFromMemory(wideStringSmd.data(), wideStringSmd.size()) || !wideStringDoc.Compile()) {
        std::printf("Wide string format setup failed: %s\n", wideStringDoc.LastError());
        return 1;
    }
    rendered.clear();
    if (!wideStringDoc.Evaluate(CaptureText, &rendered) || rendered.size() != 1000 ||
        rendered.substr(rendered.size() - 2) != "ok") {
        std::printf("Wide string format result is invalid: size=%zu\n", rendered.size());
        return 1;
    }

    std::printf("OK: 30 reps of 50-format script and wide formatting\n");
    return 0;
}
