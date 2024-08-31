#include "ut_utils.h"
#include <util/string/builder.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NUtUtils {

static size_t CalculateCommonIndent(TStringBuf text) {
    size_t result = Max<size_t>();
    for (TStringBuf line : StringSplitter(text).Split('\n')) {
        const size_t current = line.length() - StripStringLeft(line).length();
        if (current < line.length() && current < result) {
            result = current;
        }
    }
    return result;
}

TString NormalizeText(TStringBuf text) {
    const size_t indent = CalculateCommonIndent(text);
    TString result(Reserve(text.length()));
    for (TStringBuf line : StringSplitter(text).Split('\n')) {
        line.Skip(Min<size_t>(indent, line.length())); // remove common indent
        line = StripStringRight(line); // strip lines at right
        if (line.empty() && result.empty()) {
            continue; // skip empty lines in the beginning of text
        }
        result.append(line);
        result.append('\n');
    }

    // remove empty lines at the end of text
    while (result.EndsWith('\n')) {
        result.erase(result.length() - 1);
    }
    result.append('\n');

    return result;
}

void TestEqualStr(TStringBuf input, TStringBuf expectedRaw, TStringBuf actualRaw, bool normalize) {
    const TString expected = normalize ? NormalizeText(expectedRaw) : TString::Join(expectedRaw, "\n");
    const TString actual = normalize ? NormalizeText(actualRaw) : TString::Join(actualRaw, "\n");
    if (actual == expected) {
        return;
    }
    TStringBuilder out;
    out << Endl;
    out << "Error:" << Endl;
    if (!input.empty()) {
        out << "== input =====================" << Endl;
        out << NormalizeText(input);
    }
    out << "== expected ==================" << Endl;
    out << expected;
    out << "== actual ====================" << Endl;
    out << actual;
    out << "==============================" << Endl;
    UNIT_FAIL(out);
}

} // namespace NAlice::NUtUtils
