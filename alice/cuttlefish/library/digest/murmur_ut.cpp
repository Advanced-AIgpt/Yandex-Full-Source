#include <library/cpp/testing/unittest/registar.h>

#include <util/digest/murmur.h>
#include "murmur.h"


Y_UNIT_TEST_SUITE(Digest) {

Y_UNIT_TEST(NTL_MurMur32) {
    ui32 a = MurmurHash<ui32>("Hello World!", 12);
    ui32 b = NTL::MurmurHash32("Hello World!");

    UNIT_ASSERT_VALUES_EQUAL(a, b);
}


Y_UNIT_TEST(NTL_MurMur64) {
    ui64 a = MurmurHash<ui64>("Hello World!", 12);
    ui64 b = NTL::MurmurHash64("Hello World!");

    UNIT_ASSERT_VALUES_EQUAL(a, b);
}

};
