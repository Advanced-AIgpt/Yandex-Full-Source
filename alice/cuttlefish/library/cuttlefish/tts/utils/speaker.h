#pragma once

#include <alice/cuttlefish/library/protos/tts.pb.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

// Utils with inherit legacy logic from python_uniproxy
namespace NAlice::NCuttlefish::NTtsUtils {
    struct TDefaultParams {
        TString Voice;
        TString Lang;
        TString Emotion;
    };

    class ISpeaker {
    public:
        virtual ~ISpeaker() = default;
        virtual TString GetTtsBackendRequestItemType() = 0;
        virtual void FillGenerateRequest(TTS::Generate&) = 0;

    protected:
        TDefaultParams Params;
    };

    using TSpeakerPtr = THolder<ISpeaker>;

    TSpeakerPtr CreateSpeaker(const TDefaultParams&);
}
