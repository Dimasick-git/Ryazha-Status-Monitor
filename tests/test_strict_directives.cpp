// Sanity check for the newly-strict directive validation.
#include "smd_parser.hpp"
#include <cstdio>
#include <cstring>
#include <string>

static int g_pass = 0, g_fail = 0;

static void Expect(const char* label, const char* smd,
                   bool expectLoadOk, const char* expectErrSubstr) {
    smd::Document doc;
    bool ok = doc.LoadFromMemory(smd, std::strlen(smd));
    bool ok_match = (ok == expectLoadOk);
    // LastError() is wrapped to 40 columns, so a literal substring lookup
    // can fail across the wrap boundary. Collapse whitespace runs to a
    // single space in both haystack and needle before comparing -- this
    // way "empty target name" matches "empty target\nname" verbatim.
    auto collapseWs = [](const char* s) {
        std::string out;
        if (!s) return out;
        bool prevWs = false;
        for (const char* p = s; *p; ++p) {
            if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
                if (!prevWs) { out.push_back(' '); prevWs = true; }
            } else {
                out.push_back(*p); prevWs = false;
            }
        }
        return out;
    };
    std::string hay    = collapseWs(doc.LastError());
    std::string needle = collapseWs(expectErrSubstr);
    bool err_match = expectErrSubstr == nullptr
        || (!needle.empty() && hay.find(needle) != std::string::npos);
    if (ok_match && err_match) {
        std::printf("  PASS %s\n", label);
        g_pass++;
    } else {
        std::printf("  FAIL %s: ok=%d (expected %d), err=\"%s\" (expected substr=\"%s\")\n",
            label, (int)ok, (int)expectLoadOk,
            doc.LastError() ? doc.LastError() : "<null>",
            expectErrSubstr ? expectErrSubstr : "<any>");
        g_fail++;
    }
}

int main() {
    // -- stray top-level #endif
    Expect("stray-endif",
        "Name = T\nStart:\nTEXT{0,0,18,0xFFFF,true,\"x\"}\n#endif\n",
        false, "stray #endif");
    Expect("stray-endif-only",
        "Name = T\nStart:\n#endif\n",
        false, "stray #endif");

    // -- stray top-level #else / #elif
    Expect("stray-else",
        "Name = T\nStart:\n#else\nTEXT{0,0,18,0xFFFF,true,\"x\"}\n",
        false, "stray #else");
    Expect("stray-elif",
        "Name = T\nStart:\n#elif 1\nTEXT{0,0,18,0xFFFF,true,\"x\"}\n",
        false, "stray #elif");

    // -- duplicate #else inside an else branch
    Expect("double-else",
        "Name = T\nStart:\n#if 1\nTEXT{0,0,18,0xFFFF,true,\"a\"}\n"
        "#else\nTEXT{0,0,18,0xFFFF,true,\"b\"}\n"
        "#else\nTEXT{0,0,18,0xFFFF,true,\"c\"}\n"
        "#endif\n",
        false, "duplicate #else");

    // -- #elif AFTER #else (illegal)
    Expect("elif-after-else",
        "Name = T\nStart:\n#if 1\nTEXT{0,0,18,0xFFFF,true,\"a\"}\n"
        "#else\nTEXT{0,0,18,0xFFFF,true,\"b\"}\n"
        "#elif 0\nTEXT{0,0,18,0xFFFF,true,\"c\"}\n"
        "#endif\n",
        false, "#elif after #else");

    // -- VAR with no RHS
    Expect("var-empty-rhs",
        "Name = T\nStart:\nVAR{x, }\n",
        false, "empty RHS");
    Expect("var-empty-name",
        "Name = T\nStart:\nVAR{, 5}\n",
        false, "empty target name");

    // -- Positive cases: legitimate use is unchanged.
    Expect("ok-bare-if",
        "Name = T\nStart:\n#if 1\nTEXT{0,0,18,0xFFFF,true,\"a\"}\n#endif\n",
        true, nullptr);
    Expect("ok-if-else",
        "Name = T\nStart:\n#if 1\nTEXT{0,0,18,0xFFFF,true,\"a\"}\n"
        "#else\nTEXT{0,0,18,0xFFFF,true,\"b\"}\n#endif\n",
        true, nullptr);
    Expect("ok-if-elif-else",
        "Name = T\nStart:\n#if 1\nTEXT{0,0,18,0xFFFF,true,\"a\"}\n"
        "#elif 0\nTEXT{0,0,18,0xFFFF,true,\"b\"}\n"
        "#else\nTEXT{0,0,18,0xFFFF,true,\"c\"}\n#endif\n",
        true, nullptr);
    Expect("ok-nested-if",
        "Name = T\nStart:\n#if 1\n#if 1\nTEXT{0,0,18,0xFFFF,true,\"a\"}\n#endif\n#endif\n",
        true, nullptr);
    Expect("ok-var-numeric",
        "Name = T\nStart:\nVAR{x, 5}\n",
        true, nullptr);
    Expect("ok-var-string",
        "Name = T\nStart:\nVAR{x, \"hello\"}\n",
        true, nullptr);

    std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
