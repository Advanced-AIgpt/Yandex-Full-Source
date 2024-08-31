#pragma once

#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood::NModifiers {

struct TOutputSpeechInfo {
    TStringBuf StartingSpeakerTag;
    TStringBuf SimplePhrase;
};

struct TCardInfo {
    size_t Index = 0;
    const NScenarios::TLayout::TCard& Card;
    const TString& CardPhrase;
};

class TLayoutInspectorForConjugatablePhrases {
public:
    explicit TLayoutInspectorForConjugatablePhrases(const NScenarios::TLayout& layout);
    void InspectOutputSpeech(std::function<void(TOutputSpeechInfo outputSpeechInfo)> onConjugatableOutputSpeech) const;
    void InspectCards(std::function<void(TCardInfo cardInfo)> onConjugatableCard) const;
public:
    static const TString& GetPhraseFromCard(const NScenarios::TLayout::TCard& card);
    static void SetPhraseInCard(NScenarios::TLayout::TCard& card, const TString& phrase);
private:
    const NScenarios::TLayout& Layout_;
};

} // namespace NAlice::NHollywood::NModifiers
