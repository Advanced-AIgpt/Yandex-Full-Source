#pragma once

#include <alice/megamind/protos/analytics/modifiers/analytics_info.pb.h>
#include <alice/megamind/protos/modifiers/modifier_response.pb.h>

namespace NAlice::NModifiers::NConjugator {
    class TConjugator;
}

namespace NAlice::NHollywood::NModifiers {

class TModifierAnalyticsInfoBuilder : public NNonCopyable::TMoveOnly {
public:
    using TAnalyticsInfo = NAlice::NModifiers::TAnalyticsInfo;

    TModifierAnalyticsInfoBuilder() = default;

    TModifierAnalyticsInfoBuilder& SetColoredSpeaker(const TString& prevResponseTts, const TString& newResponseTts);
    TModifierAnalyticsInfoBuilder& SetVoiceDoodle(const TString& prevResponseTts, const TString& newResponseTts);
    TModifierAnalyticsInfoBuilder& SetWhisper(bool isWhisperTagApplied, bool isSoundSetLevelDirectiveApplied);
    TModifierAnalyticsInfoBuilder& SetConjugator(::NAlice::NModifiers::NConjugator::TConjugator&& conjugatorAnalyticsInfo);
    TModifierAnalyticsInfoBuilder& SetPolyglot(const size_t translatedPhrasesCount, const size_t uniqueTranslatedPhrasesCount);

    TAnalyticsInfo MoveProto() &&;

private:
    TAnalyticsInfo Proto;
};

} // namespace NAlice::NHollywood::NModifiers
