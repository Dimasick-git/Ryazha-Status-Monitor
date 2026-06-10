// Detailed inspection of the new code paths -- struct-VAR copy + indexed
// struct reads + System_Key constants -- using GameResolutions, Mini, and
// Miscellaneous.

#include "smd_parser.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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

struct Capture {
    std::vector<std::string> texts;
};

static void Cb(smd::RenderCommand& cmd, void* u) {
    Capture* cap = (Capture*)u;
    if (cmd.type == smd::RenderCmdType::Text) {
        cap->texts.push_back(cmd.text);
    } else if (cmd.type == smd::RenderCmdType::GetDimensions && cmd.outDims) {
        cmd.outDims->x = (int64_t)cmd.text.size() * (cmd.fontSize / 2);
        cmd.outDims->y = cmd.fontSize;
    }
}

int main() {
    // ===== GameResolutions: tests indexed-struct field access =====
    {
        std::printf("===== GameResolutions.smd =====\n");
        std::string buf = Slurp("Other/03.GameResolutions.smd");
        smd::Document doc;
        if (!doc.LoadFromMemory(buf.data(), buf.size())) {
            std::printf("Load fail: %s\n", doc.LastError()); return 1;
        }
        bool isGame = true;
        smd::ResolutionEntry render[8]   = {};
        smd::ResolutionEntry viewport[8] = {};
        // First entry should be 1920x1080 with 100 calls; remaining smaller.
        render[0]   = { 1920, 1080, 100 };
        render[1]   = { 1280,  720,  60 };
        render[2]   = {  960,  540,  20 };
        viewport[0] = { 1920, 1080, 100 };
        viewport[1] = { 1280,  720,  40 };
        doc.BindBool("Game_IsGameRunning", &isGame);
        doc.BindResolutionArray("Game_ResolutionRenderCalls_int",   render);
        doc.BindResolutionArray("Game_ResolutionViewportCalls_int", viewport);
        if (!doc.Compile()) { std::printf("Compile fail: %s\n", doc.LastError()); return 1; }
        Capture cap;
        if (!doc.Evaluate(Cb, &cap)) { std::printf("Eval fail: %s\n", doc.LastError()); return 1; }
        for (auto& t : cap.texts) std::printf("  TEXT: %s\n", t.c_str());
    }

    // ===== Mini.smd: tests VAR struct copy from Name[N] + .field reads =====
    {
        std::printf("===== Mini.smd =====\n");
        std::string buf = Slurp("02.Mini.smd");
        smd::Document doc;
        if (!doc.LoadFromMemory(buf.data(), buf.size())) {
            std::printf("Load fail: %s\n", doc.LastError()); return 1;
        }
        bool isGame = true, isDocked = false;
        int64_t cpuHz = 1785000000, cpuRealHz = 1785400000;
        int64_t cpuDelta = 0;
        double  c0 = 50.0, c1 = 60.0, c2 = 70.0, c3 = 80.0;
        int64_t gpuHz = 460000000, gpuRealHz = 460000000, gpuDelta = 0, gpuLoad = 555;
        int64_t ramHz = 1600000000, ramRealHz = 1600000000, ramDelta = 0;
        int64_t ramLoadAll = 250, ramLoadCPU = 100;
        float   ramU = 1234.5f, ramT = 4096.0f;
        float   socT = 45.0f, pcbT = 40.0f;
        int64_t skinT = 38500;
        float   power = -3.5f;
        int64_t batMin = 142;
        float   fanLvl = 23.0f;
        float   readSpd = 5.0f * 1048576.0f;
        float   fpsAvg = 59.9f, fpsAvgOld = 59.8f;
        smd::ResolutionEntry render[8]   = {};
        smd::ResolutionEntry viewport[8] = {};
        render[0]   = { 1920, 1080, 100 };
        render[1]   = { 1280,  720,  20 };
        viewport[0] = { 1920, 1080,  80 };
        viewport[1] = { 1280,  720,  30 };
        doc.BindBool("Game_IsGameRunning", &isGame);
        doc.BindBool("System_IsDocked",    &isDocked);
        doc.BindInt64("CPU_Hz_int", &cpuHz);
        doc.BindInt64("CPU_RealHz_int", &cpuRealHz);
        doc.BindInt64("CPU_DeltaHz_int", &cpuDelta);
        doc.BindDouble("CPU_Core0Load_double", &c0);
        doc.BindDouble("CPU_Core1Load_double", &c1);
        doc.BindDouble("CPU_Core2Load_double", &c2);
        doc.BindDouble("CPU_Core3Load_double", &c3);
        doc.BindInt64("GPU_Hz_int", &gpuHz);
        doc.BindInt64("GPU_RealHz_int", &gpuRealHz);
        doc.BindInt64("GPU_DeltaHz_int", &gpuDelta);
        doc.BindInt64("GPU_Load_int", &gpuLoad);
        doc.BindInt64("RAM_Hz_int", &ramHz);
        doc.BindInt64("RAM_RealHz_int", &ramRealHz);
        doc.BindInt64("RAM_DeltaHz_int", &ramDelta);
        doc.BindInt64("RAM_LoadAll_int", &ramLoadAll);
        doc.BindInt64("RAM_LoadCPU_int", &ramLoadCPU);
        doc.BindFloat("RAM_UsedAllMB_float", &ramU);
        doc.BindFloat("RAM_TotalAllMB_float", &ramT);
        doc.BindFloat("Board_SocTemperatureCelsius_float", &socT);
        doc.BindFloat("Board_PcbTemperatureCelsius_float", &pcbT);
        doc.BindInt64("Board_SkinTemperatureMiliCelsius_int", &skinT);
        doc.BindFloat("Board_PowerConsumption_float", &power);
        doc.BindInt64("Board_BatteryTimeEstimateInMinutes_int", &batMin);
        doc.BindFloat("Board_FanRotationPercentageLevel_float", &fanLvl);
        doc.BindFloat("Game_ReadSpeedPerSecond_float", &readSpd);
        doc.BindFloat("Game_FpsAvg_float", &fpsAvg);
        doc.BindFloat("Game_FpsAvgOld_float", &fpsAvgOld);
        doc.BindResolutionArray("Game_ResolutionRenderCalls_int",   render);
        doc.BindResolutionArray("Game_ResolutionViewportCalls_int", viewport);
        if (!doc.Compile()) { std::printf("Compile fail: %s\n", doc.LastError()); return 1; }
        Capture cap;
        if (!doc.Evaluate(Cb, &cap)) { std::printf("Eval fail: %s\n", doc.LastError()); return 1; }
        for (auto& t : cap.texts) std::printf("  TEXT: %s\n", t.c_str());
    }

    // ===== Miscellaneous.smd: tests System_Key_Y constant =====
    {
        std::printf("===== Miscellaneous.smd =====\n");
        std::string buf = Slurp("Other/02.Miscellaneous.smd");
        smd::Document doc;
        if (!doc.LoadFromMemory(buf.data(), buf.size())) {
            std::printf("Load fail: %s\n", doc.LastError()); return 1;
        }
        int64_t netType = 1;
        bool wifiPass = true;
        int64_t keysHeld = 0x8;  // Y pressed
        int64_t nv1 = 716800000, nv2 = 716800000, nv3 = 716800000;
        std::string passphrase = "supersecret123";
        doc.BindInt64("Misc_NetworkConnectionType_int", &netType);
        doc.BindBool("Misc_IsWiFiPassphrase", &wifiPass);
        doc.BindInt64("System_KeysHeld_int", &keysHeld);
        doc.BindInt64("Misc_NvDecHz_int", &nv1);
        doc.BindInt64("Misc_NvEncHz_int", &nv2);
        doc.BindInt64("Misc_NvJpgHz_int", &nv3);
        doc.BindString("Misc_WiFiPassphrase_str", &passphrase);
        if (!doc.Compile()) { std::printf("Compile fail: %s\n", doc.LastError()); return 1; }
        Capture cap;
        if (!doc.Evaluate(Cb, &cap)) { std::printf("Eval fail: %s\n", doc.LastError()); return 1; }
        for (auto& t : cap.texts) std::printf("  TEXT (Y pressed): %s\n", t.c_str());
        // Now unset Y key
        keysHeld = 0;
        cap.texts.clear();
        if (!doc.Evaluate(Cb, &cap)) { std::printf("Eval fail: %s\n", doc.LastError()); return 1; }
        for (auto& t : cap.texts) std::printf("  TEXT (Y not pressed): %s\n", t.c_str());
    }

    // ===== Read-only enforcement =====
    {
        std::printf("===== Read-only enforcement =====\n");
        const char* bad =
            "Name = T\n"
            "Start:\n"
            "VAR{CPU_Hz_int, 100}\n";
        smd::Document doc;
        bool ok = doc.LoadFromMemory(bad, std::strlen(bad));
        if (ok) {
            std::printf("  FAIL: VAR{CPU_Hz_int, ...} should have been rejected\n");
            return 1;
        }
        std::printf("  OK: rejected with: %s\n", doc.LastError());
    }

    return 0;
}
