#pragma once

#include "speaker.h"

namespace NAlice::NCuttlefish::NTtsUtils {
    class TCloudSpeakerWrap : public ISpeaker {
    public:
        explicit TCloudSpeakerWrap(const TDefaultParams& params) {
            Params = params;
        }

        static bool VoiceIsCorrect(const TString& voice) {
            return voice.StartsWith("cloud_");
        }

        void FillGenerateRequest(TTS::Generate&) override;

        TString GetTtsBackendRequestItemType() override;
    };
};
