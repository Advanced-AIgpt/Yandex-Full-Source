#include "secret_registry.h"

#include <util/system/defaults.h>
#include <util/system/env.h>

namespace NAlice::NHollywood {

namespace NImpl {

TString GetSecretAndClearEnvVar(const TString& name, const TString& defaultValue, bool failOnEmptySecrets) {
    auto secret = GetEnv(name);
    SetEnv(name, TString());
    Y_ENSURE(!secret.Empty() || !failOnEmptySecrets,
             "Secret " << name << " is empty which is not explicitly allowed by configuration");
    if (!secret.Empty()) {
        return secret;
    } else {
        return defaultValue;
    }
}

} // namespace NImpl

TSecretRegistry& TSecretRegistry::Get() {
    return *Singleton<TSecretRegistry>();
};

void TSecretRegistry::AddSecretProducer(TStringBuf name, TSecretProducer&& secretProducer) {
    Cerr << "Registering secret " << name << Endl;
    const auto [_, inserted] = SecretProducers_.emplace(name, std::move(secretProducer));
    Y_ENSURE(inserted, "Duplicate secret name: " << name);
}

void TSecretRegistry::CreateSecrets(bool failOnEmptySecrets) {
    for (const auto& [name, secretProducer] : SecretProducers_) {
        Secrets_.emplace(name, secretProducer(failOnEmptySecrets));
    }
}

NSecretString::TSecretString TSecretRegistry::GetSecret(TStringBuf name) const {
    Y_ENSURE(Secrets_.contains(name), "Secret " << name << " was not registered");
    return TStringBuf(Secrets_.at(name));
}

TSecretRegistrator::TSecretRegistrator(TStringBuf name, TSecretProducer&& secretProducer) {
    TSecretRegistry::Get().AddSecretProducer(name, std::move(secretProducer));
}

} // namespace NAlice::NHollywood 