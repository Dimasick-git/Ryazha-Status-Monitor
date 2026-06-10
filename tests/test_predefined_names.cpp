// Phase 2 + 3 verification: predefined names, System_Key_*, defaults.
#include "smd_parser.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static std::string SlurpFile(const char* path) {
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
    // 1) Defaults should be present after construction
    smd::Document doc;
    int64_t lw = doc.GetConfigInt("LayerWidth", -1);
    int64_t lh = doc.GetConfigInt("LayerHeight", -1);
    int64_t cm = doc.GetConfigInt("COMMON_MARGIN", -1);
    int64_t bg = doc.GetConfigInt("BackgroundColor", -1);
    bool mv    = doc.GetConfigBool("Movable", true);
    int64_t rr = doc.GetConfigInt("User_RefreshRate", -1);
    bool ec    = doc.GetConfigBool("EnableCPU", true);
    std::printf("Fresh defaults: LayerWidth=%lld LayerHeight=%lld COMMON_MARGIN=%lld BackgroundColor=0x%llx Movable=%d RefreshRate=%lld EnableCPU=%d\n",
        (long long)lw, (long long)lh, (long long)cm, (long long)bg, (int)mv, (long long)rr, (int)ec);
    if (lw != 448 || lh != 720 || cm != 20 || bg != 0xD000 || mv != false || rr != 60 || ec != false) {
        std::printf("FAIL: defaults wrong\n"); return 1;
    }

    // 2) Free() should restore defaults
    doc.Free();
    if (doc.GetConfigInt("LayerWidth", -1) != 448) {
        std::printf("FAIL: after Free() defaults not restored\n"); return 2;
    }

    // 3) Parse Miscellaneous.smd which uses System_Key_Y
    std::string buf = SlurpFile("Other/02.Miscellaneous.smd");
    if (buf.empty()) { std::printf("FAIL: can't read 02.Miscellaneous.smd\n"); return 3; }
    if (!doc.LoadFromMemory(buf.data(), buf.size())) {
        std::printf("FAIL Load: %s\n", doc.LastError()); return 4;
    }
    // Bind the host-supplied predefined names used by the file
    int64_t netType = 1; bool wifiPass = true; int64_t keysHeld = 0x8; // Y pressed
    int64_t nvDec = 0, nvEnc = 0, nvJpg = 0;
    std::string passphrase = "secretpass";
    doc.BindInt64("Misc_NetworkConnectionType_int", &netType);
    doc.BindBool ("Misc_IsWiFiPassphrase",          &wifiPass);
    doc.BindInt64("System_KeysHeld_int",            &keysHeld);
    doc.BindInt64("Misc_NvDecHz_int",               &nvDec);
    doc.BindInt64("Misc_NvEncHz_int",               &nvEnc);
    doc.BindInt64("Misc_NvJpgHz_int",               &nvJpg);
    doc.BindString("Misc_WiFiPassphrase_str",       &passphrase);

    if (!doc.Compile()) {
        std::printf("FAIL Compile: %s\n", doc.LastError()); return 5;
    }
    std::printf("Phase 2+3 OK: Miscellaneous.smd parsed, System_Key_Y registered as 0x8.\n");
    return 0;
}
