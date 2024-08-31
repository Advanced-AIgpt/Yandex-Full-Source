#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <library/cpp/string_utils/secret_string/secret_string.h>

#include <util/generic/hash.h>

#include <functional>

namespace NAlice::NHollywood {

namespace NImpl {

TString GetSecretAndClearEnvVar(const TString& name, const TString& defaultValue, bool failOnEmptySecrets);

} // namespace NImpl

using TSecretProducer = std::function<TString(bool)>;

class TSecretRegistry {
public:
    static TSecretRegistry& Get();

public:
    void AddSecretProducer(TStringBuf name, TSecretProducer&& secretProducer);
    void CreateSecrets(bool failOnEmptySecrets);
    NSecretString::TSecretString GetSecret(TStringBuf name) const;

private:
    THashMap<TString, TSecretProducer> SecretProducers_;
    THashMap<TString, TString> Secrets_;
};

struct TSecretRegistrator {
    TSecretRegistrator(TStringBuf name, TSecretProducer&& secretProducer);
};

} // namespace NAlice::NHollywood

#define REGISTER_SECRET(name, defaultValue) \
static NAlice::NHollywood::TSecretRegistrator \
Y_GENERATE_UNIQUE_ID(SecretRegistrator)(name, [](bool failOnEmptySecrets) -> TString { \
    return NAlice::NHollywood::NImpl::GetSecretAndClearEnvVar(TString(name), TString(defaultValue), failOnEmptySecrets); \
})
