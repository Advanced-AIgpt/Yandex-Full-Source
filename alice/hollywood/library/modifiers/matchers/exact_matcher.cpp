#include "exact_matcher.h"

#include <util/charset/utf8.h>
#include <util/stream/file.h>

namespace NAlice::NHollywood::NModifiers {

namespace {

TVector<TTtsAndText>
ParseTtsAndTextProto(const google::protobuf::RepeatedPtrField<TExactMappingConfig_TTtsAndText>& proto) {
    TVector<TTtsAndText> ttsAndTextList(Reserve(proto.size()));
    for (const auto& ttsAndText : proto) {
        ttsAndTextList.emplace_back(ttsAndText.GetTts(), ttsAndText.GetText());
    }
    return ttsAndTextList;
}

TExactMatcher::TExactKey BuildKey(const NClient::EPromoType promoType, const TString& productScenarioName,
                                  const TString& oldTts) {
    return TExactMatcher::TExactKey{promoType, productScenarioName, ToLowerUTF8(oldTts)};
}

TExactMatcher::TExactMap MakeMappingFromConfig(const TExactMappingConfig& config,
                                               const TExactKeyGroups& groupsConfig) {
    THashMap<TString, TVector<TString>> phraseGroups;
    phraseGroups.reserve(groupsConfig.GetGroups().size());
    for (const auto& group : groupsConfig.GetGroups()) {
        phraseGroups[group.GetGroupName()] = TVector<TString>{group.GetPhrases().begin(), group.GetPhrases().end()};
    }

    TExactMatcher::TExactMap mapping;
    for (const auto& rule : config.GetMappings()) {
        const auto& groupName = rule.GetOldTtsGroupName();
        const auto* groupPtr = phraseGroups.FindPtr(groupName);
        Y_ENSURE(groupPtr);
        for (const auto& phrase : *groupPtr) {
            mapping[BuildKey(rule.GetDeviceColor(), rule.GetProductScenarioName(), phrase)] =
                ParseTtsAndTextProto(rule.GetNewTtsTextList());
        }
    }
    return mapping;
}

} // namespace

TExactMatcher::TExactMatcher(const TExactMappingConfig& mappingConfig, const TExactKeyGroups& keyGroups)
    : Mapping{MakeMappingFromConfig(mappingConfig, keyGroups)}
{
}

const TVector<TTtsAndText>* TExactMatcher::FindPtr(const NClient::EPromoType promoType,
                                                   const TString& productScenarioName, const TString& tts) const {
    return Mapping.FindPtr(BuildKey(promoType, productScenarioName, tts));
}

} // namespace NAlice::NHollywood::NModifiers
