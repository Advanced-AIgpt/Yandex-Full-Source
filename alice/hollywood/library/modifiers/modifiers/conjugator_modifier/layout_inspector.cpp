#include "layout_inspector.h"
#include <alice/protos/div/div2card.pb.h>
#include <util/generic/yexception.h>

namespace NAlice::NHollywood::NModifiers {

namespace {

TOutputSpeechInfo ParseOutputSpeech(TStringBuf outputSpeech) {
    constexpr TStringBuf SpeakerTagPrefix = "<speaker";
    if (outputSpeech.StartsWith(SpeakerTagPrefix)) {
        const size_t speakerTagEnd = outputSpeech.find('>');
        if (speakerTagEnd != TStringBuf::npos) {
            return TOutputSpeechInfo {
                .StartingSpeakerTag = outputSpeech.SubStr(0, speakerTagEnd + 1),
                .SimplePhrase = outputSpeech.SubStr(speakerTagEnd + 1),
            };
        }
    }
    return TOutputSpeechInfo { .SimplePhrase = outputSpeech };
}

} // namespace

TLayoutInspectorForConjugatablePhrases::TLayoutInspectorForConjugatablePhrases(const NScenarios::TLayout& layout)
    : Layout_(layout)
{
}

void TLayoutInspectorForConjugatablePhrases::InspectOutputSpeech(
    std::function<void(TOutputSpeechInfo outputSpeechInfo)> onConjugatableOutputSpeech) const
{
    const auto outputSpeechInfo = ParseOutputSpeech(Layout_.GetOutputSpeech());
    if (outputSpeechInfo.SimplePhrase) {
        onConjugatableOutputSpeech(outputSpeechInfo);
    }
}

void TLayoutInspectorForConjugatablePhrases::InspectCards(std::function<void(TCardInfo cardInfo)> onConjugatableCard) const {
    for (size_t index = 0; index < Layout_.CardsSize(); ++index) {
        const auto& card = Layout_.GetCards(index);
        if (const auto& phrase = GetPhraseFromCard(card)) {
            onConjugatableCard(TCardInfo{index, card, phrase});
        }
    }
}

const TString& TLayoutInspectorForConjugatablePhrases::GetPhraseFromCard(const NScenarios::TLayout::TCard& card) {
    if (card.HasText()) {
        return card.GetText();
    } else if (card.HasTextWithButtons()) {
        return card.GetTextWithButtons().GetText();
    } else if (card.HasDiv2CardExtended()) {
        return card.GetDiv2CardExtended().GetText();
    } else if (card.HasDiv2CardExtendedNew()) {
        return card.GetDiv2CardExtendedNew().GetText();
    } else {
        return Default<TString>();
    }
}

void TLayoutInspectorForConjugatablePhrases::SetPhraseInCard(NScenarios::TLayout::TCard& card, const TString& phrase) {
    if (card.HasText()) {
        card.SetText(phrase);
    } else if (card.HasTextWithButtons()) {
        card.MutableTextWithButtons()->SetText(phrase);
    } else if (card.HasDiv2CardExtended()) {
        card.MutableDiv2CardExtended()->SetText(phrase);
    } else if (card.HasDiv2CardExtendedNew()) {
        card.MutableDiv2CardExtendedNew()->SetText(phrase);
    } else {
        ythrow yexception() << "Logic error: SetPhraseInCard is not consistent with GetPhraseFromCard";
    }
}

} // namespace NAlice::NHollywood::NModifiers
