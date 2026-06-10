// Tests LastError() word-wrap behaviour. Per spec: each line is <= 40
// columns, with breaks preferring whitespace boundaries.

#include "test_support.hpp"

#include <cstdio>
#include <cstring>
#include <string>

namespace {

int g_failures = 0;

bool AllLinesWithinLimit(const char* s, size_t maxCol) {
    if (!s) return true;
    size_t col = 0;
    for (const char* p = s; *p; ++p) {
        if (*p == '\n') { col = 0; continue; }
        if (++col > maxCol) return false;
    }
    return true;
}

void Expect40(smd::Document& doc, const char* label) {
    const char* msg = doc.LastError();
    std::printf("[%s] last error:\n", label);
    // Print indented so the 40-col envelope is visually obvious
    std::printf("    ");
    for (const char* p = msg; *p; ++p) {
        std::putchar(*p);
        if (*p == '\n') std::printf("    ");
    }
    std::printf("\n");
    if (!AllLinesWithinLimit(msg, 40)) {
        std::printf("  FAIL: a line exceeded 40 columns\n");
        g_failures++;
    } else {
        std::printf("  PASS: all lines <= 40 cols\n");
    }
}

} // namespace

int main() {
    // Trigger a few different error messages, all should be wrapped.
    {
        smd::Document doc;
        const char* bad = "Name = T\nStart:\nVAR{Game_FPS_int, 5}\n";
        doc.LoadFromMemory(bad, std::strlen(bad));
        Expect40(doc, "VAR-on-readonly");
    }
    {
        smd::Document doc;
        const char* bad = "Name = T\nStart:\n#endif\n";
        doc.LoadFromMemory(bad, std::strlen(bad));
        Expect40(doc, "stray-endif");
    }
    {
        smd::Document doc;
        const char* bad = "Name = T\nStart:\nTEXT{0, 0, 18, 0xFFFF, true, unknown_identifier_that_is_very_long_indeed}\n";
        doc.LoadFromMemory(bad, std::strlen(bad));
        if (!doc.Compile()) Expect40(doc, "long-ident-compile");
        else std::printf("[long-ident-compile] no error -- skipping\n");
    }
    {
        // Long word that exceeds 40 cols -- must hard-break.
        smd::Document doc;
        const char* bad = "Name = T\nStart:\nVAR{Game_ResolutionRenderCalls_int, 5}\n";
        doc.LoadFromMemory(bad, std::strlen(bad));
        Expect40(doc, "long-readonly-name");
    }
    std::printf("\n%d failures total\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
