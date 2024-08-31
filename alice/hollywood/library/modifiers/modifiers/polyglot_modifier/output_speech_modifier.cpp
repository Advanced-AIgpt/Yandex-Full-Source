#include "output_speech_modifier.h"
#include <util/generic/hash.h>

namespace NAlice::NHollywood::NModifiers {

namespace {

constexpr TStringBuf SpeakerTagStart = "<speaker";
constexpr TStringBuf VoiceAttributePattern = " voice=\"";

bool TryFindNextSpeakerTag(TStringBuf& outputSpeech, TStringBuf& plainSpeechChunk, TStringBuf& speakerTag) {
    const auto speakerTagStartIndex = outputSpeech.find(SpeakerTagStart);
    if (speakerTagStartIndex == TStringBuf::npos) {
        plainSpeechChunk = outputSpeech;
        outputSpeech.Clear();
        speakerTag.Clear();
        return false;
    }

    TStringBuf speakerTagWithSuffix;
    outputSpeech.SplitAt(speakerTagStartIndex, plainSpeechChunk, speakerTagWithSuffix);

    const auto speakerTagEndIndex = speakerTagWithSuffix.find('>');
    Y_ENSURE(speakerTagEndIndex != TStringBuf::npos, "Cannot find end of speaker tag");

    speakerTagWithSuffix.SplitAt(speakerTagEndIndex + 1, speakerTag, outputSpeech);
    return true;
}

void ForEachAttributeInSpeakerTag(TStringBuf speakerTag, std::function<void(const TStringBuf name, const TStringBuf value)> onSpeakerTagAttribute) {
    Y_ENSURE(speakerTag.StartsWith(SpeakerTagStart), "Failed to find speaker tag start");
    Y_ENSURE(speakerTag.EndsWith('>'), "Failed to find speaker tag end");

    speakerTag = speakerTag.Skip(SpeakerTagStart.size());

    while (true) {
        speakerTag = speakerTag.Skip(speakerTag.find_first_not_of(' '));
        if (speakerTag.StartsWith('>')) {
            break;
        }

        TStringBuf attributeName, attributeNameSuffix;
        speakerTag.Split('=', attributeName, attributeNameSuffix);

        Y_ENSURE(attributeNameSuffix.StartsWith('"'), "Failed to find attribute value starting quote");
        attributeNameSuffix = attributeNameSuffix.Skip(1);

        TStringBuf attributeValue, attributeValueSuffix;
        Y_ENSURE(attributeNameSuffix.TrySplit('"', attributeValue, attributeValueSuffix), "Failed to find attribute value ending quote");

        onSpeakerTagAttribute(attributeName, attributeValue);
        speakerTag = attributeValueSuffix;
    }
}

} // namespace

TOutputSpeechModifier::TOutputSpeechModifier(TString replacementSpeaker)
    : ReplacementSpeaker_(std::move(replacementSpeaker))
{
    ForEachAttributeInSpeakerTag(ReplacementSpeaker_, [this](const TStringBuf name, const TStringBuf) {
        ReplacementSpeakerAttributes_.emplace(name);
    });
}

void TOutputSpeechModifier::ModifyOutputSpeech(TString& outputSpeech) const {
    TStringBuilder result;

    TStringBuf outputSpeechBuf = outputSpeech;
    TStringBuf plainSpeechChunk, speakerTag;
    bool voiceSpeakerAlreadyAdded = false;

    while (TryFindNextSpeakerTag(outputSpeechBuf, plainSpeechChunk, speakerTag)) {
        AppendPlainSpeech(plainSpeechChunk, result, voiceSpeakerAlreadyAdded);
        ReplaceSpeakerTagAttributes(speakerTag, result, voiceSpeakerAlreadyAdded);
    }
    AppendPlainSpeech(plainSpeechChunk, result, voiceSpeakerAlreadyAdded);

    outputSpeech = static_cast<TString>(result);
}

void TOutputSpeechModifier::AppendPlainSpeech(const TStringBuf plainSpeech, TStringBuilder& result, bool& voiceSpeakerAlreadyAdded) const {
    if (plainSpeech.Empty()) {
        return;
    }

    if (!voiceSpeakerAlreadyAdded) {
        result << ReplacementSpeaker_;
        voiceSpeakerAlreadyAdded = true;
    }

    result << plainSpeech;
}

void TOutputSpeechModifier::ReplaceSpeakerTagAttributes(const TStringBuf speakerTag, TStringBuilder& result, bool& voiceSpeakerAlreadyAdded) const {
    if (!speakerTag.Contains(VoiceAttributePattern)) {
        result << speakerTag;
        return;
    }

    result << TStringBuf(ReplacementSpeaker_).Head(ReplacementSpeaker_.size() - 1); // remove '>'
    ForEachAttributeInSpeakerTag(speakerTag, [this, &result](const TStringBuf name, const TStringBuf value){
        if (!ReplacementSpeakerAttributes_.contains(name)) {
            result << ' ' << name << '=' << '"' << value << '"';
        }
    });
    result << '>';
    voiceSpeakerAlreadyAdded = true;
}

}
