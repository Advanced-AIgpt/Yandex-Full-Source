#pragma once

#include <util/generic/hash_set.h>
#include <util/string/builder.h>

namespace NAlice::NHollywood::NModifiers {

class TOutputSpeechModifier {
public:
    explicit TOutputSpeechModifier(TString replacementSpeaker);
    void ModifyOutputSpeech(TString& outputSpeech) const;
private:
    void AppendPlainSpeech(const TStringBuf plainSpeech, TStringBuilder& result, bool& voiceSpeakerAlreadyAdded) const;
    void ReplaceSpeakerTagAttributes(const TStringBuf speakerTag, TStringBuilder& result, bool& voiceSpeakerAlreadyAdded) const;
private:
    TString ReplacementSpeaker_;
    THashSet<TStringBuf> ReplacementSpeakerAttributes_;
};

}
