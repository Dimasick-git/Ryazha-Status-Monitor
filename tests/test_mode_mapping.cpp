#include "status_monitor_mode.hpp"

#include <array>
#include <cstdio>
#include <string_view>

int main() {
    struct Case {
        std::string_view file;
        std::string_view expected;
    };

    constexpr std::array cases{
        Case{"01.Full.smd",                 "full"},
        Case{"02.Mini.smd",                 "mini"},
        Case{"03.Micro.smd",                "micro"},
        Case{"FPS/01.FPSGraph.smd",         "fps_graph"},
        Case{"FPS/02.FPSCounter.smd",       "fps_counter"},
        Case{"FPS/03.RyazhaStatus.smd",     "ryazha_status"},
        Case{"Other/01.BatteryCharger.smd", "battery_charger"},
        Case{"Other/02.Miscellaneous.smd",  "miscellaneous"},
        Case{"Other/03.GameResolutions.smd", "game_resolutions"},
    };

    int failures = 0;
    for (const auto& test : cases) {
        const auto actual = StatusMonitorModeId(test.file);
        if (actual != test.expected || !IsStatusMonitorMode(test.file)) {
            std::printf("FAIL %.*s: got %.*s, expected %.*s\n",
                        static_cast<int>(test.file.size()), test.file.data(),
                        static_cast<int>(actual.size()), actual.data(),
                        static_cast<int>(test.expected.size()), test.expected.data());
            ++failures;
        }
    }

    if (IsStatusMonitorMode("Unknown.smd") || !StatusMonitorModeId("Unknown.smd").empty()) {
        std::puts("FAIL unknown SMD path was recognised as a mode");
        ++failures;
    }

    if (failures == 0)
        std::puts("PASS all nine Status Monitor mode IDs");
    return failures == 0 ? 0 : 1;
}
