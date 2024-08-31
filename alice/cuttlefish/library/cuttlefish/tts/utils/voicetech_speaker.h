#pragma once

#include "speaker.h"

#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>

namespace NAlice::NCuttlefish::NTtsUtils {
    extern const TString DEFAULT_LANG;

    struct TVoicetechSpeaker {
        // Gender
        static const TString GENDER_FEMALE;
        static const TString GENDER_MALE;

        // Emotion
        static const TString EMOTION_EVIL;
        static const TString EMOTION_GOOD;
        static const TString EMOTION_NEUTRAL;

        // Mode
        static const TString MODE_CPU;
        static const TString MODE_GPU;
        static const TString MODE_GPU_OKSANA;
        static const TString MODE_GPU_VALTZ;

        // Lang
        static const TString LANG_EN;
        static const TString LANG_RU;
        static const TString LANG_TR;
        static const TString LANG_UK;
        static const TString LANG_AR;

        enum EGender {
            GenderFemale = 1,
            GenderMale = 2,
        };
        EGender Gender = GenderFemale;
        // Index voice
        TString Voice;
        TString Lang;
        TString DisplayName;
        TString Mode = MODE_CPU;
        bool CoreVoice = true;
        ui32 SampleRate = 48000;
        ::NProtoBuf::RepeatedPtrField<TTS::Generate::WeightedParam> Voices;
        ::NProtoBuf::RepeatedPtrField<TTS::Generate::WeightedParam> Emotions;
        ::NProtoBuf::RepeatedPtrField<TTS::Generate::WeightedParam> Genders;
        TString Fallback;
        TMaybe<float> MdsThreshold;
        TMaybe<float> Lf0Postfilter;
        // Old/legacy cpu tts-server is used for generation
        bool IsLegacy = false;

        TString GetTtsBackendRequestItemTypeForLang(const TString& lang) const;
    };

    class TVoicetechSpeakerWrap : public ISpeaker {
    public:
        TVoicetechSpeakerWrap(const TDefaultParams&, const TVoicetechSpeaker&);

        void FillGenerateRequest(TTS::Generate&) override;
        TString GetTtsBackendRequestItemType() override;

    private:
        const TVoicetechSpeaker& Speaker;
    };

    const TVoicetechSpeaker* FindVoicetechSpeaker(const TString& voice);
};

namespace NAlice::NCuttlefish::NTtsVoicetechSpeakersTesting {
    const TVector<TString>& GetVoiceList();

    // Dump speakers in json format (almost same as it was in config["ttsserver"]["voices"])
    const NJson::TJsonValue& GetSpeakersJson();

    // Dump speakers in json format (totally same as it was in config["ttsserver"]["voices"])
    const NJson::TJsonValue& GetSpeakersJsonPythonUniproxyFormat();
};
