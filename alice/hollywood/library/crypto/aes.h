#pragma once

#include <contrib/libs/openssl/include/openssl/aes.h>

#include <library/cpp/string_utils/secret_string/secret_string.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

#include <atomic>

namespace NAlice::NHollywood::NCrypto {

bool AESEncrypt(NSecretString::TSecretString keyHexEncoded, const TString& input, TString& output, const TString& iv);
bool AESDecrypt(NSecretString::TSecretString keyHexEncoded, const TString& input, TString& output, const TString& iv);

bool AESEncryptWeak(NSecretString::TSecretString keyHexEncoded, const TString& input, TString& output);
bool AESDecryptWeak(NSecretString::TSecretString keyHexEncoded, const TString& input, TString& output);

bool AESEncryptWithSecret(TStringBuf keySecret, const TString& input, TString& output, const TString& iv);
bool AESDecryptWithSecret(TStringBuf keySecret, const TString& input, TString& output, const TString& iv);

bool AESEncryptWeakWithSecret(TStringBuf keySecret, const TString& input, TString& output);
bool AESDecryptWeakWithSecret(TStringBuf keySecret, const TString& input, TString& output);

template <typename TIVProvider>
[[nodiscard]] TMaybe<TString> AESEncryptAndGetIV(NSecretString::TSecretString keyHexEncoded,
                                                 const TString& input,
                                                 TString& output,
                                                 TIVProvider& ivProvider)
{
    auto iv = ivProvider.GetIV();
    Y_ENSURE(iv.Size() == AES_BLOCK_SIZE, "Expected Initialization vector to be of size " << AES_BLOCK_SIZE
                                          << " bytes for AES algorithm. Got " << iv.Size() << " bytes instead.");
    if (!AESEncrypt(keyHexEncoded, input, output, iv)) {
        return Nothing();
    }
    return iv;
}

class TConstantIVProvider {
public:
    template <typename TStringU>
    explicit TConstantIVProvider(TStringU&& iv) : IV_(std::forward<TStringU>(iv)) {
    }

    [[nodiscard]] const TString& GetIV() const;

private:
    const TString IV_;
};

class TIncrementIVProvider {
public:
    [[nodiscard]] TString GetIV();

private:
    static std::atomic<ui64> IVCounter_;
};

} // namespace NAlice::NHollywood::NCrypto
