// Per-fixture render-output checks. Loads each of the SMD files known to
// be in the repository, feeds the parser realistic host state, runs one
// frame through Evaluate, captures every render command, and asserts:
//   1. Specific RenderCmdType values fired for that fixture (TEXT, BOX,
//      EMPTY_BOX, DASHED_LINE, GET_DIMENSIONS, GRAPH_LINE_CHART -- and,
//      only in DEBUG builds, HistoryUpdate/HistoryClean).
//   2. Specific substrings appear in the concatenated TEXT output.
// Substring matching is intentionally loose -- exact full-text matches
// are brittle to small format-spec tweaks.
//
// Per Marek's spec: HISTORY_UPDATE / HISTORY_CLEAN are debug-only render
// commands. In release builds they're never delivered, even when the
// script triggers them.

#include "test_support.hpp"

#include <cstdio>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

using namespace test_support;

namespace {

struct Capture {
    std::vector<std::string> texts;
    std::set<smd::RenderCmdType> typesSeen;
    int totalCmds = 0;
};

void RecordCallback(smd::RenderCommand& cmd, void* user) {
    auto* cap = static_cast<Capture*>(user);
    cap->totalCmds++;
    cap->typesSeen.insert(cmd.type);
    if (cmd.type == smd::RenderCmdType::Text) {
        cap->texts.push_back(cmd.text);
    } else if (cmd.type == smd::RenderCmdType::GetDimensions && cmd.outDims) {
        cmd.outDims->x = (int64_t)cmd.text.size() * (cmd.fontSize / 2);
        cmd.outDims->y = cmd.fontSize;
    }
}

bool ContainsText(const Capture& cap, const std::string& needle) {
    for (const auto& t : cap.texts) {
        if (t.find(needle) != std::string::npos) return true;
    }
    return false;
}

struct FixtureExpectation {
    const char*                     filename;
    std::vector<smd::RenderCmdType> requireTypes;
    std::vector<std::string>        requireSubstrings;
};

void ConfigureHost(const std::string& file, DummyHost& h) {
    h.Game_IsGameRunning = true;
    h.System_DisplayRefreshRate_int = 60;
    h.formattedKeyCombo = "L+R+ZL";
    h.Misc_WiFiPassphrase_str = "supersecret";
    h.render[0]   = { 1920, 1080, 100 };
    h.render[1]   = { 1280,  720,  60 };
    h.viewport[0] = { 1920, 1080,  80 };
    h.viewport[1] = { 1280,  720,  30 };

    if (file == "Other/01.BatteryCharger.smd") {
        h.Board_PowerConsumption_float = -3.5f;
        h.Board_BatteryTimeEstimateInMinutes_int = 142;
        h.Board_BatteryVoltageAvg_float = 4.05f;
        h.Board_ChargerConnected_int = 1;
        h.Board_BatteryChargePercentage_float = 78.5f;
    } else if (file == "01.Full.smd") {
        h.Board_PowerConsumption_float = -3.5f;
        h.Board_BatteryTimeEstimateInMinutes_int = 142;
        h.CPU_Hz_int = 1785000000;
        h.GPU_Hz_int = 460000000;
        h.RAM_Hz_int = 1600000000;
    } else if (file == "FPS/02.FPSCounter.smd") {
        h.Game_FPS_int = 60;
        h.Game_FpsAvg_float = 59.9f;
    } else if (file == "FPS/01.FPSGraph.smd") {
        h.Game_FPS_int = 60;
        h.Game_FpsAvg_float = 59.9f;
        h.Game_FpsAvgOld_float = 59.8f;
    } else if (file == "Micro.smd") {
        h.CPU_Hz_int = 1785000000;
        h.CPU_RealHz_int = 1785400000;
        h.CPU_Core0Load_double = 50;
        h.CPU_Core1Load_double = 60;
        h.CPU_Core2Load_double = 70;
        h.CPU_Core3Load_double = 80;
        h.GPU_Hz_int = 460000000;
        h.GPU_Load_int = 555;
        h.RAM_Hz_int = 1600000000;
        h.Board_SocTemperatureCelsius_float = 45;
        h.Board_PcbTemperatureCelsius_float = 40;
        h.Board_SkinTemperatureMiliCelsius_int = 38500;
        h.Board_PowerConsumption_float = -3.5f;
        h.Board_BatteryTimeEstimateInMinutes_int = 142;
        h.Game_FpsAvg_float = 59.9f;
        h.Game_FpsAvgOld_float = 59.8f;
    } else if (file == "Mini.smd") {
        h.CPU_Hz_int = 1785000000;
        h.CPU_RealHz_int = 1785400000;
        h.CPU_Core0Load_double = 50;
        h.CPU_Core1Load_double = 60;
        h.CPU_Core2Load_double = 70;
        h.CPU_Core3Load_double = 80;
        h.GPU_Hz_int = 460000000;
        h.GPU_Load_int = 555;
        h.RAM_Hz_int = 1600000000;
        h.Board_PowerConsumption_float = -3.5f;
        h.Board_BatteryTimeEstimateInMinutes_int = 142;
        h.Board_SocTemperatureCelsius_float = 45;
        h.Board_PcbTemperatureCelsius_float = 40;
        h.Board_SkinTemperatureMiliCelsius_int = 38500;
        h.Game_FpsAvg_float = 59.9f;
        h.Game_FpsAvgOld_float = 59.8f;
    } else if (file == "Other/Miscellaneous.smd") {
        h.Misc_NetworkConnectionType_int = 1;
        h.Misc_IsWiFiPassphrase = true;
        h.System_KeysHeld_int = 0x8;  // Y pressed
        h.Misc_NvDecHz_int = 716800000;
        h.Misc_NvEncHz_int = 716800000;
        h.Misc_NvJpgHz_int = 716800000;
    }
}

const std::vector<FixtureExpectation>& Expectations() {
    using T = smd::RenderCmdType;
    // Per-fixture expectations. requireTypes lists the RenderCmdType
    // values that MUST appear at least once for the fixture; requireSub-
    // strings lists strings that MUST appear in some TEXT's contents.
    // The set below is grounded in what each fixture's source actually
    // contains -- updating a fixture to add e.g. a DASHED_LINE should
    // also bump the expectation here.
    static const std::vector<FixtureExpectation> kE = {
        // BatteryCharger uses TEXT only (no GET_DIMENSIONS in this file,
        // contrary to first instinct).
        { "Other/01.BatteryCharger.smd",
          { T::Text },
          { "Battery" } },

        // Design uses TEXT and GET_DIMENSIONS extensively (no boxes /
        // dashed lines).
        { "01.Full.smd",
          { T::Text, T::GetDimensions },
          { "Battery Power Flow", "CPU Usage", "GPU Usage", "RAM Usage" } },

        // FPSCounter draws a single BOX behind the FPS number plus the
        // text itself; measures the text first.
        { "FPS/02.FPSCounter.smd",
          { T::Text, T::Box, T::GetDimensions },
          { } },

        // FPSGraph is the only fixture that exercises EMPTY_BOX, DASHED_LINE
        // and GRAPH_LINE_CHART, so it carries the bulk of the visual-
        // command coverage.
        { "FPS/01.FPSGraph.smd",
          { T::Text, T::Box, T::EmptyBox, T::DashedLine, T::GraphLineChart },
          { } },

        // GameResolutions: a background BOX and a column of TEXT.
        { "Other/03.GameResolutions.smd",
          { T::Text, T::Box },
          { "Depth:", "Viewport:", "1920x1080" } },

        // Micro / Mini both compose with BOX + GET_DIMENSIONS + TEXT.
        { "03.Micro.smd",
          { T::Text, T::Box, T::GetDimensions },
          { } },

        { "02.Mini.smd",
          { T::Text, T::Box, T::GetDimensions },
          { "1920x1080" } },

        // Miscellaneous: TEXT-only Network/Multimedia info panel. The
        // passphrase reveal sits behind a chain of #ifs that don't all
        // fire under default host state, so we only assert on the
        // unconditional headings here.
        { "Other/02.Miscellaneous.smd",
          { T::Text },
          { "Multimedia clock rates", "Network" } },
    };
    return kE;
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

int RunOne(const FixtureExpectation& e) {
    std::printf("[%s]\n", e.filename);

    std::string buf = Slurp(e.filename);
    if (buf.empty()) {
        std::printf("  FAIL: could not read fixture\n");
        return 1;
    }

    smd::Document doc;
    if (!doc.LoadFromMemory(buf.data(), buf.size())) {
        std::printf("  FAIL Load: %s\n", doc.LastError());
        return 1;
    }
    DummyHost h;
    ConfigureHost(e.filename, h);
    BindAllPredefined(doc, h);
    if (!doc.Compile()) {
        std::printf("  FAIL Compile: %s\n", doc.LastError());
        return 1;
    }

    Capture cap;
    if (!doc.Evaluate(RecordCallback, &cap)) {
        std::printf("  FAIL Evaluate: %s\n", doc.LastError());
        return 1;
    }

    std::printf("  %d render commands; types: ", cap.totalCmds);
    for (auto t : cap.typesSeen) std::printf("%s ", TypeName(t));
    std::printf("\n");
    for (const auto& t : cap.texts) std::printf("    TEXT: %s\n", t.c_str());

    int failures = 0;
    for (auto required : e.requireTypes) {
        if (cap.typesSeen.find(required) == cap.typesSeen.end()) {
            std::printf("  FAIL: expected RenderCmdType::%s not emitted\n",
                TypeName(required));
            failures++;
        }
    }
    for (const auto& sub : e.requireSubstrings) {
        if (!ContainsText(cap, sub)) {
            std::printf("  FAIL: expected substring '%s' not found in any TEXT\n",
                sub.c_str());
            failures++;
        }
    }
    if (failures == 0) std::printf("  PASS\n");
    return failures;
}

} // namespace

int main() {
    int totalFailures = 0;
    int totalFixtures = 0;
    int cleanFixtures = 0;
    for (const auto& e : Expectations()) {
        totalFixtures++;
        int f = RunOne(e);
        totalFailures += f;
        if (f == 0) cleanFixtures++;
    }
    std::printf("\n%d / %d fixtures clean (%d total assertion failures)\n",
        cleanFixtures, totalFixtures, totalFailures);
    return totalFailures == 0 ? 0 : 1;
}
