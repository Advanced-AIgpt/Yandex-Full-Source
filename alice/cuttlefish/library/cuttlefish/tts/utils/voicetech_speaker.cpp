#include "voicetech_speaker.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <util/charset/utf8.h>

#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <util/generic/hash_set.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/system/byteorder.h>

using namespace NAlice::NCuttlefish::NTtsUtils;
using namespace NAlice::NCuttlefish::NTtsVoicetechSpeakersTesting;

// Gender
const TString TVoicetechSpeaker::GENDER_FEMALE = "female";
const TString TVoicetechSpeaker::GENDER_MALE = "male";

// Emotion
const TString TVoicetechSpeaker::EMOTION_EVIL = "evil";
const TString TVoicetechSpeaker::EMOTION_GOOD = "good";
const TString TVoicetechSpeaker::EMOTION_NEUTRAL = "neutral";

// Mode
const TString TVoicetechSpeaker::MODE_CPU = "cpu";
const TString TVoicetechSpeaker::MODE_GPU = "gpu";
const TString TVoicetechSpeaker::MODE_GPU_OKSANA = "gpu_oksana";
const TString TVoicetechSpeaker::MODE_GPU_VALTZ = "gpu_valtz";

// Lang
const TString TVoicetechSpeaker::LANG_EN = "en";
const TString TVoicetechSpeaker::LANG_RU = "ru";
const TString TVoicetechSpeaker::LANG_TR = "tr";
const TString TVoicetechSpeaker::LANG_UK = "uk";
const TString TVoicetechSpeaker::LANG_AR = "ar";

const TString DEFAULT_LANG = "ru";

namespace {

    struct TVoicetechSpeakerImpl : public TVoicetechSpeaker {
        TVoicetechSpeakerImpl& SetLang(const TString& lang) {
            Lang = lang;
            return *this;
        }
        TVoicetechSpeakerImpl& SetMode(const TString& mode) {
            Mode = mode;
            return *this;
        }
        TVoicetechSpeakerImpl& SetCoreVoice(bool b) {
            CoreVoice = b;
            return *this;
        }
        TVoicetechSpeakerImpl& SetDisplayName(const TString& s) {
            DisplayName = s;
            return *this;
        }
        TVoicetechSpeakerImpl& ClearVoices() {
            Voices.Clear();
            return *this;
        }
        TVoicetechSpeakerImpl& AddVoice(const TString& name, double weight = 1.0) {
            auto& param = *Voices.Add();
            param.set_name(name);
            param.set_weight(weight);
            return *this;
        }
        TVoicetechSpeakerImpl& ClearEmotions() {
            Emotions.Clear();
            return *this;
        }
        TVoicetechSpeakerImpl& AddEmotion(const TString& name = EMOTION_NEUTRAL, double weight = 1.0) {
            auto& param = *Emotions.Add();
            param.set_name(name);
            param.set_weight(weight);
            return *this;
        }
        TVoicetechSpeakerImpl& ClearGenders() {
            Genders.Clear();
            return *this;
        }
        TVoicetechSpeakerImpl& AddGender(const TString& name, double weight = 1.0) {
            auto& param = *Genders.Add();
            param.set_name(name);
            param.set_weight(weight);
            return *this;
        }
        TVoicetechSpeakerImpl& SetFallback(const TString& voice) {
            Fallback = voice;
            return *this;
        }
        TVoicetechSpeakerImpl& SetMdsThreshold(float v) {
            MdsThreshold = v;
            return *this;
        }
        TVoicetechSpeakerImpl& SetLf0Postfilter(float v) {
            Lf0Postfilter = v;
            return *this;
        }
        TVoicetechSpeakerImpl& SetIsLegacy(bool isLegacy) {
            IsLegacy = isLegacy;
            return *this;
        }
        void AddVoice(EGender gender, const TString& voice, const TString& displayName, const TString& weightedVoice) {
            Gender = gender;
            Voice = voice;
            DisplayName = displayName;
            AddVoice(weightedVoice);
            AddEmotion();  // neutral = 1
            if (gender == GenderFemale) {
                AddGender(GENDER_FEMALE);
            } else {
                AddGender(GENDER_MALE);
            }
        }
        NJson::TJsonValue ToJson(bool usePythonUniproxyFormat = false) const {
            NJson::TJsonValue result;

            result["gender"] = static_cast<i32>(Gender);
            if (!usePythonUniproxyFormat || !Lang.empty()) {
                result["lang"] = Lang;
            }
            result["displayName"] = DisplayName;

            if (!usePythonUniproxyFormat || Mode != MODE_CPU) {
                result["mode"] = Mode;
            }

            if (!usePythonUniproxyFormat || !CoreVoice) {
                result["coreVoice"] = CoreVoice;
            }
            result["sampleRate"] = SampleRate;

            static auto weightedParamsToJson = [](const ::NProtoBuf::RepeatedPtrField<TTS::Generate::WeightedParam> params) {
                NJson::TJsonValue result = NJson::TJsonArray();
                for (const auto& param : params) {
                    NJson::TJsonValue paramJson;
                    paramJson["name"] = param.name();
                    paramJson["weight"] = param.weight();
                    result.AppendValue(paramJson);
                }
                return result;
            };

            if (!usePythonUniproxyFormat || !Voices.empty()) {
                result["voices"] = weightedParamsToJson(Voices);
            }
            if (!usePythonUniproxyFormat || !Emotions.empty()) {
                result["emotions"] = weightedParamsToJson(Emotions);
            }
            if (!usePythonUniproxyFormat || !Genders.empty()) {
                result["genders"] = weightedParamsToJson(Genders);
            }
            if (!usePythonUniproxyFormat || !Fallback.empty()) {
                result["fallback"] = Fallback;
            }

            if (!usePythonUniproxyFormat || MdsThreshold.Defined() || Lf0Postfilter.Defined()) {
                result["tuning"] = NJson::TJsonArray();
                NJson::TJsonValue& tuning = result["tuning"];
                if (MdsThreshold.Defined()) {
                    NJson::TJsonValue paramJson;
                    paramJson["name"] = "msd_threshold";
                    paramJson["weight"] = *MdsThreshold;
                    tuning.AppendValue(paramJson);
                }
                if (Lf0Postfilter.Defined()) {
                    NJson::TJsonValue paramJson;
                    paramJson["name"] = "lf0_postfilter";
                    paramJson["weight"] = *Lf0Postfilter;
                    tuning.AppendValue(paramJson);
                }
            }

            if (!usePythonUniproxyFormat) {
                result["is_legacy"] = IsLegacy;
            }

            return result;
        }
    };

    TString DisplayNameFromVoice(const TString& voice) {
        // uppercase first char & all after '.'
        TString displayName(voice);
        bool isTail = false;
        for (size_t i = 0; i < displayName.size(); ++i) {
            if (i == 0 || isTail) {
                displayName[i] = AsciiToUpper(displayName[i]);
            } else {
                if (displayName[i] == '.') {
                    isTail = true;
                }
            }
        }
        return displayName;
    }

    TString WeghtedVoiceFromVoice(const TString& voice) {
        // cut suffix (beginned with '.')
        auto s = TStringBuf(voice).Before('.');
        if (s.size() == voice.size()) {
            return voice;
        }

        return ToString(s);
    }

    TString FallbackVoiceFromVoice(const TString& voice) {
        // cut suffix (beginned with '.')
        auto s = TStringBuf(voice).Before('.');
        if (s.size() == voice.size()) {
            return voice;
        }

        return ToString(s);
    }

    class TVoicetechSpeakers {
    public:
        TVoicetechSpeakers() {
            const TString noFallback;
            const auto female = TVoicetechSpeaker::GenderFemale;
            const auto male = TVoicetechSpeaker::GenderMale;

            // hack for testing?
            AddSpeaker(female, "fallback2jane")
                .SetDisplayName("Fallback2Jane")
                .ClearVoices().AddVoice("no_such_voice")
                .SetLang(TVoicetechSpeaker::LANG_RU)
                .SetFallback("jane")
                .SetCoreVoice(false)
                .SetIsLegacy(true);

            AddSpeaker(female, "selay.gpu")
                .SetLang(TVoicetechSpeaker::LANG_TR)
                .SetMode(TVoicetechSpeaker::MODE_GPU)
                .SetFallback("silaerkan");
            AddSpeaker(female, "shitova.gpu")  // <<<<<< main Alice voice
                .SetLang(TVoicetechSpeaker::LANG_RU)
                .SetMode(TVoicetechSpeaker::MODE_GPU)
                .SetFallback("shitova.us");
            AddSpeaker(female, "shitova_whisper.gpu")
                .ClearVoices().AddVoice("shitova")
                .SetLang(TVoicetechSpeaker::LANG_RU)
                .SetMode(TVoicetechSpeaker::MODE_GPU)
                .SetFallback("shitova.gpu");
            AddSpeaker(female, "fairy_tales")
                .ClearVoices().AddVoice("fairy_tales")
                .SetLang(TVoicetechSpeaker::LANG_RU)
                .SetMode(TVoicetechSpeaker::MODE_GPU)
                .SetFallback("shitova.gpu");
            AddSpeaker(female, "vtb_brand_voice.cloud")
                .ClearVoices().AddVoice("cloud_vtb_brand_voice")
                .SetLang(TVoicetechSpeaker::LANG_RU)
                .SetMode(TVoicetechSpeaker::MODE_GPU);
            AddSpeaker(male, "arabic.gpu")
                .SetLang(TVoicetechSpeaker::LANG_AR)
                .SetMode(TVoicetechSpeaker::MODE_GPU);

            TVector<TVoicetechSpeakerImpl*> oksanaMultiSpeakers = {
                &AddSpeaker(female, "oksana.gpu"),
                &AddSpeaker(male, "anton_samokhvalov.gpu").SetDisplayName("Samokhvalov.GPU"),
                &AddSpeaker(male, "ermil.gpu").SetDisplayName("Ermilov.GPU"),
                &AddSpeaker(female, "jane.gpu").ClearEmotions().AddEmotion(TVoicetechSpeaker::EMOTION_GOOD),
                &AddSpeaker(male, "kolya.gpu"),
                &AddSpeaker(male, "kostya.gpu"),
                &AddSpeaker(male, "krosh.gpu"),
                &AddSpeaker(female, "nastya.gpu"),
                &AddSpeaker(female, "omazh.gpu").SetDisplayName("Mozhara.GPU"),
                &AddSpeaker(female, "sasha.gpu"),
                &AddSpeaker(female, "tatyana_abramova.gpu").SetDisplayName("Abramova.GPU"),
                &AddSpeaker(male, "zahar.gpu")
            };
            for (auto s : oksanaMultiSpeakers) {
                s->SetLang(TVoicetechSpeaker::LANG_RU);
                s->SetMode(TVoicetechSpeaker::MODE_GPU_OKSANA);
                s->SetFallback(FallbackVoiceFromVoice(s->Voice));
            }

            AddSpeaker(male, "valtz.gpu")
                .SetLang(TVoicetechSpeaker::LANG_RU)
                .SetMode(TVoicetechSpeaker::MODE_GPU_VALTZ)
                .SetFallback("valtz");

            TVector<TVoicetechSpeakerImpl*> legacySpeakers = {
                // Legacy/CPU speakers
                &AddSpeaker(male, "krosh")
                    .SetLang(TVoicetechSpeaker::LANG_RU),
                &AddSpeaker(female, "assistant")
                    .SetLang(TVoicetechSpeaker::LANG_RU)
                    .ClearVoices().AddVoice("shitova"),
                &AddSpeaker(female, "shitova.us")
                    .SetLang(TVoicetechSpeaker::LANG_RU),
                &AddSpeaker(female, "shitova")
                    .SetLang(TVoicetechSpeaker::LANG_RU),
                &AddSpeaker(male, "valtz.us")
                    .SetLang(TVoicetechSpeaker::LANG_RU)
                    .ClearVoices().AddVoice("valtz.us"),
                &AddSpeaker(male, "valtz")
                    .SetLang(TVoicetechSpeaker::LANG_RU),
                &AddSpeaker(male, "levitan"),
                &AddSpeaker(male, "ermil")
                    .SetDisplayName("Ermilov"),
                &AddSpeaker(male, "zahar"),
                &AddSpeaker(female, "silaerkan"),
                &AddSpeaker(female, "oksana"),
                &AddSpeaker(female, "good_oksana")
                    .SetDisplayName("Good Oksana")
                    .ClearVoices().AddVoice("oksana")
                    .ClearEmotions().AddEmotion(TVoicetechSpeaker::EMOTION_GOOD),
                &AddSpeaker(female, "oksana.en")
                    .SetLang(TVoicetechSpeaker::LANG_EN),
                &AddSpeaker(female, "lj.gpu")
                    .SetDisplayName("LJ.GPU")
                    .ClearVoices().AddVoice("lj")
                    .ClearEmotions().AddEmotion(TVoicetechSpeaker::EMOTION_NEUTRAL)
                    .SetLang(TVoicetechSpeaker::LANG_EN)
                    .SetMode(TVoicetechSpeaker::MODE_GPU),
                &AddSpeaker(male, "david.gpu")
                    .SetDisplayName("David_en.GPU")
                    .ClearVoices().AddVoice("david_en")
                    .ClearEmotions().AddEmotion(TVoicetechSpeaker::EMOTION_NEUTRAL)
                    .SetLang(TVoicetechSpeaker::LANG_EN)
                    .SetMode(TVoicetechSpeaker::MODE_GPU),
                &AddSpeaker(female, "jane")
                    .ClearEmotions().AddEmotion(TVoicetechSpeaker::EMOTION_GOOD),
                &AddSpeaker(female, "omazh")
                    .SetDisplayName("Mozhara"),
                &AddSpeaker(male, "kolya"),
                &AddSpeaker(male, "kostya"),
                &AddSpeaker(female, "nastya"),
                &AddSpeaker(female, "sasha"),
                &AddSpeaker(male, "nick"),
                &AddSpeaker(male, "erkanyavas"),
                &AddSpeaker(male, "zhenya"),
                &AddSpeaker(female, "tanya"),
                &AddSpeaker(male, "anton_samokhvalov")
                    .SetDisplayName("Samokhvalov"),
                &AddSpeaker(female, "tatyana_abramova")
                    .SetDisplayName("Abramova"),
                &AddSpeaker(female, "alyss"),
                &AddSpeaker(female, "voicesearch")
                    .SetDisplayName("VoiceSearch")
                    .ClearVoices().AddVoice("omazh")
                    .SetCoreVoice(false),

                // Voices generated by tuning/mixing from exit
                &AddSpeaker(male, "ermil_with_tuning")
                    .SetDisplayName("Ermilla")
                    .ClearVoices().AddVoice("ermil")
                    .ClearEmotions().AddEmotion(TVoicetechSpeaker::EMOTION_NEUTRAL)
                    .ClearGenders().AddGender(TVoicetechSpeaker::GENDER_FEMALE, 2.)
                    .SetCoreVoice(false),
                &AddSpeaker(male, "ermilov")
                    .ClearVoices().AddVoice("ermil")
                    .SetCoreVoice(false),
                &AddSpeaker(male, "robot")
                    .SetDisplayName("Робот Захар")
                    .ClearVoices().AddVoice("ermil")
                    .ClearEmotions().AddEmotion(TVoicetechSpeaker::EMOTION_NEUTRAL).AddEmotion(TVoicetechSpeaker::EMOTION_GOOD).AddEmotion(TVoicetechSpeaker::EMOTION_EVIL)
                    .SetLf0Postfilter(0)
                    .SetCoreVoice(false),
                &AddSpeaker(male, "dude")
                    .ClearVoices().AddVoice("ermil").AddVoice("zahar")
                    .ClearEmotions().AddEmotion(TVoicetechSpeaker::EMOTION_EVIL, 5.)
                    .ClearGenders().AddGender(TVoicetechSpeaker::GENDER_MALE, 2.)
                    .SetCoreVoice(false),
                &AddSpeaker(male, "zombie")
                    .ClearVoices().AddVoice("ermil").AddVoice("zahar")
                    .ClearEmotions().AddEmotion(TVoicetechSpeaker::EMOTION_GOOD, 5.)
                    .ClearGenders().AddGender(TVoicetechSpeaker::GENDER_FEMALE, 2.)
                    .SetCoreVoice(false),
                &AddSpeaker(male, "smoky")
                    .ClearVoices().AddVoice("ermil")
                    .ClearEmotions().AddEmotion(TVoicetechSpeaker::EMOTION_GOOD, 1.)
                    .ClearGenders()
                    .SetMdsThreshold(1)
                    .SetCoreVoice(false),
            };
            for (auto s : legacySpeakers) {
                s->SetIsLegacy(true);
            }

        }

        const TVoicetechSpeakerImpl* FindVoice(const TString& voice) const {
            auto it = Voices_.find(voice);
            return it == Voices_.end() ? nullptr : &it->second;
        }

        TVector<TString> GetVoiceList() const {
            TVector<TString> voices;
            for (const auto& [voice, _] : Voices_) {
                voices.push_back(voice);
            }

            Sort(voices.begin(), voices.end());
            return voices;
        }

        NJson::TJsonValue ToJson(bool usePythonUniproxyFormat = false) const {
            NJson::TJsonValue result;
            for (const auto& [voice, speaker] : Voices_) {
                result[voice] = speaker.ToJson(usePythonUniproxyFormat);
            }
            return result;
        }

    private:
        TVoicetechSpeakerImpl& AddSpeaker(TVoicetechSpeaker::EGender gender, const TString& voice) {
            auto& speaker = Voices_[voice];
            TString displayName = DisplayNameFromVoice(voice);
            TString weightedVoice = WeghtedVoiceFromVoice(voice);
            speaker.AddVoice(gender, voice, displayName, weightedVoice);

            return speaker;
        }

        THashMap<TString, TVoicetechSpeakerImpl> Voices_;
    };

    const TVoicetechSpeakers& VoicetechSpeakers() {
        return *Singleton<TVoicetechSpeakers>();
    }
}

TString TVoicetechSpeaker::GetTtsBackendRequestItemTypeForLang(const TString& lang) const {
    return TStringBuilder()
        << NAlice::NCuttlefish::ITEM_TYPE_TTS_BACKEND_REQUEST_PREFIX
        << (Lang.empty() ? lang : Lang) << '_'
        << Mode << '_'
        << Voice
    ;
}

const TVoicetechSpeaker* NAlice::NCuttlefish::NTtsUtils::FindVoicetechSpeaker(const TString& voice) {
    return VoicetechSpeakers().FindVoice(voice);
}

const TVector<TString>& NAlice::NCuttlefish::NTtsVoicetechSpeakersTesting::GetVoiceList() {
    static const TVector<TString> voiceList = VoicetechSpeakers().GetVoiceList();
    return voiceList;
}

const NJson::TJsonValue& NAlice::NCuttlefish::NTtsVoicetechSpeakersTesting::GetSpeakersJson() {
    static const NJson::TJsonValue speakersJson = VoicetechSpeakers().ToJson();
    return speakersJson;
}

const NJson::TJsonValue& NAlice::NCuttlefish::NTtsVoicetechSpeakersTesting::GetSpeakersJsonPythonUniproxyFormat() {
    static const NJson::TJsonValue speakersJson = VoicetechSpeakers().ToJson(/* usePythonUniproxyFormat = */ true);
    return speakersJson;
}

TVoicetechSpeakerWrap::TVoicetechSpeakerWrap(const TDefaultParams& params, const TVoicetechSpeaker& speaker)
    : Speaker(speaker) {
    Params = params;
}

void TVoicetechSpeakerWrap::FillGenerateRequest(TTS::Generate& req) {
    req.set_lang(Speaker.Lang ? Speaker.Lang : Params.Lang);
    if (!Params.Emotion.empty()) {
        auto* emotionParam = req.mutable_emotions()->Add();
        emotionParam->set_name(Params.Emotion);
        emotionParam->set_weight(1.0);
    } else {
        // Use default emotion params for this speaker
        *req.mutable_emotions() = Speaker.Emotions;
    }

    *req.mutable_voices() = Speaker.Voices;
    *req.mutable_genders() = Speaker.Genders;
    if (Speaker.MdsThreshold.Defined()) {
        req.set_msd_threshold(*Speaker.MdsThreshold);
    }
    if (Speaker.Lf0Postfilter.Defined()) {
        req.set_lf0_postfilter(*Speaker.Lf0Postfilter);
    }
}

TString TVoicetechSpeakerWrap::GetTtsBackendRequestItemType() {
    return Speaker.GetTtsBackendRequestItemTypeForLang(Params.Lang);
}
