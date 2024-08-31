#include "aes.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/string/hex.h>

#include <iostream>

namespace NAlice::NHollywood::NCrypto {

namespace {

constexpr TStringBuf AES_KEY_HEX_ENCODED = "A3BB12D1CE9FB2C5966E945DDA6BE1902E403D42F2CE9B64F3C2D185D5184533";
constexpr TStringBuf INVALID_AES_KEY_HEX_ENCRYPTED = "A3BB12D1CE9FB2C5966E945DDA6BE1902E403D42F2CE9B64F3C2D185D518";

const TString DEFAULT_IV = HexDecode("9FFB3F7B016C3255AF86630DD57C9980");

} // namespace

Y_UNIT_TEST_SUITE(CryptoAES) {

Y_UNIT_TEST(EncryptAndDecryptDifferentStringStyles) {
    TVector<TString> originalStrings = {
        "SOMEVERYSECRETTOKEN",
        "1234567890",
        "snake_case_data",
        "PascalCaseData",
        "camelCaseData",
        "\0",
        "data with newline\n"
    };

    for (const auto& originalString : originalStrings) {
        TString encoded;
        UNIT_ASSERT(AESEncrypt(AES_KEY_HEX_ENCODED, originalString, encoded, DEFAULT_IV));
        UNIT_ASSERT_VALUES_UNEQUAL(originalString, encoded); // unlikely to fail
        TString decoded;
        UNIT_ASSERT(AESDecrypt(AES_KEY_HEX_ENCODED, encoded, decoded, DEFAULT_IV));
        UNIT_ASSERT_VALUES_EQUAL(originalString, decoded);
    }
}

Y_UNIT_TEST(EncryptAndDecryptDifferentStringLengths) {
    TVector<TString> originalStrings;
    for (size_t stringLength = 0; stringLength <= 256; ++ stringLength) {
        originalStrings.emplace_back(stringLength, '\0');
    }

    for (const auto& originalString : originalStrings) {
        TString encoded;
        UNIT_ASSERT(AESEncrypt(AES_KEY_HEX_ENCODED, originalString, encoded, DEFAULT_IV));
        UNIT_ASSERT_VALUES_UNEQUAL(originalString, encoded); // unlikely to fail
        TString decoded;
        UNIT_ASSERT(AESDecrypt(AES_KEY_HEX_ENCODED, encoded, decoded, DEFAULT_IV));
        UNIT_ASSERT_VALUES_EQUAL(originalString, decoded);
    }
}

Y_UNIT_TEST(EncryptionUsingConstantIVProvider) {
    TString originalString = "SOMEVERYSECRETTOKEN";
    TConstantIVProvider ivProvider(TString(16, '\0'));

    TString encoded;
    auto iv = AESEncryptAndGetIV(AES_KEY_HEX_ENCODED, originalString, encoded, ivProvider);
    UNIT_ASSERT(iv);
    UNIT_ASSERT_VALUES_UNEQUAL(originalString, encoded); // unlikely to fail

    TString decoded;
    UNIT_ASSERT(AESDecrypt(AES_KEY_HEX_ENCODED, encoded, decoded, *iv));
    UNIT_ASSERT_VALUES_EQUAL(originalString, decoded);
}

Y_UNIT_TEST(EncryptionUsingConstantIVProviderFailsWithInvalidIV) {
    TString originalString = "SOMEVERYSECRETTOKEN";
    TConstantIVProvider ivProvider("1234567890");

    TString encoded;
    UNIT_ASSERT_EXCEPTION(AESEncryptAndGetIV(AES_KEY_HEX_ENCODED, originalString, encoded, ivProvider), yexception);
}

Y_UNIT_TEST(IncrementIVProviderProvidesDifferentIVs) {
    TIncrementIVProvider ivProvider;
    THashSet<TString> generatedIVs;
    for (auto iv_counter = 0; iv_counter < 100; ++iv_counter) {
        auto iv = ivProvider.GetIV();
        UNIT_ASSERT(!generatedIVs.contains(iv));
        generatedIVs.insert(std::move(iv));
    }
}

Y_UNIT_TEST(IncrementIVProviderEncryption) {
    TString originalString = "SOMEVERYSECRETTOKEN";
    TString decoded;

    TString encodedFirst;
    {
        TIncrementIVProvider ivProvider;
        auto iv = AESEncryptAndGetIV(AES_KEY_HEX_ENCODED, originalString, encodedFirst, ivProvider);
        UNIT_ASSERT(iv);
        UNIT_ASSERT_VALUES_UNEQUAL(originalString, encodedFirst); // unlikely to fail

        UNIT_ASSERT(AESDecrypt(AES_KEY_HEX_ENCODED, encodedFirst, decoded, *iv));
        UNIT_ASSERT_VALUES_EQUAL(originalString, decoded);
    }

    TString encodedSecond;
    {
        TIncrementIVProvider ivProvider;
        auto iv = AESEncryptAndGetIV(AES_KEY_HEX_ENCODED, originalString, encodedSecond, ivProvider);
        UNIT_ASSERT(iv);
        UNIT_ASSERT_VALUES_UNEQUAL(originalString, encodedSecond); // unlikely to fail

        UNIT_ASSERT(AESDecrypt(AES_KEY_HEX_ENCODED, encodedSecond, decoded, *iv));
        UNIT_ASSERT_VALUES_EQUAL(originalString, decoded);
    }

    UNIT_ASSERT_VALUES_UNEQUAL(encodedFirst, encodedSecond); // unlikely to fail
}

// NOTE: Do not change this test. If it fails some encrypted data in production
// might become unavailable to decrypt due to fixed initialization vector in previous revision
Y_UNIT_TEST(BackwardCompatibilityTestForEncryptionWithWeakIV) {
    TString originalString = "SOMEVERYSECRETTOKEN";
    TString encoded = HexDecode("790C23FD4572CEE3926A9B6E8AC2180CFA1AA75D4C1F2CF160D4FAA92296B992");

    TString decoded;
    UNIT_ASSERT(AESDecryptWeak(AES_KEY_HEX_ENCODED, encoded, decoded));
    UNIT_ASSERT_VALUES_EQUAL(originalString, decoded);
}

Y_UNIT_TEST(EncryptionUsingShortKeyFails) {
    TString originalString = "SOMEVERYSECRETTOKEN";

    TString encoded;
    UNIT_ASSERT_EXCEPTION(AESEncryptWeak(INVALID_AES_KEY_HEX_ENCRYPTED, originalString, encoded), yexception);
}

}

} // namespace NAlice::NHollywood::NCrypto
