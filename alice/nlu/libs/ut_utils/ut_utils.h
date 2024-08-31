#pragma once

#include <util/generic/strbuf.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NAlice::NUtUtils {

    // Normalizations:
    //   - remove common indent of lines,
    //   - strip lines at right,
    //   - normalize line endings,
    //   - remove \n at begin and end of text,
    //   - append \n at end of text.
    TString NormalizeText(TStringBuf text);

    // Test strings equal. Call UNIT_FAIL with verbose message (suitable for multiline strings).
    // Arguments:
    //   input - some additional information about test case data (empty if not needed).
    //   expected - expected result of some test.
    //   actual - actual result of some test.
    //   normalize - normalize expected and actual (by NormalizeText) before compare.
    void TestEqualStr(TStringBuf input, TStringBuf expected, TStringBuf actual, bool normalize = true);

    template <class TExpected, class TActual>
    void TestEqualSeq(TStringBuf input, const TExpected& expected, const TActual& actual) {
        TestEqualStr(input, JoinSeq("\n", expected), JoinSeq("\n", actual));
    }

    template <class TExpected, class TActual>
    void TestEqual(TStringBuf input, const TExpected& expected, const TActual& actual) {
        TestEqualStr(input, ToString(expected), ToString(actual));
    }

} // namespace NAlice::NUtUtils
