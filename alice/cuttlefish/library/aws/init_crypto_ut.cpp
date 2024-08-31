#include "init_crypto.h"

#include <library/cpp/testing/unittest/registar.h>

#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-core/include/aws/core/utils/crypto/Factories.h>

using namespace NAlice::NCuttlefish::NAws;


class TCuttlefishAwsInitCryptoTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishAwsInitCryptoTest);
    UNIT_TEST(TestCryptoInitialized);
    UNIT_TEST_SUITE_END();

public:
    void TestCryptoInitialized() {
        UNIT_ASSERT_C(AWS_INIT_CRYPTO, "Something went wrong, crypto init flag is false");

        // Without init crypto this function cause coredump
        // Just sanity check
        auto md5 = Aws::Utils::Crypto::CreateMD5Implementation();
        Y_UNUSED(md5);
    }

};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishAwsInitCryptoTest)
