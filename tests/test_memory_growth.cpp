// Per-frame heap-growth regression test.
//
// Designed to catch leaks like the two we fixed where local std::unordered_map
// caches in Evaluate() and EvaluateFormatArg() pushed te_expr trees into
// the Document-lifetime ownedExprs vector but discarded the cache itself at
// end of call. The leak was invisible to AddressSanitizer at process exit
// (the trees were eventually freed by Document::Free()) but accumulated
// during runtime at ~100 bytes/frame -- enough to crash the overlay after
// ~15 seconds on Switch.
//
// Method: run a long warmup so transient one-time allocations (lazy caches,
// string buffers reaching steady-state capacity) finish growing, then run
// a long measurement period and compare glibc's uordblks (bytes-in-use)
// before and after. Any meaningful delta is a per-frame leak.
//
// Platform: requires glibc's mallinfo2() (Linux). Other platforms get a
// trivial pass with an explanatory message -- the same suite can still
// exercise sanitizers there.

#include "test_support.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

// glibc-specific malloc introspection. Available on Linux with glibc 2.33+
// (released Feb 2021, present on every supported runner image). Newlib /
// devkitPro / musl do not provide mallinfo2; on those we skip the assertion.
#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33))
  #include <malloc.h>
  #define SMD_HAVE_MALLINFO2 1
#else
  #define SMD_HAVE_MALLINFO2 0
#endif

using namespace test_support;

namespace {

#if SMD_HAVE_MALLINFO2
// Bytes-in-use reported by the allocator. uordblks counts heap blocks that
// have been malloc'd and not yet free'd, in bytes.
size_t BytesInUse() {
    auto m = mallinfo2();
    return m.uordblks;
}
#endif

// Runs `frames` Evaluate() cycles. Each cycle does Reset() + Evaluate(),
// the typical per-frame pattern on Switch.
void RunFrames(smd::Document& doc, int frames) {
    for (int i = 0; i < frames; ++i) {
        doc.Reset();
        if (!doc.Evaluate(NoOpCallback, nullptr)) {
            std::printf("Evaluate failed at frame %d: %s\n", i, doc.LastError());
            std::exit(2);
        }
    }
}

// Configure a host with a tiny bit of state so conditions and format
// expressions exercise live code paths (otherwise everything might
// short-circuit into the empty branches).
void Prime(DummyHost& h) {
    h.Game_IsGameRunning = true;
    h.System_DisplayRefreshRate_int = 60;
    h.CPU_Hz_int = 1785000000;
    h.GPU_Hz_int = 460000000;
    h.RAM_Hz_int = 1600000000;
    h.CPU_Core0Load_double = 50;
    h.CPU_Core1Load_double = 60;
    h.CPU_Core2Load_double = 70;
    h.CPU_Core3Load_double = 80;
    h.Board_BatteryTimeEstimateInMinutes_int = 142;
    h.Board_PowerConsumption_float = -3.5f;
    h.formattedKeyCombo = "L+R+ZL";
    h.Misc_WiFiPassphrase_str = "secret";
    h.render[0]   = { 1920, 1080, 100 };
    h.viewport[0] = { 1920, 1080,  80 };
}

// Run the per-fixture memory check. Allowed-growth threshold is generous
// to absorb noise from unordered_map bucket rehashes (which can happen at
// arbitrary timing as new keys appear), but tight enough that the original
// 100-bytes-per-frame leak (~100 KB over 1000 frames) would trip it
// immediately. After fix the actual measured growth is 0 bytes across
// 1000 frames on glibc.
bool CheckFixture(const char* path) {
    std::printf("[%s] ", path);
    std::string buf = Slurp(path);
    if (buf.empty()) { std::printf("FAIL: unreadable\n"); return false; }

    smd::Document doc;
    if (!doc.LoadFromMemory(buf.data(), buf.size())) {
        std::printf("FAIL Load: %s\n", doc.LastError()); return false;
    }
    DummyHost h; Prime(h);
    BindAllPredefined(doc, h);
    if (!doc.Compile()) {
        std::printf("FAIL Compile: %s\n", doc.LastError()); return false;
    }

    // Warmup: lets one-time caches stabilise (arithCache fills on first
    // hits, dimsMeasureCache fills with static strings, etc).
#if SMD_HAVE_MALLINFO2
    constexpr int kWarmup = 200;
    constexpr int kMeasure = 1000;
    constexpr size_t kAllowedGrowthBytes = 4 * 1024;  // 4 KiB slack

    RunFrames(doc, kWarmup);
    size_t before = BytesInUse();

    RunFrames(doc, kMeasure);
    size_t after = BytesInUse();

    ssize_t delta = (ssize_t)after - (ssize_t)before;
    std::printf("warmup=%d measure=%d before=%zu after=%zu delta=%+zd bytes",
        kWarmup, kMeasure, before, after, delta);
    if (delta < 0) { std::printf("  (PASS)\n"); return true; }
    if ((size_t)delta <= kAllowedGrowthBytes) {
        std::printf("  PASS (within %zu KiB slack)\n", kAllowedGrowthBytes/1024);
        return true;
    }
    // Per-frame leak: report rate.
    double perFrame = (double)delta / (double)kMeasure;
    std::printf("\n  FAIL: per-frame heap growth ~%.1f bytes/frame "
                "(%zd bytes over %d frames, threshold %zu KiB)\n",
                perFrame, delta, kMeasure, kAllowedGrowthBytes/1024);
    return false;
#else
    // Without mallinfo2 we still run frames (catches outright crashes /
    // sanitizer trips under qemu/native arm64) but skip the assertion.
    RunFrames(doc, 1200);
    std::printf("SKIP heap-growth check (mallinfo2 unavailable on this platform)\n");
    return true;
#endif
}

} // namespace

int main() {
#if SMD_HAVE_MALLINFO2
    std::printf("Memory growth test (mallinfo2 enabled)\n");
#else
    std::printf("Memory growth test (mallinfo2 unavailable -- assertions skipped)\n");
#endif

    auto fixtures = DiscoverFixtures(".");
    if (fixtures.empty()) {
        std::printf("FAIL: no .smd fixtures in working directory\n"); return 1;
    }

    int passed = 0, failed = 0;
    for (const auto& f : fixtures) {
        if (CheckFixture(f.c_str())) ++passed; else ++failed;
    }
    std::printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
