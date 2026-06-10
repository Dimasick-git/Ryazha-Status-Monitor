// Validate that the brace-wrapped ternary in Design.smd's BatteryChargingText
// produces the expected output for both branches.
#include "smd_parser.hpp"
#include <cstdio>
#include <cstring>
#include <string>

static std::string Slurp(const char* path) {
    FILE* fp = std::fopen(path, "rb");
    if (!fp) return "";
    std::fseek(fp, 0, SEEK_END);
    long sz = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    std::string s; s.resize(sz);
    if (sz > 0) { size_t r = std::fread(&s[0], 1, sz, fp); if (r != (size_t)sz) s.clear(); }
    std::fclose(fp);
    return s;
}

int main() {
    std::string buf = Slurp("01.Full.smd");
    smd::Document doc;
    if (!doc.LoadFromMemory(buf.data(), buf.size())) {
        std::printf("Load: %s\n", doc.LastError()); return 1;
    }
    bool isGame = false;
    float power = -3.5f;
    int64_t batMin = 142;
    int64_t cc = 0, vc = 0, cn = 1;
    float bca = 100, bcc = 80, bct = 30, dfb = 4000, afb = 3900;
    bool batFilt = false;
    float bv = 4.05f, bvi = 4.0f;
    float soc = 45, pcb = 40;
    int64_t skin = 38500;
    float fan = 23;
    // Bind enough that Compile doesn't fail
    doc.BindBool("Game_IsGameRunning", &isGame);
    doc.BindFloat("Board_PowerConsumption_float", &power);
    doc.BindInt64("Board_BatteryTimeEstimateInMinutes_int", &batMin);
    doc.BindInt64("Board_ChargerCurrentLimit_int", &cc);
    doc.BindInt64("Board_ChargerVoltageLimit_int", &vc);
    doc.BindInt64("Board_ChargerConnected_int", &cn);
    doc.BindFloat("Board_BatteryCurrentAvg_float", &bv);
    doc.BindFloat("Board_BatteryVoltageAvg_float", &bvi);
    doc.BindBool("Board_IsBatteryFiltered", &batFilt);
    doc.BindFloat("Board_BatteryAgePercentage_float", &bca);
    doc.BindFloat("Board_BatteryChargePercentage_float", &bcc);
    doc.BindFloat("Board_BatteryTemperatureCelcius_float", &bct);
    doc.BindFloat("Board_DesignedFullBatteryCapacity_float", &dfb);
    doc.BindFloat("Board_ActualFullBatteryCapacity_float", &afb);
    doc.BindFloat("Board_SocTemperatureCelsius_float", &soc);
    doc.BindFloat("Board_PcbTemperatureCelsius_float", &pcb);
    doc.BindInt64("Board_SkinTemperatureMiliCelsius_int", &skin);
    doc.BindFloat("Board_FanRotationPercentageLevel_float", &fan);
    if (!doc.Compile()) { std::printf("Compile: %s\n", doc.LastError()); return 1; }

    auto run = [&](const char* tag, int64_t mins) {
        batMin = mins;
        std::printf("--- %s (batMin=%lld) ---\n", tag, (long long)mins);
        std::string out;
        if (doc.FormatConfigString("BatteryChargingText", out))
            std::printf("BatteryChargingText: '%s'\n", out.c_str());
        else
            std::printf("BatteryChargingText: <missing>\n");
    };
    run("positive", 142);
    run("zero",     0);
    run("negative", -1);
    return 0;
}
