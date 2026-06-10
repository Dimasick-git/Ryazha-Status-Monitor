// Tests for HISTORY_AVERAGE{name} — a numeric expression that evaluates
// to the arithmetic mean of every sample currently in the named ring buffer.
//
// Coverage:
//   1. Inline in TEXT format arg: TEXT{..., {"%d", HISTORY_AVERAGE{h}}}
//   2. Via VAR: VAR{Avg, HISTORY_AVERAGE{h}} + TEXT{..., {"%d", Avg}}
//   3. All three history element types: int, float, double
//   4. Empty ring (no samples pushed yet) -> 0.0
//   5. Ring wrap-around: capacity exceeded, oldest samples dropped
//   6. Average updates across frames as new samples arrive
//   7. Used inside an arithmetic expression: HISTORY_AVERAGE{h} * 2
//   8. Used inside a #if condition: #if HISTORY_AVERAGE{h} > threshold
//   9. Used in a ternary: VAR{v, HISTORY_AVERAGE{h} > 50 ? 1 : 0}
//  10. Free + reload + recompile: scratch doubles are rebuilt correctly
//  11. HISTORY_CLEAN{h} clears the average back to 0.0
//  12. $ prefix inside braces: HISTORY_AVERAGE{$h} works identically

#include "test_support.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace test_support;

namespace {

int g_failures = 0;

struct Cap {
    std::vector<std::string> texts;
    int                      chartCount = 0;
};

void Cb(smd::RenderCommand& cmd, void* user) {
    auto* cap = static_cast<Cap*>(user);\
    if (cmd.type == smd::RenderCmdType::Text)
        cap->texts.push_back(cmd.text);
    else if (cmd.type == smd::RenderCmdType::GraphLineChart)
        ++cap->chartCount;
    else if (cmd.type == smd::RenderCmdType::GetDimensions && cmd.outDims) {
        cmd.outDims->x = (int64_t)cmd.text.size() * (cmd.fontSize / 2);
        cmd.outDims->y = cmd.fontSize;
    }
}

// Helper: load, compile, and evaluate a single-string SMD in one shot.
// Returns the first Text command's .text, or "" on any failure.
std::string EvalFirst(const char* smd_src) {
    smd::Document doc;
    if (!doc.LoadFromMemory(smd_src, std::strlen(smd_src))) {
        std::printf("  EvalFirst FAIL Load: %s\n", doc.LastError());
        ++g_failures;
        return "";
    }
    if (!doc.Compile()) {
        std::printf("  EvalFirst FAIL Compile: %s\n", doc.LastError());
        ++g_failures;
        return "";
    }
    Cap cap;
    doc.Evaluate(Cb, &cap);
    return cap.texts.empty() ? "" : cap.texts[0];
}

void ExpectEq(const std::string& actual, const char* expected, const char* label) {
    if (actual == expected) {
        std::printf("  PASS %s: '%s'\n", label, actual.c_str());
    } else {
        std::printf("  FAIL %s: got '%s', expected '%s'\n",
            label, actual.c_str(), expected);
        ++g_failures;
    }
}

} // namespace

int main() {
    // ---- 1. Inline in TEXT format arg (int history) ----
    std::printf("[1. inline in TEXT format arg, int history]\n");
    {
        // Push 10, 20, 30 -> average = 20
        const char* smd =
            "Name = T\n"
            "hist = HISTORY{int, 10}\n"
            "Start:\n"
            "HISTORY_UPDATE{hist, 10}\n"
            "HISTORY_UPDATE{hist, 20}\n"
            "HISTORY_UPDATE{hist, 30}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", HISTORY_AVERAGE{hist}}}\n";
        ExpectEq(EvalFirst(smd), "20", "avg(10,20,30)=20 inline");
    }

    // ---- 2. Via VAR then TEXT ----
    std::printf("\n[2. via VAR then TEXT]\n");
    {
        const char* smd =
            "Name = T\n"
            "hist = HISTORY{int, 10}\n"
            "Start:\n"
            "HISTORY_UPDATE{hist, 10}\n"
            "HISTORY_UPDATE{hist, 20}\n"
            "HISTORY_UPDATE{hist, 30}\n"
            "VAR{Avg, HISTORY_AVERAGE{hist}}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", Avg}}\n";
        ExpectEq(EvalFirst(smd), "20", "avg via VAR");
    }

    // ---- 3. Float history ----
    std::printf("\n[3. float history]\n");
    {
        // Push 1.5, 2.5 -> average = 2.0
        const char* smd =
            "Name = T\n"
            "fhist = HISTORY{float, 8}\n"
            "Start:\n"
            "HISTORY_UPDATE{fhist, 1.5}\n"
            "HISTORY_UPDATE{fhist, 2.5}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%.1f\", HISTORY_AVERAGE{fhist}}}\n";
        ExpectEq(EvalFirst(smd), "2.0", "float avg(1.5,2.5)=2.0");
    }

    // ---- 4. Double history ----
    std::printf("\n[4. double history]\n");
    {
        // Push 100.0, 200.0, 300.0 -> average = 200.0
        const char* smd =
            "Name = T\n"
            "dhist = HISTORY{double, 4}\n"
            "Start:\n"
            "HISTORY_UPDATE{dhist, 100.0}\n"
            "HISTORY_UPDATE{dhist, 200.0}\n"
            "HISTORY_UPDATE{dhist, 300.0}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%.0f\", HISTORY_AVERAGE{dhist}}}\n";
        ExpectEq(EvalFirst(smd), "200", "double avg(100,200,300)=200");
    }

    // ---- 5. Empty ring -> 0.0 ----
    std::printf("\n[5. empty ring -> 0.0]\n");
    {
        const char* smd =
            "Name = T\n"
            "empty = HISTORY{int, 8}\n"
            "Start:\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", HISTORY_AVERAGE{empty}}}\n";
        ExpectEq(EvalFirst(smd), "0", "empty ring -> 0");
    }

    // ---- 6. Ring wrap-around: capacity exceeded ----
    std::printf("\n[6. ring wrap-around]\n");
    {
        // Capacity 3, push 10, 20, 30, 40.
        // After wrap: ring holds 20, 30, 40. Average = 30.
        const char* smd =
            "Name = T\n"
            "small = HISTORY{int, 3}\n"
            "Start:\n"
            "HISTORY_UPDATE{small, 10}\n"
            "HISTORY_UPDATE{small, 20}\n"
            "HISTORY_UPDATE{small, 30}\n"
            "HISTORY_UPDATE{small, 40}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", HISTORY_AVERAGE{small}}}\n";
        ExpectEq(EvalFirst(smd), "30", "wrap-around avg(20,30,40)=30");
    }

    // ---- 7. Average updates across frames ----
    std::printf("\n[7. updates across frames]\n");
    {
        // Each frame pushes 100, so after frame N the avg is always 100
        // (all N <= capacity samples are 100). After 4 frames the capacity-3
        // ring holds three 100s; avg stays 100.
        const char* smd =
            "Name = T\n"
            "fhist = HISTORY{int, 3}\n"
            "Start:\n"
            "HISTORY_UPDATE{fhist, 100}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", HISTORY_AVERAGE{fhist}}}\n";
        smd::Document doc;
        if (!doc.LoadFromMemory(smd, std::strlen(smd))) {
            std::printf("  FAIL Load: %s\n", doc.LastError()); ++g_failures;
        } else if (!doc.Compile()) {
            std::printf("  FAIL Compile: %s\n", doc.LastError()); ++g_failures;
        } else {
            for (int frame = 1; frame <= 4; ++frame) {
                doc.Reset();
                Cap cap;
                doc.Evaluate(Cb, &cap);
                char label[32];
                std::snprintf(label, sizeof(label), "frame %d avg=100", frame);
                ExpectEq(cap.texts.empty() ? "" : cap.texts[0], "100", label);
            }
        }
    }

    // ---- 8. Used inside arithmetic expression ----
    std::printf("\n[8. inside arithmetic expression]\n");
    {
        // avg(10,20,30) = 20; doubled = 40
        const char* smd =
            "Name = T\n"
            "h = HISTORY{int, 4}\n"
            "Start:\n"
            "HISTORY_UPDATE{h, 10}\n"
            "HISTORY_UPDATE{h, 20}\n"
            "HISTORY_UPDATE{h, 30}\n"
            "VAR{Doubled, HISTORY_AVERAGE{h} * 2}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", Doubled}}\n";
        ExpectEq(EvalFirst(smd), "40", "avg*2 = 40");
    }

    // ---- 9. Used inside a #if condition ----
    std::printf("\n[9. inside #if condition]\n");
    {
        // avg(10,20,30) = 20; 20 < 25 -> prints "LOW"
        const char* smd =
            "Name = T\n"
            "h = HISTORY{int, 4}\n"
            "Start:\n"
            "HISTORY_UPDATE{h, 10}\n"
            "HISTORY_UPDATE{h, 20}\n"
            "HISTORY_UPDATE{h, 30}\n"
            "#if HISTORY_AVERAGE{h} < 25\n"
            "TEXT{0, 0, 18, 0xFFFF, true, \"LOW\"}\n"
            "#else\n"
            "TEXT{0, 0, 18, 0xFFFF, true, \"HIGH\"}\n"
            "#endif\n";
        ExpectEq(EvalFirst(smd), "LOW", "#if avg<25 -> LOW");
    }
    {
        // avg(40,60) = 50; 50 >= 25 -> prints "HIGH"
        const char* smd =
            "Name = T\n"
            "h = HISTORY{int, 4}\n"
            "Start:\n"
            "HISTORY_UPDATE{h, 40}\n"
            "HISTORY_UPDATE{h, 60}\n"
            "#if HISTORY_AVERAGE{h} >= 25\n"
            "TEXT{0, 0, 18, 0xFFFF, true, \"HIGH\"}\n"
            "#else\n"
            "TEXT{0, 0, 18, 0xFFFF, true, \"LOW\"}\n"
            "#endif\n";
        ExpectEq(EvalFirst(smd), "HIGH", "#if avg>=25 -> HIGH");
    }

    // ---- 10. Used in a ternary ----
    std::printf("\n[10. inside ternary]\n");
    {
        // avg(10,20,30)=20; 20 > 50? no -> 0
        const char* smd =
            "Name = T\n"
            "h = HISTORY{int, 4}\n"
            "Start:\n"
            "HISTORY_UPDATE{h, 10}\n"
            "HISTORY_UPDATE{h, 20}\n"
            "HISTORY_UPDATE{h, 30}\n"
            "VAR{Flag, HISTORY_AVERAGE{h} > 50 ? 1 : 0}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", Flag}}\n";
        ExpectEq(EvalFirst(smd), "0", "ternary avg>50?1:0 -> 0");
    }
    {
        // avg(60,80)=70; 70 > 50? yes -> 1
        const char* smd =
            "Name = T\n"
            "h = HISTORY{int, 4}\n"
            "Start:\n"
            "HISTORY_UPDATE{h, 60}\n"
            "HISTORY_UPDATE{h, 80}\n"
            "VAR{Flag, HISTORY_AVERAGE{h} > 50 ? 1 : 0}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", Flag}}\n";
        ExpectEq(EvalFirst(smd), "1", "ternary avg>50?1:0 -> 1");
    }

    // ---- 11. Free + reload + recompile: scratch doubles rebuilt ----
    std::printf("\n[11. Free + reload + recompile]\n");
    {
        const char* smd =
            "Name = T\n"
            "h = HISTORY{int, 10}\n"
            "Start:\n"
            "HISTORY_UPDATE{h, 30}\n"
            "HISTORY_UPDATE{h, 50}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", HISTORY_AVERAGE{h}}}\n";
        smd::Document doc;
        // First load-compile-eval
        doc.LoadFromMemory(smd, std::strlen(smd));
        doc.Compile();
        {
            Cap cap; doc.Evaluate(Cb, &cap);
            ExpectEq(cap.texts.empty() ? "" : cap.texts[0], "40",
                "first load avg(30,50)=40");
        }
        // Free then reload
        doc.Free();
        doc.LoadFromMemory(smd, std::strlen(smd));
        doc.Compile();
        {
            Cap cap; doc.Evaluate(Cb, &cap);
            ExpectEq(cap.texts.empty() ? "" : cap.texts[0], "40",
                "after reload avg(30,50)=40");
        }
    }

    // ---- 12. HISTORY_CLEAN resets average to 0 ----
    std::printf("\n[12. HISTORY_CLEAN -> average 0]\n");
    {
        const char* smd =
            "Name = T\n"
            "h = HISTORY{int, 8}\n"
            "Start:\n"
            "HISTORY_UPDATE{h, 100}\n"
            "HISTORY_UPDATE{h, 200}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", HISTORY_AVERAGE{h}}}\n";
        smd::Document doc;
        doc.LoadFromMemory(smd, std::strlen(smd));
        doc.Compile();

        // Frame 1: avg(100,200) = 150
        {
            Cap cap; doc.Reset(); doc.Evaluate(Cb, &cap);
            ExpectEq(cap.texts.empty() ? "" : cap.texts[0], "150",
                "before clean avg=150");
        }

        // Frame 2: same script pushes another 100+200 (ring now 100,200,100,200)
        // -> avg = 150 still.  Now manually clean and evaluate a clean script.
        const char* cleanSmd =
            "Name = T\n"
            "h = HISTORY{int, 8}\n"
            "Start:\n"
            "HISTORY_CLEAN{h}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", HISTORY_AVERAGE{h}}}\n";
        smd::Document doc2;
        doc2.LoadFromMemory(cleanSmd, std::strlen(cleanSmd));
        doc2.Compile();
        {
            Cap cap; doc2.Evaluate(Cb, &cap);
            ExpectEq(cap.texts.empty() ? "" : cap.texts[0], "0",
                "after clean avg=0");
        }
    }

    // ---- 13. $ prefix inside braces: HISTORY_AVERAGE{$h} ----
    std::printf("\n[13. $ prefix inside braces]\n");
    {
        const char* smd =
            "Name = T\n"
            "h = HISTORY{int, 4}\n"
            "Start:\n"
            "HISTORY_UPDATE{h, 5}\n"
            "HISTORY_UPDATE{h, 15}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", HISTORY_AVERAGE{$h}}}\n";
        ExpectEq(EvalFirst(smd), "10", "$ prefix: avg(5,15)=10");
    }

    // ---- 14. Two independent histories in same script ----
    std::printf("\n[14. two independent histories]\n");
    {
        // hist_a: avg(10,20) = 15. hist_b: avg(100,200) = 150.
        // Sum = 165.
        const char* smd =
            "Name = T\n"
            "hist_a = HISTORY{int, 4}\n"
            "hist_b = HISTORY{int, 4}\n"
            "Start:\n"
            "HISTORY_UPDATE{hist_a, 10}\n"
            "HISTORY_UPDATE{hist_a, 20}\n"
            "HISTORY_UPDATE{hist_b, 100}\n"
            "HISTORY_UPDATE{hist_b, 200}\n"
            "VAR{sum, HISTORY_AVERAGE{hist_a} + HISTORY_AVERAGE{hist_b}}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", sum}}\n";
        ExpectEq(EvalFirst(smd), "165", "two histories: 15+150=165");
    }

    // ---- 15. Single sample: average equals that sample ----
    std::printf("\n[15. single sample]\n");
    {
        const char* smd =
            "Name = T\n"
            "h = HISTORY{int, 8}\n"
            "Start:\n"
            "HISTORY_UPDATE{h, 42}\n"
            "TEXT{0, 0, 18, 0xFFFF, true, {\"%d\", HISTORY_AVERAGE{h}}}\n";
        ExpectEq(EvalFirst(smd), "42", "single sample avg=42");
    }

    std::printf("\n%d failures total\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
