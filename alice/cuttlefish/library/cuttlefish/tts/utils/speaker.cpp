#include "speaker.h"
#include "cloud_speaker.h"
#include "voicetech_speaker.h"

using namespace NAlice::NCuttlefish::NTtsUtils;

TSpeakerPtr NAlice::NCuttlefish::NTtsUtils::CreateSpeaker(const TDefaultParams& params) {
    auto* voicetechSpeaker = FindVoicetechSpeaker(params.Voice);
    if (voicetechSpeaker) {
        return MakeHolder<TVoicetechSpeakerWrap>(params, *voicetechSpeaker);
    }

    if (TCloudSpeakerWrap::VoiceIsCorrect(params.Voice)) {
        return MakeHolder<TCloudSpeakerWrap>(params);
    }
    return nullptr;
}
