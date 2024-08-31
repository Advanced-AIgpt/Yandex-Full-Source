#include "builder.h"

#include <alice/megamind/protos/analytics/modifiers/colored_speaker/colored_speaker.pb.h>
#include <alice/megamind/protos/analytics/modifiers/conjugator/conjugator.pb.h>
#include <alice/megamind/protos/analytics/modifiers/polyglot/polyglot.pb.h>
#include <alice/megamind/protos/analytics/modifiers/voice_doodle/voice_doodle.pb.h>
#include <alice/megamind/protos/analytics/modifiers/whisper/whisper.pb.h>

namespace NAlice::NHollywood::NModifiers {

TModifierAnalyticsInfoBuilder& TModifierAnalyticsInfoBuilder::SetColoredSpeaker(const TString& prevResponseTts,
                                                                                const TString& newResponseTts) {
    auto& coloredSpeaker = *Proto.MutableColoredSpeaker();
    coloredSpeaker.SetPrevResponseTts(prevResponseTts);
    coloredSpeaker.SetNewResponseTts(newResponseTts);
    return *this;
}

TModifierAnalyticsInfoBuilder& TModifierAnalyticsInfoBuilder::SetVoiceDoodle(const TString& prevResponseTts,
                                                                             const TString& newResponseTts) {
    auto& voiceDoodle = *Proto.MutableVoiceDoodle();
    voiceDoodle.SetPrevResponseTts(prevResponseTts);
    voiceDoodle.SetNewResponseTts(newResponseTts);
    return *this;
}

TModifierAnalyticsInfoBuilder& TModifierAnalyticsInfoBuilder::SetWhisper(bool isWhisperTagApplied,
                                                                         bool isSoundSetLevelDirectiveApplied) {
    auto& whisper = *Proto.MutableWhisper();
    whisper.SetIsWhisperTagApplied(isWhisperTagApplied);
    whisper.SetIsSoundSetLevelDirectiveApplied(isSoundSetLevelDirectiveApplied);
    return *this;
}

TModifierAnalyticsInfoBuilder& TModifierAnalyticsInfoBuilder::SetConjugator(::NAlice::NModifiers::NConjugator::TConjugator&& conjugatorAnalyticsInfo) {
    *Proto.MutableConjugator() = std::move(conjugatorAnalyticsInfo);
    return *this;
}

TModifierAnalyticsInfoBuilder& TModifierAnalyticsInfoBuilder::SetPolyglot(
    const size_t translatedPhrasesCount, const size_t uniqueTranslatedPhrasesCount) {
    auto& polyglot = *Proto.MutablePolyglot();
    polyglot.SetTranslatedPhrasesCount(translatedPhrasesCount);
    polyglot.SetUniqueTranslatedPhrasesCount(uniqueTranslatedPhrasesCount);
    return *this;
}

TModifierAnalyticsInfoBuilder::TAnalyticsInfo TModifierAnalyticsInfoBuilder::MoveProto() && {
    return std::move(Proto);
}

} // namespace NAlice::NHollywood::NModifiers
