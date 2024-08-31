#include "aes.h"

#include <alice/hollywood/library/registry/secret_registry.h>

#include <library/cpp/openssl/holders/evp.h>

#include <util/memory/blob.h>
#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/string/builder.h>
#include <util/string/hex.h>

#include <array>
#include <cstring>

namespace NAlice::NHollywood::NCrypto {

namespace {

inline constexpr size_t IV_COUNTER_PART_SIZE = Min<size_t>(sizeof(ui64), AES_BLOCK_SIZE);

const TString WEAK_IV = TString(16, '\0');

template <class T>
bool AESImpl(NSecretString::TSecretString key, const T& input, TString& output, bool decrypt, const TString& iv) {
    NOpenSSL::TEvpCipherCtx context;
    Y_ENSURE(std::strlen(key.Value().c_str()) == 32, "Key for AES 256 cbc algorithm must be 32-byte");
    auto keyData = reinterpret_cast<const unsigned char*>(key.Value().c_str());
    auto ivData = reinterpret_cast<const unsigned char*>(iv.data());
    if (EVP_CipherInit_ex(context, EVP_aes_256_cbc(), nullptr, keyData, ivData, decrypt ? 0 : 1) != 1) {
        return false;
    }

    ui32 itersCount = (input.Size() / AES_BLOCK_SIZE);
    if (input.size() % AES_BLOCK_SIZE > 0) {
        ++itersCount;
    }
    TBufferOutput result;
    for (ui32 i = 0; i < itersCount; ++i) {
        ui32 shift = i * AES_BLOCK_SIZE;
        int bytes = 0;

        auto in = reinterpret_cast<const unsigned char*>(input.data()) + shift;
        auto inl = Min<int>(AES_BLOCK_SIZE, input.Size() - shift);

        // the amount of data written can be anything from zero bytes to 2 * AES_BLOCK_SIZE bytes
        std::array<unsigned char, 2 * AES_BLOCK_SIZE> buffer;
        buffer.fill(0);

        if (EVP_CipherUpdate(context, buffer.data(), &bytes, in, inl) != 1) {
            return false;
        }
        result.Write(buffer.data(), bytes);
    }
    output = TString(result.Buffer().data(), result.Buffer().size());
    output.append(AES_BLOCK_SIZE, '\0');
    int finalBytes = 0;
    auto outm = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(output.data())) + result.Buffer().size();
    if (EVP_CipherFinal_ex(context, outm, &finalBytes) != 1) {
        return false;
    }
    output.resize(result.Buffer().size() + finalBytes);
    return true;
}

} // namespace

bool AESEncrypt(NSecretString::TSecretString keyHexEncoded, const TString& input, TString& output, const TString& iv) {
    auto key = HexDecode(keyHexEncoded.Value());
    return AESImpl(TStringBuf(key), input, output, false, iv);
}

bool AESDecrypt(NSecretString::TSecretString keyHexEncoded, const TString& input, TString& output, const TString& iv) {
    auto key = HexDecode(keyHexEncoded.Value());
    return AESImpl(TStringBuf(key), input, output, true, iv);
}

bool AESEncryptWeak(NSecretString::TSecretString keyHexEncoded, const TString& input, TString& output) {
    return AESEncrypt(keyHexEncoded, input, output, WEAK_IV);
}

bool AESDecryptWeak(NSecretString::TSecretString keyHexEncoded, const TString& input, TString& output) {
    return AESDecrypt(keyHexEncoded, input, output, WEAK_IV);
}

bool AESEncryptWithSecret(TStringBuf keySecret, const TString& input, TString& output, const TString& iv) {
    return AESEncrypt(TSecretRegistry::Get().GetSecret(keySecret), input, output, iv);
}
bool AESDecryptWithSecret(TStringBuf keySecret, const TString& input, TString& output, const TString& iv) {
    return AESDecrypt(TSecretRegistry::Get().GetSecret(keySecret), input, output, iv);
}

bool AESEncryptWeakWithSecret(TStringBuf keySecret, const TString& input, TString& output) {
    return AESEncryptWeak(TSecretRegistry::Get().GetSecret(keySecret), input, output);
}

bool AESDecryptWeakWithSecret(TStringBuf keySecret, const TString& input, TString& output) {
    return AESDecryptWeak(TSecretRegistry::Get().GetSecret(keySecret), input, output);
}

const TString& TConstantIVProvider::GetIV() const {
    return IV_;
}

std::atomic<ui64> TIncrementIVProvider::IVCounter_ = 0;

TString TIncrementIVProvider::GetIV() {
    ui64 ivValue = IVCounter_++;
    auto ivCounterPartBlob = TBlob::NoCopy(&ivValue, IV_COUNTER_PART_SIZE);
    return TStringBuilder{} << TString(AES_BLOCK_SIZE - IV_COUNTER_PART_SIZE, '\0') << ivCounterPartBlob.AsStringBuf();
}

} // namespace NAlice::NHollywood::NCrypto
