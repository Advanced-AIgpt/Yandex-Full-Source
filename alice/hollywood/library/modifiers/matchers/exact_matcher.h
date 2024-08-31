#pragma once

#include <alice/hollywood/library/modifiers/internal/config/proto/exact_key_groups.pb.h>
#include <alice/hollywood/library/modifiers/internal/config/proto/exact_mapping_config.pb.h>
#include <alice/hollywood/library/modifiers/util/tts_and_text.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood::NModifiers {

class TExactMatcher : public NNonCopyable::TMoveOnly {
public:
    using TExactKey = std::tuple<NClient::EPromoType, TString, TString>;
    using TExactMap = THashMap<TExactKey, TVector<TTtsAndText>>;

    TExactMatcher(const TExactMappingConfig& mappingConfig, const TExactKeyGroups& keyGroups);

    const TVector<TTtsAndText>* FindPtr(const NClient::EPromoType promoType, const TString& productScenarioName,
                                        const TString& tts) const;

private:
    const TExactMap Mapping;
};

} // namespace NAlice::NHollywood::NModifiers
