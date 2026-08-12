// Exercises all combinations of the frequency-related Full mode settings.
// Each combination must render cleanly, including configurations where only
// real frequencies or deltas are enabled.

#include "test_support.hpp"

#include <array>
#include <cstdio>
#include <string>

using namespace test_support;

namespace {

struct Options {
    bool showTarget;
    bool showReal;
    bool showDeltas;
};

bool Configure(smd::Document& document, const Options& options) {
    return document.SetConfigBool("User_ShowTargetFrequencies", options.showTarget)
        && document.SetConfigBool("User_ShowRealFrequencies", options.showReal)
        && document.SetConfigBool("User_ShowFrequenceDeltas", options.showDeltas);
}

} // namespace

int main() {
    const std::string source = Slurp("01.Full.smd");
    if (source.empty()) {
        std::printf("FAIL: could not read 01.Full.smd\n");
        return 1;
    }

    smd::Document document;
    if (!document.LoadFromMemory(source.data(), source.size())) {
        std::printf("FAIL Load: %s\n", document.LastError());
        return 1;
    }

    DummyHost host;
    host.Game_IsGameRunning = true;
    host.CPU_Hz_int = 1785000000;
    host.CPU_RealHz_int = 1785400000;
    host.GPU_Hz_int = 460000000;
    host.GPU_RealHz_int = 465000000;
    host.RAM_Hz_int = 1600000000;
    host.RAM_RealHz_int = 1595000000;
    host.GPU_Load_int = 555;
    host.RAM_LoadAll_int = 420;
    host.RAM_LoadCPU_int = 160;
    BindAllPredefined(document, host);

    if (!document.Compile()) {
        std::printf("FAIL Compile: %s\n", document.LastError());
        return 1;
    }

    constexpr std::array<Options, 8> kOptions = {{
        { false, false, false },
        { false, false, true },
        { false, true, false },
        { false, true, true },
        { true, false, false },
        { true, false, true },
        { true, true, false },
        { true, true, true },
    }};

    for (const Options& options : kOptions) {
        if (!Configure(document, options)) {
            std::printf("FAIL: could not apply Full mode options\n");
            return 1;
        }
        document.ClearDimsMeasureCache();
        if (!document.Evaluate(NoOpCallback, nullptr)) {
            std::printf(
                "FAIL Evaluate (target=%d, real=%d, deltas=%d): %s\n",
                options.showTarget,
                options.showReal,
                options.showDeltas,
                document.LastError()
            );
            return 1;
        }
    }

    std::printf("PASS: all Full mode frequency option combinations rendered\n");
    return 0;
}
