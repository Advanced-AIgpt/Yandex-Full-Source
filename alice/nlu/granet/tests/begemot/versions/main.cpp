#include <library/cpp/string_utils/scan/scan.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/strip.h>

Y_UNIT_TEST_SUITE(AliceVersions) {

    Y_UNIT_TEST(Test) {
        const TString text = TFileInput(BinaryPath("search/wizard/data/wizard/AliceVersions/versions.txt")).ReadAll();
        ScanKeyValue<false, '\n', ':'>(text, [&] (TStringBuf, TStringBuf value) {
            ui32 v = 0;
            UNIT_ASSERT(TryFromString(StripString(value), v));
        });
    }
}
