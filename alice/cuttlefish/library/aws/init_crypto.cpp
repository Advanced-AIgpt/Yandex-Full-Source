#include "init_crypto.h"

#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-core/include/aws/core/utils/crypto/Factories.h>

namespace NAlice::NCuttlefish::NAws {

namespace {

class TInitCrypto {
public:
    TInitCrypto() {
        Aws::Utils::Crypto::InitCrypto();
    }

    ~TInitCrypto() {
        Aws::Utils::Crypto::CleanupCrypto();
    }

    bool CheckInstance() const {
        return true;
    }
};

static const TInitCrypto AWS_INIT_CRYPTO_INTERNAL;

} // namespace

const bool AWS_INIT_CRYPTO = AWS_INIT_CRYPTO_INTERNAL.CheckInstance();

} // namespace NAlice::NCuttlefish::NAws
