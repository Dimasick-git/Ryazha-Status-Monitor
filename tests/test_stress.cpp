// Sanitizer-targeted stress test for error paths, lifecycle edge cases,
// and repeated reload/compile cycles. Designed to surface leaks (ASan),
// undefined behaviour (UBSan), and use-after-free issues.

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

static void NoOpCb(smd::RenderCommand& cmd, void*) {
    if (cmd.type == smd::RenderCmdType::GetDimensions && cmd.outDims) {
        cmd.outDims->x = (int64_t)cmd.text.size() * (cmd.fontSize / 2);
        cmd.outDims->y = cmd.fontSize;
    }
}

int main() {
    // -- 1. Double-Free is idempotent (already documented behaviour).
    std::printf("[1] Double Free\n");
    {
        smd::Document doc;
        doc.Free();
        doc.Free();   // no double-free, no UB
        // Defaults must still be present after the second Free
        if (doc.GetConfigInt("LayerWidth", 0) != 448) {
            std::printf("  FAIL: defaults missing after double Free\n"); return 1;
        }
    }
    std::printf("  OK\n");

    // -- 2. Destructor on an empty (default-constructed) Document.
    std::printf("[2] Destructor on empty Document\n");
    { smd::Document doc; }  // RAII Free
    std::printf("  OK\n");

    // -- 3. Load -> Free -> Load again (full reuse).
    std::printf("[3] Load/Free/Load reuse\n");
    {
        smd::Document doc;
        std::string a = Slurp("FPS/02.FPSCounter.smd");
        std::string b = Slurp("Other/02.Miscellaneous.smd");
        for (int i = 0; i < 3; ++i) {
            if (!doc.LoadFromMemory(a.data(), a.size())) {
                std::printf("  FAIL load A: %s\n", doc.LastError()); return 1;
            }
            doc.Free();
            if (!doc.LoadFromMemory(b.data(), b.size())) {
                std::printf("  FAIL load B: %s\n", doc.LastError()); return 1;
            }
        }
    }
    std::printf("  OK\n");

    // -- 4. Compile failure -> Free safely. Parser must not leak the
    // partial state. Create a file with a deliberate parse error.
    std::printf("[4] Compile failure -> recovery\n");
    {
        smd::Document doc;
        const char* bad =
            "Name = T\n"
            "Start:\n"
            "VAR{x, this_is_not_a_valid_expression $$$ }\n"
            "TEXT{0, 0, 18, 0xFFFF, true, x}\n";
        if (doc.LoadFromMemory(bad, std::strlen(bad))) {
            // Load succeeds (it's lazy); Compile is where the failure shows.
            if (doc.Compile()) {
                std::printf("FAIL: Compile succeeded on bad input\n");
                return 1;
            } else {
                std::printf("  Compile rejected (as expected): %s\n",
                    doc.LastError());
            }
        }
        // Now try to reload with a valid file -- must succeed.
        std::string a = Slurp("FPS/02.FPSCounter.smd");
        if (!doc.LoadFromMemory(a.data(), a.size())) {
            std::printf("  FAIL recovery load: %s\n", doc.LastError()); return 1;
        }
    }
    std::printf("  OK\n");

    // -- 5. Read-only enforcement -> recovery
    std::printf("[5] Read-only VAR rejection -> recovery\n");
    {
        smd::Document doc;
        const char* bad =
            "Name = T\n"
            "Start:\n"
            "VAR{LayerWidth, 100}\n";   // NB: LayerWidth is RW-default, not RO
        // LayerWidth is in the R/W defaults, so VAR{LayerWidth, ...} is OK.
        // Use a true read-only name:
        const char* bad2 =
            "Name = T\n"
            "Start:\n"
            "VAR{Game_FPS_int, 100}\n";
        if (doc.LoadFromMemory(bad2, std::strlen(bad2))) {
            std::printf("FAIL unexpected: load succeeded\n");
            return 1;
        } else {
            std::printf("  rejected as expected: %s\n", doc.LastError());
        }
        (void)bad;
        // Recovery
        std::string a = Slurp("FPS/02.FPSCounter.smd");
        if (!doc.LoadFromMemory(a.data(), a.size())) {
            std::printf("  FAIL recovery: %s\n", doc.LastError()); return 1;
        }
    }
    std::printf("  OK\n");

    // -- 6. Move-construct + move-assign
    std::printf("[6] Move semantics\n");
    {
        smd::Document a;
        std::string buf = Slurp("FPS/02.FPSCounter.smd");
        if (!a.LoadFromMemory(buf.data(), buf.size())) return 1;
        int64_t fps = 60;
        bool isGame = true;
        a.BindInt64("Game_FPS_int", &fps);
        a.BindBool("Game_IsGameRunning", &isGame);
        if (!a.Compile()) { std::printf("  FAIL Compile: %s\n", a.LastError()); return 1; }

        smd::Document b = std::move(a);
        // a should now be empty-but-valid (defaults restored).
        if (a.GetConfigInt("LayerWidth", 0) != 448) {
            std::printf("  FAIL: moved-from doesn't have defaults\n"); return 1;
        }
        if (!b.Evaluate(NoOpCb, nullptr)) {
            std::printf("  FAIL b.Evaluate: %s\n", b.LastError()); return 1;
        }

        // Now reload `a` and use it -- proves moved-from state is sane.
        std::string buf2 = Slurp("Other/02.Miscellaneous.smd");
        if (!a.LoadFromMemory(buf2.data(), buf2.size())) return 1;
        int64_t kh = 0, nt = 1;
        bool wp = false;
        std::string wpStr = "x";
        a.BindInt64("System_KeysHeld_int", &kh);
        a.BindInt64("Misc_NetworkConnectionType_int", &nt);
        a.BindBool("Misc_IsWiFiPassphrase", &wp);
        a.BindString("Misc_WiFiPassphrase_str", &wpStr);
        int64_t nv = 0;
        a.BindInt64("Misc_NvDecHz_int", &nv);
        a.BindInt64("Misc_NvEncHz_int", &nv);
        a.BindInt64("Misc_NvJpgHz_int", &nv);
        if (!a.Compile()) { std::printf("  FAIL a.Compile: %s\n", a.LastError()); return 1; }
        if (!a.Evaluate(NoOpCb, nullptr)) return 1;

        // Move-assign back
        a = std::move(b);
        if (!a.Evaluate(NoOpCb, nullptr)) return 1;
    }
    std::printf("  OK\n");

    // -- 7. Repeated Evaluate (many frames) on a single Document
    std::printf("[7] 200 frames of Evaluate\n");
    {
        smd::Document doc;
        std::string buf = Slurp("02.Mini.smd");
        if (!doc.LoadFromMemory(buf.data(), buf.size())) return 1;
        bool isGame = true, isDocked = false;
        int64_t cpuHz = 1785000000, cpuRealHz = 1785000000, cpuDelta = 0;
        double  c0 = 50.0, c1 = 60.0, c2 = 70.0, c3 = 80.0;
        int64_t gpuHz = 460000000, gpuRealHz = 460000000, gpuDelta = 0, gpuLoad = 555;
        int64_t ramHz = 1600000000, ramRealHz = 1600000000, ramDelta = 0;
        int64_t ramLoadAll = 250, ramLoadCPU = 100;
        float ramU = 1234.5f, ramT = 4096.0f;
        float socT = 45, pcbT = 40;
        int64_t skinT = 38500;
        float power = -3.5f;
        int64_t batMin = 142;
        float fanLvl = 23, readSpd = 5 * 1048576.0f;
        float fpsAvg = 59.9f, fpsAvgOld = 59.8f;
        smd::ResolutionEntry render[8] = {{1920,1080,100},{1280,720,20},{}};
        smd::ResolutionEntry viewport[8] = {{1920,1080,80},{1280,720,30},{}};
        doc.BindBool("Game_IsGameRunning", &isGame);
        doc.BindBool("System_IsDocked", &isDocked);
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
        doc.BindResolutionArray("Game_ResolutionRenderCalls_int", render);
        doc.BindResolutionArray("Game_ResolutionViewportCalls_int", viewport);
        if (!doc.Compile()) { std::printf("  FAIL Compile: %s\n", doc.LastError()); return 1; }

        // Mutate state slightly each frame
        for (int f = 0; f < 200; ++f) {
            cpuHz   += 1000;
            gpuLoad  = (gpuLoad + 7) % 1000;
            batMin   = (batMin + 1) % 250;
            render[0].calls   = (render[0].calls + 1) % 200;
            viewport[0].calls = (viewport[0].calls + 1) % 200;
            if (f % 50 == 0) doc.Reset();
            if (!doc.Evaluate(NoOpCb, nullptr)) {
                std::printf("  FAIL Eval frame %d: %s\n", f, doc.LastError()); return 1;
            }
        }
    }
    std::printf("  OK\n");

    // -- 8. ClearDimsMeasureCache after frames
    std::printf("[8] ClearDimsMeasureCache\n");
    {
        smd::Document doc;
        std::string buf = Slurp("02.Mini.smd");
        if (!doc.LoadFromMemory(buf.data(), buf.size())) return 1;
        // Just compile + a few evaluates with cache clearing
        // (skip full binding -- defaults will go through and produce 0s)
        // We need at least the read-only names that the file references.
        // Stub binds:
        int64_t z = 0; double dz = 0; bool bz = false; float fz = 0;
        // Skip detailed binds; rely on no-bind behavior with strict_string_checks
        // The names will resolve to 0 implicitly via implicit-zero path. But
        // BindBool/BindInt64 are needed for any names mentioned in conditions.
        doc.BindBool("Game_IsGameRunning", &bz);
        doc.BindBool("System_IsDocked", &bz);
        doc.BindInt64("CPU_Hz_int", &z);
        doc.BindInt64("CPU_RealHz_int", &z);
        doc.BindInt64("CPU_DeltaHz_int", &z);
        doc.BindDouble("CPU_Core0Load_double", &dz);
        doc.BindDouble("CPU_Core1Load_double", &dz);
        doc.BindDouble("CPU_Core2Load_double", &dz);
        doc.BindDouble("CPU_Core3Load_double", &dz);
        doc.BindInt64("GPU_Hz_int", &z); doc.BindInt64("GPU_RealHz_int", &z);
        doc.BindInt64("GPU_DeltaHz_int", &z); doc.BindInt64("GPU_Load_int", &z);
        doc.BindInt64("RAM_Hz_int", &z); doc.BindInt64("RAM_RealHz_int", &z);
        doc.BindInt64("RAM_DeltaHz_int", &z); doc.BindInt64("RAM_LoadAll_int", &z);
        doc.BindInt64("RAM_LoadCPU_int", &z);
        doc.BindFloat("RAM_UsedAllMB_float", &fz); doc.BindFloat("RAM_TotalAllMB_float", &fz);
        doc.BindFloat("Board_SocTemperatureCelsius_float", &fz);
        doc.BindFloat("Board_PcbTemperatureCelsius_float", &fz);
        doc.BindInt64("Board_SkinTemperatureMiliCelsius_int", &z);
        doc.BindFloat("Board_PowerConsumption_float", &fz);
        doc.BindInt64("Board_BatteryTimeEstimateInMinutes_int", &z);
        doc.BindFloat("Board_FanRotationPercentageLevel_float", &fz);
        doc.BindFloat("Game_ReadSpeedPerSecond_float", &fz);
        doc.BindFloat("Game_FpsAvg_float", &fz); doc.BindFloat("Game_FpsAvgOld_float", &fz);
        smd::ResolutionEntry render[8] = {};
        smd::ResolutionEntry viewport[8] = {};
        doc.BindResolutionArray("Game_ResolutionRenderCalls_int", render);
        doc.BindResolutionArray("Game_ResolutionViewportCalls_int", viewport);
        if (!doc.Compile()) { std::printf("  FAIL Compile: %s\n", doc.LastError()); return 1; }
        for (int i = 0; i < 5; ++i) {
            if (!doc.Evaluate(NoOpCb, nullptr)) return 1;
            doc.ClearDimsMeasureCache();
        }
    }
    std::printf("  OK\n");

    // -- 9. Repeated Compile() on the same Document
    std::printf("[9] Repeated Compile() (recompile after Bind changes)\n");
    {
        smd::Document doc;
        std::string buf = Slurp("FPS/02.FPSCounter.smd");
        if (!doc.LoadFromMemory(buf.data(), buf.size())) return 1;
        bool isGame = true;
        int64_t fps = 60;
        doc.BindBool("Game_IsGameRunning", &isGame);
        doc.BindInt64("Game_FPS_int", &fps);
        for (int i = 0; i < 5; ++i) {
            if (!doc.Compile()) { std::printf("  FAIL Compile %d: %s\n", i, doc.LastError()); return 1; }
            if (!doc.Evaluate(NoOpCb, nullptr)) return 1;
        }
    }
    std::printf("  OK\n");

    // -- 10. Peek (static, no Document::Impl allocation)
    std::printf("[10] Peek across all SMD files\n");
    {
        const char* files[] = {
            "Other/01.BatteryCharger.smd", "01.Full.smd", "FPS/02.FPSCounter.smd", "FPS/01.FPSGraph.smd",
            "Other/03.GameResolutions.smd", "03.Micro.smd", "02.Mini.smd", "Other/02.Miscellaneous.smd",
        };
        for (const char* f : files) {
            smd::Document::PeekInfo info;
            if (!smd::Document::Peek(f, info)) {
                std::printf("  Peek failed: %s (no Name field is fine)\n", f);
            }
        }
    }
    std::printf("  OK\n");

    // -- 11. Many Documents stacked + destroyed
    std::printf("[11] 50 Documents created/destroyed\n");
    {
        for (int i = 0; i < 50; ++i) {
            smd::Document doc;
            std::string buf = Slurp("Other/02.Miscellaneous.smd");
            doc.LoadFromMemory(buf.data(), buf.size());
            int64_t z = 0; bool b = false;
            std::string s = "x";
            doc.BindInt64("System_KeysHeld_int", &z);
            doc.BindInt64("Misc_NetworkConnectionType_int", &z);
            doc.BindBool("Misc_IsWiFiPassphrase", &b);
            doc.BindInt64("Misc_NvDecHz_int", &z);
            doc.BindInt64("Misc_NvEncHz_int", &z);
            doc.BindInt64("Misc_NvJpgHz_int", &z);
            doc.BindString("Misc_WiFiPassphrase_str", &s);
            doc.Compile();
            doc.Evaluate(NoOpCb, nullptr);
        }
    }
    std::printf("  OK\n");

    std::printf("\nAll stress tests passed.\n");
    return 0;
}
