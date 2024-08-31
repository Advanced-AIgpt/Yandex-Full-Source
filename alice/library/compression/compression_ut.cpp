#include <alice/library/compression/compression.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

Y_UNIT_TEST_SUITE(Compression) {
    Y_UNIT_TEST(ZLib) {
        const TStringBuf sample = "Привет мир!";
        UNIT_ASSERT_EQUAL(ZLibDecompress(ZLibCompress(sample)), sample);
    }
}
