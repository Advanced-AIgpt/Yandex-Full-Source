#pragma once

#include <algorithm>

#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/protos/common/events.pb.h>

#include <util/generic/vector.h>
#include <util/string/join.h>

namespace NAlice {

class TVoiceInputEvent: public IEvent {
public:
    explicit TVoiceInputEvent(const TEvent& event)
        : IEvent(event)
    {
        if (event.AsrResultSize() == 0) {
            return;
        }

        const auto& bestAsrResult = event.GetAsrResult(0);
        if (bestAsrResult.WordsSize() > 0) {
            TVector<TString> words;
            for(auto& word: bestAsrResult.GetWords()) {
                words.push_back(word.GetValue());
            }
            Utterance_ = JoinSeq(" ", words);
        } else {
            Utterance_ = bestAsrResult.GetNormalized();
        }

        if (bestAsrResult.HasNormalized()) {
            AsrNormalizedUtterance_ = bestAsrResult.GetNormalized();
        }
        IsWhisper_ = event.GetAsrWhisper();
    }

    const TString& GetUtterance() const override {
        return Utterance_;
    }

    TStringBuf GetAsrNormalizedUtterance() const override {
        if (AsrNormalizedUtterance_) {
            return *AsrNormalizedUtterance_;
        }
        return {};
    }

    virtual bool IsVoiceInput() const override {
        return true;
    }

    bool HasUtterance() const override {
        return true;
    }

    bool HasAsrNormalizedUtterance() const override {
        return AsrNormalizedUtterance_.Defined();
    }

    bool IsAsrWhisper() const override {
        return IsWhisper_;
    }

    void FillScenarioInput(const TMaybe<TString>& normalizedUtterance, NScenarios::TInput* input) const override {
        auto* voice = input->MutableVoice();
        *voice->MutableAsrData() = SpeechKitEvent().GetAsrResult();
        for (auto& asrResult : *voice->MutableAsrData()) {
            if (const auto& normalized = asrResult.GetNormalized(); !normalized.empty()) {
                asrResult.SetUtterance(normalized);
            }
        }

        if (normalizedUtterance.Defined()) {
            voice->SetUtterance(normalizedUtterance.GetRef());
        }
        if (SpeechKitEvent().HasBiometryScoring()) {
            *voice->MutableBiometryScoring() = SpeechKitEvent().GetBiometryScoring();
        }
        if (SpeechKitEvent().HasBiometryClassification()) {
            *voice->MutableBiometryClassification() = SpeechKitEvent().GetBiometryClassification();
        }
    }

private:
    bool IsWhisper_ = false;
    TString Utterance_;
    TMaybe<TString> AsrNormalizedUtterance_;
};

} // namespace NAlice
