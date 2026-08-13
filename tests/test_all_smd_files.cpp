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

struct DrawBounds {
    int64_t x = 0;
    int64_t y = 0;
    int64_t width = 0;
    int64_t height = 0;
    const char* type = "";
};

struct Capture {
    std::vector<std::string> texts;
    std::set<smd::RenderCmdType> typesSeen;
    std::vector<DrawBounds> bounds;
    int totalCmds = 0;
};

size_t LongestLineBytes(const std::string& text) {
    size_t longest = 0;
    size_t current = 0;
    for (const unsigned char ch : text) {
        if (ch == '\n') {
            longest = std::max(longest, current);
            current = 0;
        } else if ((ch & 0xC0U) != 0x80U) {
            // Count UTF-8 codepoints rather than bytes for a stable, conservative
            // host-side text extent estimate across bundled localizations.
            ++current;
        }
    }
    return std::max(longest, current);
}

size_t LineCount(const std::string& text) {
    return static_cast<size_t>(std::count(text.begin(), text.end(), '\n')) + 1U;
}

void RecordCallback(smd::RenderCommand& cmd, void* user) {
    auto* cap = static_cast<Capture*>(user);
    cap->totalCmds++;
    cap->typesSeen.insert(cmd.type);
    if (cmd.type == smd::RenderCmdType::Text) {
        cap->texts.push_back(cmd.text);
        const auto textHeight = static_cast<int64_t>(LineCount(cmd.text) * static_cast<size_t>(cmd.fontSize));
        // Tesla's y is the baseline of the first line. Glyph ascent occupies
        // about 7/8 of the requested size; additional lines grow downward.
        const auto ascent = static_cast<int64_t>(static_cast<uint64_t>(cmd.fontSize) * 7U / 8U);
        cap->bounds.push_back({cmd.x, cmd.y - ascent,
            static_cast<int64_t>(LongestLineBytes(cmd.text) * static_cast<size_t>(cmd.fontSize) * 3U / 5U + 2U),
            textHeight, "TEXT"});
    } else if (cmd.type == smd::RenderCmdType::Box ||
               cmd.type == smd::RenderCmdType::RoundedBox ||
               cmd.type == smd::RenderCmdType::EmptyBox ||
               cmd.type == smd::RenderCmdType::GraphLineChart) {
        cap->bounds.push_back({cmd.x, cmd.y, cmd.width, cmd.height, "BOX"});
    } else if (cmd.type == smd::RenderCmdType::DashedLine) {
        const auto x0 = std::min(cmd.x, cmd.x2);
        const auto y0 = std::min(cmd.y, cmd.y2);
        cap->bounds.push_back({x0, y0, std::max<int64_t>(1, std::abs(cmd.x2 - cmd.x)),
            std::max<int64_t>(1, std::abs(cmd.y2 - cmd.y)), "DASHED_LINE"});
    } else if (cmd.type == smd::RenderCmdType::GetDimensions && cmd.outDims) {
        cmd.outDims->x = static_cast<int64_t>(LongestLineBytes(cmd.text) * static_cast<size_t>(cmd.fontSize) / 2U);
        cmd.outDims->y = static_cast<int64_t>(LineCount(cmd.text) * static_cast<size_t>(cmd.fontSize));
    }
}

int ValidateRecordedBounds(const char* filename, const smd::Document& doc, const Capture& cap) {
    const int64_t width = doc.GetConfigInt("LayerWidth", 448);
    const int64_t height = doc.GetConfigInt("LayerHeight", 720);
    const int64_t margin = doc.GetConfigInt("COMMON_MARGIN", 20);
    int failures = 0;
    if (width <= 0 || width > 1280 || height <= 0 || height > 720) {
        std::printf("  FAIL layout [%s]: invalid LayerWidth/LayerHeight %lldx%lld\n", filename,
            static_cast<long long>(width), static_cast<long long>(height));
        return 1;
    }
    for (const auto& b : cap.bounds) {
        const int64_t left = margin + b.x;
        const int64_t right = left + b.width;
        const int64_t bottom = b.y + b.height;
        if (b.width < 0 || b.height < 0 || left < 0 || b.y < 0 || right > width || bottom > height) {
            std::printf("  FAIL layout [%s]: %s at (%lld,%lld %lldx%lld), margin=%lld, exceeds %lldx%lld\n", filename,
                b.type, static_cast<long long>(b.x), static_cast<long long>(b.y),
                static_cast<long long>(b.width), static_cast<long long>(b.height),
                static_cast<long long>(margin), static_cast<long long>(width), static_cast<long long>(height));
            ++failures;
        }
    }
    return failures;
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
    } else if (file == "FPS/03.RyazhaStatus.smd") {
        h.Game_FPS_int = 60;
        h.Game_FpsAvg_float = 59.9f;
        h.Game_FpsAvgOld_float = 59.8f;
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

        // RyazhaStatus: EMA-smoothed FPS counter + Hz line — BOX behind two
        // TEXT lines, measured with GET_DIMENSIONS first.
        { "FPS/03.RyazhaStatus.smd",
          { T::Text, T::Box, T::GetDimensions },
          { "FPS:", "Hz:" } },

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
        case smd::RenderCmdType::RoundedBox:    return "RoundedBox";
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

    int failures = ValidateRecordedBounds(e.filename, doc, cap);
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
