// Discovers .smd files in the working directory at runtime and runs the
// full pipeline (Load -> Bind predefined names -> Compile -> Evaluate one
// frame) on each. No fixture-specific knowledge: any .smd dropped into
// the directory will be exercised. Useful as a smoke test for new SMD
// files added later -- if it can be parsed at all, this test will catch
// regressions.
//
// Reports a summary line per file (commands emitted, command types seen)
// so the CI log is informative even when nothing fails.

#include "test_support.hpp"

#include <cstdio>
#include <set>
#include <string>
#include <vector>

using namespace test_support;

namespace {

struct Capture {
    int totalCmds = 0;
    std::set<smd::RenderCmdType> typesSeen;
};

void RecordCallback(smd::RenderCommand& cmd, void* user) {
    auto* cap = static_cast<Capture*>(user);
    cap->totalCmds++;
    cap->typesSeen.insert(cmd.type);
    if (cmd.type == smd::RenderCmdType::GetDimensions && cmd.outDims) {
        cmd.outDims->x = (int64_t)cmd.text.size() * (cmd.fontSize / 2);
        cmd.outDims->y = cmd.fontSize;
    }
}

const char* TypeName(smd::RenderCmdType t) {
    switch (t) {
        case smd::RenderCmdType::Text:          return "Text";
        case smd::RenderCmdType::Box:           return "Box";
        case smd::RenderCmdType::EmptyBox:      return "EmptyBox";
        case smd::RenderCmdType::DashedLine:    return "DashedLine";
        case smd::RenderCmdType::GetDimensions: return "GetDimensions";
        case smd::RenderCmdType::GraphLineChart:return "GraphLineChart";
#ifdef DEBUG
        case smd::RenderCmdType::HistoryUpdate: return "HistoryUpdate";
        case smd::RenderCmdType::HistoryClean:  return "HistoryClean";
#endif
    }
    return "?";
}

// Run the pipeline on one fixture. Returns true on success, false on
// any failure (Load / Compile / Evaluate).
bool Process(const std::string& file) {
    std::printf("[%s] ", file.c_str());

    std::string buf = Slurp(file.c_str());
    if (buf.empty()) {
        std::printf("FAIL: empty / unreadable\n");
        return false;
    }
    smd::Document doc;
    if (!doc.LoadFromMemory(buf.data(), buf.size())) {
        std::printf("FAIL Load: %s\n", doc.LastError());
        return false;
    }

    DummyHost h;
    // Set a few benign defaults so feature-gated branches activate
    // somewhere across the fixture set. We deliberately do NOT fine-tune
    // per file -- this test is supposed to be fixture-agnostic.
    h.Game_IsGameRunning = true;
    h.System_DisplayRefreshRate_int = 60;
    h.System_KeysHeld_int = 0x8;
    h.Misc_NetworkConnectionType_int = 1;
    h.Misc_IsWiFiPassphrase = true;
    h.formattedKeyCombo = "L+R+ZL";
    h.Misc_WiFiPassphrase_str = "passphrase";
    h.render[0] = { 1920, 1080, 100 };
    h.viewport[0] = { 1920, 1080, 80 };

    BindAllPredefined(doc, h);
    if (!doc.Compile()) {
        std::printf("FAIL Compile: %s\n", doc.LastError());
        return false;
    }
    Capture cap;
    if (!doc.Evaluate(RecordCallback, &cap)) {
        std::printf("FAIL Evaluate: %s\n", doc.LastError());
        return false;
    }
    std::printf("ok (%d cmds:", cap.totalCmds);
    for (auto t : cap.typesSeen) std::printf(" %s", TypeName(t));
    std::printf(")\n");
    return true;
}

} // namespace

int main() {
    auto fixtures = DiscoverFixtures(".");
    if (fixtures.empty()) {
        std::printf("FAIL: no .smd files found in working directory\n");
        return 1;
    }
    std::printf("Discovered %zu fixture(s)\n", fixtures.size());

    int passed = 0, failed = 0;
    for (const auto& f : fixtures) {
        if (Process(f)) ++passed; else ++failed;
    }
    std::printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
