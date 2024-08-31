#include "builder.h"

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/vector.h>

namespace NAlice::NHollywood::NModifiers {

TResponseBodyBuilder::TResponseBodyBuilder(TModifierBody&& proto)
    : Proto{std::move(proto)}
{
}

TResponseBodyBuilder::TResponseBodyBuilder(const TModifierBody& proto)
    : Proto{proto}
{
}

TModifierBody TResponseBodyBuilder::MoveProto() && {
    return std::move(Proto);
}

const TModifierBody& TResponseBodyBuilder::GetModifierBody() const {
    return Proto;
}

TModifierBody& TResponseBodyBuilder::MutableModifierBody() {
    return Proto;
}

void TResponseBodyBuilder::SetCard(NScenarios::TLayout::TCard&& card, const size_t index) {
    Y_ENSURE(index < Proto.GetLayout().CardsSize(), "Invalid card index " << index << " >= " << Proto.GetLayout().CardsSize());
    *Proto.MutableLayout()->MutableCards(index) = std::move(card);
}

void TResponseBodyBuilder::SetVoice(const TString& tts) {
    auto* layout = Proto.MutableLayout();
    if(!layout->GetOutputSpeech().empty()) {
        *layout->MutableOutputSpeech() = tts;
    }
}

void TResponseBodyBuilder::SetText(const TString& text) {
    auto* layout = Proto.MutableLayout();
    if (auto* cards = layout->MutableCards(); !cards->empty()) {
        *cards->begin()->MutableText() = text;
    }
}

void TResponseBodyBuilder::SetVoiceAndText(const TString& tts, const TString& text) {
    SetVoice(tts);
    SetText(text);
}

void TResponseBodyBuilder::SetRandomPhrase(const TVector<TTtsAndText>& phrases, IRng& rng) {
    if (!phrases.empty()) {
        const auto& phrase = phrases[rng.RandomInteger(phrases.size())];
        SetVoiceAndText(phrase.Tts, phrase.Text);
    }
}

void TResponseBodyBuilder::AppendVoice(const TString& tts) {
    auto* layout = Proto.MutableLayout();
    if(!layout->GetOutputSpeech().empty()) {
        auto& outputSpeech = *layout->MutableOutputSpeech();
        outputSpeech.append(tts);
    }
}

void TResponseBodyBuilder::PrependVoice(const TString& tts) {
    auto* layout = Proto.MutableLayout();
    if(!layout->GetOutputSpeech().empty()) {
        auto& outputSpeech = *layout->MutableOutputSpeech();
        outputSpeech.prepend(tts);
    }
}

void TResponseBodyBuilder::AddDirective(NScenarios::TDirective&& directive) {
    *Proto.MutableLayout()->AddDirectives() = std::move(directive);
}

void TResponseBodyBuilder::AddDirectiveToFront(NScenarios::TDirective&& directive) {
    google::protobuf::RepeatedPtrField<NScenarios::TDirective> newDirectivesList;
    newDirectivesList.Add(std::move(directive));
    const auto& oldDirectives = Proto.GetLayout().GetDirectives();
    newDirectivesList.Add(oldDirectives.begin(), oldDirectives.end());
    Proto.MutableLayout()->MutableDirectives()->Swap(&newDirectivesList);
}

void TResponseBodyBuilder::AddDirectivesToFront(const TVector<NScenarios::TDirective>& directives) {
    google::protobuf::RepeatedPtrField<NScenarios::TDirective> newDirectivesList;
    newDirectivesList.Add(directives.begin(), directives.end());
    const auto& oldDirectives = Proto.GetLayout().GetDirectives();
    newDirectivesList.Add(oldDirectives.begin(), oldDirectives.end());
    Proto.MutableLayout()->MutableDirectives()->Swap(&newDirectivesList);
}

} // namespace NAlice::NHollywood::NModifiers
