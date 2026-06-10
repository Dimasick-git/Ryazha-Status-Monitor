// Shared helpers for the test suite. All tests include this header to
// avoid duplicating `Slurp`, the dummy host, and the no-op render
// callback. The slurp helper checks fread's return value (avoids the
// -Wunused-result warning we get from `(void)fread(...)`).

#pragma once

#include "smd_parser.hpp"

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>

namespace test_support {

// Read a whole file into a std::string. Returns "" on any read failure
// (open, ftell, short read).
inline std::string Slurp(const char* path) {
    std::string out;
    FILE* fp = std::fopen(path, "rb");
    if (!fp) return out;
    if (std::fseek(fp, 0, SEEK_END) != 0) { std::fclose(fp); return out; }
    long sz = std::ftell(fp);
    if (sz < 0)                         { std::fclose(fp); return out; }
    if (std::fseek(fp, 0, SEEK_SET) != 0){ std::fclose(fp); return out; }
    out.resize((size_t)sz);
    if (sz > 0) {
        size_t got = std::fread(&out[0], 1, (size_t)sz, fp);
        if (got != (size_t)sz) out.clear();
    }
    std::fclose(fp);
    return out;
}

// No-op render callback that satisfies GetDimensions with a deterministic
// fake measurement (width = strlen*fontSize/2, height = fontSize). Most
// tests use this as their callback.
inline void NoOpCallback(smd::RenderCommand& cmd, void*) {
    if (cmd.type == smd::RenderCmdType::GetDimensions && cmd.outDims) {
        cmd.outDims->x = (int64_t)cmd.text.size() * (cmd.fontSize / 2);
        cmd.outDims->y = cmd.fontSize;
    }
}

// Live host state covering every predefined runtime variable from
// the spec. Bind once with BindAllPredefined() so tests can focus on
// what they actually want to drive.
struct DummyHost {
    int64_t CPU_Hz_int{}, CPU_RealHz_int{}, CPU_DeltaHz_int{};
    double  CPU_Core0Load_double{}, CPU_Core1Load_double{},
            CPU_Core2Load_double{}, CPU_Core3Load_double{};
    int64_t GPU_Hz_int{}, GPU_RealHz_int{}, GPU_DeltaHz_int{}, GPU_Load_int{};
    int64_t RAM_Hz_int{}, RAM_RealHz_int{}, RAM_DeltaHz_int{},
            RAM_LoadAll_int{}, RAM_LoadCPU_int{};
    float   RAM_UsedAllMB_float{}, RAM_TotalAllMB_float{},
            RAM_UsedApplicationMB_float{}, RAM_TotalApplicationMB_float{},
            RAM_UsedAppletMB_float{}, RAM_TotalAppletMB_float{},
            RAM_UsedSystemMB_float{}, RAM_TotalSystemMB_float{},
            RAM_UsedSystemUnsafeMB_float{}, RAM_TotalSystemUnsafeMB_float{};
    int64_t Board_ChargerCurrentLimit_int{}, Board_ChargerVoltageLimit_int{},
            Board_ChargerConnected_int{};
    float   Board_BatteryCurrentAvg_float{}, Board_BatteryVoltageAvg_float{};
    bool    Board_IsBatteryFiltered{};
    float   Board_BatteryAgePercentage_float{}, Board_BatteryChargePercentage_float{},
            Board_BatteryTemperatureCelcius_float{},
            Board_DesignedFullBatteryCapacity_float{},
            Board_ActualFullBatteryCapacity_float{},
            Board_PowerConsumption_float{};
    int64_t Board_BatteryTimeEstimateInMinutes_int{};
    float   Board_SocTemperatureCelsius_float{}, Board_PcbTemperatureCelsius_float{};
    int64_t Board_SkinTemperatureMiliCelsius_int{};
    float   Board_FanRotationPercentageLevel_float{};
    int64_t Game_LastFrameNumber_int{};
    bool    Game_IsGameRunning{};
    int64_t Game_FPS_int{};
    float   Game_FpsAvgOld_float{}, Game_FpsAvg_float{}, Game_ReadSpeedPerSecond_float{};
    smd::ResolutionEntry render[8]{};
    smd::ResolutionEntry viewport[8]{};
    int64_t System_DisplayRefreshRate_int{};
    bool    System_IsDocked{};
    int64_t System_KeysDown_int{}, System_KeysHeld_int{};
    std::string formattedKeyCombo;
    bool    Misc_IsWiFiPassphrase{};
    int64_t Misc_NvDecHz_int{}, Misc_NvEncHz_int{}, Misc_NvJpgHz_int{},
            Misc_NetworkConnectionType_int{};
    std::string Misc_WiFiPassphrase_str;
};

inline void BindAllPredefined(smd::Document& doc, DummyHost& h) {
    doc.BindInt64 ("CPU_Hz_int",                          &h.CPU_Hz_int);
    doc.BindInt64 ("CPU_RealHz_int",                      &h.CPU_RealHz_int);
    doc.BindInt64 ("CPU_DeltaHz_int",                     &h.CPU_DeltaHz_int);
    doc.BindDouble("CPU_Core0Load_double",                &h.CPU_Core0Load_double);
    doc.BindDouble("CPU_Core1Load_double",                &h.CPU_Core1Load_double);
    doc.BindDouble("CPU_Core2Load_double",                &h.CPU_Core2Load_double);
    doc.BindDouble("CPU_Core3Load_double",                &h.CPU_Core3Load_double);
    doc.BindInt64 ("GPU_Hz_int",                          &h.GPU_Hz_int);
    doc.BindInt64 ("GPU_RealHz_int",                      &h.GPU_RealHz_int);
    doc.BindInt64 ("GPU_DeltaHz_int",                     &h.GPU_DeltaHz_int);
    doc.BindInt64 ("GPU_Load_int",                        &h.GPU_Load_int);
    doc.BindInt64 ("RAM_Hz_int",                          &h.RAM_Hz_int);
    doc.BindInt64 ("RAM_RealHz_int",                      &h.RAM_RealHz_int);
    doc.BindInt64 ("RAM_DeltaHz_int",                     &h.RAM_DeltaHz_int);
    doc.BindInt64 ("RAM_LoadAll_int",                     &h.RAM_LoadAll_int);
    doc.BindInt64 ("RAM_LoadCPU_int",                     &h.RAM_LoadCPU_int);
    doc.BindFloat ("RAM_UsedAllMB_float",                 &h.RAM_UsedAllMB_float);
    doc.BindFloat ("RAM_TotalAllMB_float",                &h.RAM_TotalAllMB_float);
    doc.BindFloat ("RAM_UsedApplicationMB_float",         &h.RAM_UsedApplicationMB_float);
    doc.BindFloat ("RAM_TotalApplicationMB_float",        &h.RAM_TotalApplicationMB_float);
    doc.BindFloat ("RAM_UsedAppletMB_float",              &h.RAM_UsedAppletMB_float);
    doc.BindFloat ("RAM_TotalAppletMB_float",             &h.RAM_TotalAppletMB_float);
    doc.BindFloat ("RAM_UsedSystemMB_float",              &h.RAM_UsedSystemMB_float);
    doc.BindFloat ("RAM_TotalSystemMB_float",             &h.RAM_TotalSystemMB_float);
    doc.BindFloat ("RAM_UsedSystemUnsafeMB_float",        &h.RAM_UsedSystemUnsafeMB_float);
    doc.BindFloat ("RAM_TotalSystemUnsafeMB_float",       &h.RAM_TotalSystemUnsafeMB_float);
    doc.BindInt64 ("Board_ChargerCurrentLimit_int",       &h.Board_ChargerCurrentLimit_int);
    doc.BindInt64 ("Board_ChargerVoltageLimit_int",       &h.Board_ChargerVoltageLimit_int);
    doc.BindInt64 ("Board_ChargerConnected_int",          &h.Board_ChargerConnected_int);
    doc.BindFloat ("Board_BatteryCurrentAvg_float",       &h.Board_BatteryCurrentAvg_float);
    doc.BindFloat ("Board_BatteryVoltageAvg_float",       &h.Board_BatteryVoltageAvg_float);
    doc.BindBool  ("Board_IsBatteryFiltered",             &h.Board_IsBatteryFiltered);
    doc.BindFloat ("Board_BatteryAgePercentage_float",    &h.Board_BatteryAgePercentage_float);
    doc.BindFloat ("Board_BatteryChargePercentage_float", &h.Board_BatteryChargePercentage_float);
    doc.BindFloat ("Board_BatteryTemperatureCelcius_float", &h.Board_BatteryTemperatureCelcius_float);
    doc.BindFloat ("Board_DesignedFullBatteryCapacity_float", &h.Board_DesignedFullBatteryCapacity_float);
    doc.BindFloat ("Board_ActualFullBatteryCapacity_float", &h.Board_ActualFullBatteryCapacity_float);
    doc.BindFloat ("Board_PowerConsumption_float",        &h.Board_PowerConsumption_float);
    doc.BindInt64 ("Board_BatteryTimeEstimateInMinutes_int", &h.Board_BatteryTimeEstimateInMinutes_int);
    doc.BindFloat ("Board_SocTemperatureCelsius_float",   &h.Board_SocTemperatureCelsius_float);
    doc.BindFloat ("Board_PcbTemperatureCelsius_float",   &h.Board_PcbTemperatureCelsius_float);
    doc.BindInt64 ("Board_SkinTemperatureMiliCelsius_int", &h.Board_SkinTemperatureMiliCelsius_int);
    doc.BindFloat ("Board_FanRotationPercentageLevel_float", &h.Board_FanRotationPercentageLevel_float);
    doc.BindInt64 ("Game_LastFrameNumber_int",            &h.Game_LastFrameNumber_int);
    doc.BindBool  ("Game_IsGameRunning",                  &h.Game_IsGameRunning);
    doc.BindInt64 ("Game_FPS_int",                        &h.Game_FPS_int);
    doc.BindFloat ("Game_FpsAvgOld_float",                &h.Game_FpsAvgOld_float);
    doc.BindFloat ("Game_FpsAvg_float",                   &h.Game_FpsAvg_float);
    doc.BindFloat ("Game_ReadSpeedPerSecond_float",       &h.Game_ReadSpeedPerSecond_float);
    doc.BindResolutionArray("Game_ResolutionRenderCalls_int",   h.render);
    doc.BindResolutionArray("Game_ResolutionViewportCalls_int", h.viewport);
    doc.BindInt64 ("System_DisplayRefreshRate_int",       &h.System_DisplayRefreshRate_int);
    doc.BindBool  ("System_IsDocked",                     &h.System_IsDocked);
    doc.BindInt64 ("System_KeysDown_int",                 &h.System_KeysDown_int);
    doc.BindInt64 ("System_KeysHeld_int",                 &h.System_KeysHeld_int);
    doc.BindString("formattedKeyCombo",                   &h.formattedKeyCombo);
    doc.BindBool  ("Misc_IsWiFiPassphrase",               &h.Misc_IsWiFiPassphrase);
    doc.BindInt64 ("Misc_NvDecHz_int",                    &h.Misc_NvDecHz_int);
    doc.BindInt64 ("Misc_NvEncHz_int",                    &h.Misc_NvEncHz_int);
    doc.BindInt64 ("Misc_NvJpgHz_int",                    &h.Misc_NvJpgHz_int);
    doc.BindInt64 ("Misc_NetworkConnectionType_int",      &h.Misc_NetworkConnectionType_int);
    doc.BindString("Misc_WiFiPassphrase_str",             &h.Misc_WiFiPassphrase_str);
}

// Enumerate every .smd file in the current working directory (sorted).
// Used by tests that should treat fixtures opaquely.
inline std::vector<std::string> DiscoverFixtures(const char* dir = ".") {
    std::vector<std::string> out;
    std::error_code ec;
    for (auto& e : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!e.is_regular_file()) continue;
        const auto& p = e.path();
        if (p.extension() == ".smd") out.push_back(p.filename().string());
    }
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace test_support
