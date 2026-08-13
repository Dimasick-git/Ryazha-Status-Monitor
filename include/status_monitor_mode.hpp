#pragma once

#include <string_view>

// Keep the SMD filename-to-mode mapping in one place. libryazhahand uses this
// identifier to recognise passive Status Monitor modes and must see the same
// value whether a mode was selected from the UI or launched with --file.
inline constexpr std::string_view StatusMonitorModeId(std::string_view relativePath) {
    if (relativePath == "01.Full.smd")                 return "full";
    if (relativePath == "02.Mini.smd")                 return "mini";
    if (relativePath == "03.Micro.smd")                return "micro";
    if (relativePath == "FPS/01.FPSGraph.smd")         return "fps_graph";
    if (relativePath == "FPS/02.FPSCounter.smd")       return "fps_counter";
    if (relativePath == "FPS/03.RyazhaStatus.smd")     return "ryazha_status";
    if (relativePath == "Other/01.BatteryCharger.smd") return "battery_charger";
    if (relativePath == "Other/02.Miscellaneous.smd")  return "miscellaneous";
    if (relativePath == "Other/03.GameResolutions.smd") return "game_resolutions";
    return {};
}

inline constexpr bool IsStatusMonitorMode(std::string_view relativePath) {
    return !StatusMonitorModeId(relativePath).empty();
}
