// Edge-case sanitizer test: inputs designed to exercise unusual parse paths.
#include "smd_parser.hpp"
#include <cstdio>
#include <cstring>

static void NoOpCb(smd::RenderCommand&, void*) {}

static void TryLoad(const char* label, const char* smd) {
    std::printf("[%s]\n", label);
    smd::Document doc;
    bool ok = doc.LoadFromMemory(smd, std::strlen(smd));
    if (!ok) {
        std::printf("  Load rejected: %s\n", doc.LastError());
        return;
    }
    if (!doc.Compile()) {
        std::printf("  Compile rejected: %s\n", doc.LastError());
        return;
    }
    if (!doc.Evaluate(NoOpCb, nullptr)) {
        std::printf("  Evaluate rejected: %s\n", doc.LastError());
        return;
    }
    std::printf("  Accepted\n");
}

int main() {
    // -- Empty file
    TryLoad("empty", "");
    // -- Only Name
    TryLoad("name-only", "Name = T\n");
    // -- No Start: section
    TryLoad("no-start", "Name = T\nLayerWidth: 100\n");
    // -- Start: with no commands
    TryLoad("empty-start", "Name = T\nStart:\n");
    // -- Trailing whitespace, blank lines, comments
    TryLoad("whitespace", "\n\n;comment\nName = T\n\n\n;Start: not yet\nStart:\n\n;render\n");
    // -- Unmatched #if (must be rejected)
    TryLoad("unclosed-if",
        "Name = T\nStart:\n#if 1\nTEXT{0,0,18,0xFFFF,true,\"x\"}\n");
    // -- Extra #endif (must be rejected)
    TryLoad("extra-endif",
        "Name = T\nStart:\nTEXT{0,0,18,0xFFFF,true,\"x\"}\n#endif\n");
    // -- VAR with empty RHS
    TryLoad("var-empty-rhs",
        "Name = T\nStart:\nVAR{x, }\nTEXT{0,0,18,0xFFFF,true,\"x\"}\n");
    // -- VAR self-reference numeric
    TryLoad("var-self",
        "Name = T\nStart:\nVAR{x, 0}\nVAR{x, x + 1}\n");
    // -- Mix of indexed accesses with bound + unbound resolution arrays
    TryLoad("indexed-unbound-arr",
        "Name = T\nEnableGame: true\nStart:\n"
        "#if $Game_IsGameRunning\n"
        "TEXT{0, 0, 18, 0xFFFF, true, \"x\"}\n"
        "#endif\n"
        "VAR{w, SomeUnboundArray[3].calls + 5}\n");
    // -- Deeply nested format spec (3 levels)
    TryLoad("deep-nested-fmt",
        "Name = T\n"
        "A: {\"outer:%s\", {\"mid:%s\", {\"inner:%d\", 42}}}\n"
        "Start:\nTEXT{0,0,18,0xFFFF,true,A}\n");
    // -- Brace-wrapped ternary with nested brace-wrap in then
    TryLoad("nested-brace-ternary",
        "Name = T\n"
        "A: {\"r=%s\", {1 > 0 ? {\"y\"} : {\"n\"}}}\n"
        "Start:\nTEXT{0,0,18,0xFFFF,true,A}\n");
    // -- HISTORY + HISTORY_UPDATE + GRAPH_LINE_CHART (exercises history pool)
    TryLoad("history-cycle",
        "Name = T\n"
        "hist: HISTORY{int, 64}\n"
        "conds: GRAPH_CONDITIONS{{x < 30, 0xF00F, 0xF005}, {x >= 30, 0x0F0F, 0x0F05}}\n"
        "EnableGame: true\nStart:\n"
        "HISTORY_UPDATE{hist, 42}\n"
        "GRAPH_LINE_CHART{0, 0, 100, 60, 0, 100, LEFT_TO_RIGHT, 0xFFFF, 0xFFF8, conds, hist}\n");
    // -- String VAR with %s placeholder pointing at another string VAR
    TryLoad("string-var-chain",
        "Name = T\n"
        "A: \"hello\"\n"
        "Start:\n"
        "VAR{b, \"\"}\n"
        "VAR{b, b + A + \"world\"}\n"
        "TEXT{0,0,18,0xFFFF,true,b}\n");
    // -- System_Key_* as constants in conditions
    TryLoad("system-key-cond",
        "Name = T\nStart:\n"
        "#if $System_KeysHeld_int == System_Key_A or $System_KeysHeld_int == System_Key_B\n"
        "TEXT{0,0,18,0xFFFF,true,\"a-or-b\"}\n"
        "#endif\n");

    return 0;
}
