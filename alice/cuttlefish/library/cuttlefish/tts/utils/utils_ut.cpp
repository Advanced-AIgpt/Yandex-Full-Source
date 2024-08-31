#include "utils.h"
#include "speaker.h"
#include "voicetech_speaker.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/resource/resource.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NCuttlefish::NTtsUtils;
using namespace NAlice::NCuttlefish::NTtsVoicetechSpeakersTesting;

class TCuttlefishTtsUtilsTest : public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishTtsUtilsTest);
    UNIT_TEST(TestFindVoicetechSpeaker);
    UNIT_TEST(TestGetTtsBackendRequestItemTypeForLang)
    UNIT_TEST(TestAllSpeakersConfigsAreSame);
    UNIT_TEST(TestTryNormalizeLang);
    UNIT_TEST(TestFormatToMime);
    UNIT_TEST(TestVoiceQualityFromString);
    UNIT_TEST(TestVoiceQualityToString);
    UNIT_TEST_SUITE_END();

public:
    void TestFindVoicetechSpeaker() {
        {
            auto speaker = FindVoicetechSpeaker("xxx");
            UNIT_ASSERT(!speaker);
        }
        {
            auto speaker = FindVoicetechSpeaker("fallback2jane");
            UNIT_ASSERT(speaker);
            UNIT_ASSERT_VALUES_EQUAL(int(speaker->Gender), int(TVoicetechSpeaker::GenderFemale));
            UNIT_ASSERT_VALUES_EQUAL(speaker->SampleRate, 48000);
            UNIT_ASSERT_VALUES_EQUAL(speaker->DisplayName, "Fallback2Jane");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Lang, TVoicetechSpeaker::LANG_RU);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Mode, TVoicetechSpeaker::MODE_CPU);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Fallback, "jane");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices[0].name(), "no_such_voice");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Voices[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions[0].name(), "neutral");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Emotions[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders[0].name(), "female");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Genders[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->IsLegacy, true);
        }
        {
            auto speaker = FindVoicetechSpeaker("selay.gpu");
            UNIT_ASSERT(speaker);
            UNIT_ASSERT_VALUES_EQUAL(int(speaker->Gender), int(TVoicetechSpeaker::GenderFemale));
            UNIT_ASSERT_VALUES_EQUAL(speaker->SampleRate, 48000);
            UNIT_ASSERT_VALUES_EQUAL(speaker->DisplayName, "Selay.GPU");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Lang, TVoicetechSpeaker::LANG_TR);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Mode, TVoicetechSpeaker::MODE_GPU);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Fallback, "silaerkan");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices[0].name(), "selay");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Voices[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions[0].name(), "neutral");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Emotions[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders[0].name(), "female");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Genders[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->IsLegacy, false);
        }
        {
            auto speaker = FindVoicetechSpeaker("shitova.gpu");
            UNIT_ASSERT(speaker);
            UNIT_ASSERT_VALUES_EQUAL(int(speaker->Gender), int(TVoicetechSpeaker::GenderFemale));
            UNIT_ASSERT_VALUES_EQUAL(speaker->SampleRate, 48000);
            UNIT_ASSERT_VALUES_EQUAL(speaker->DisplayName, "Shitova.GPU");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Lang, TVoicetechSpeaker::LANG_RU);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Mode, TVoicetechSpeaker::MODE_GPU);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Fallback, "shitova.us");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices[0].name(), "shitova");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Voices[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions[0].name(), "neutral");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Emotions[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders[0].name(), "female");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Genders[0].weight(), 1.0, 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->IsLegacy, false);
        }
        {
            auto speaker = FindVoicetechSpeaker("anton_samokhvalov.gpu");
            UNIT_ASSERT(speaker);
            UNIT_ASSERT_VALUES_EQUAL(int(speaker->Gender), int(TVoicetechSpeaker::GenderMale));
            UNIT_ASSERT_VALUES_EQUAL(speaker->SampleRate, 48000);
            UNIT_ASSERT_VALUES_EQUAL(speaker->DisplayName, "Samokhvalov.GPU");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Lang, TVoicetechSpeaker::LANG_RU);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Mode, TVoicetechSpeaker::MODE_GPU_OKSANA);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Fallback, "anton_samokhvalov");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices[0].name(), "anton_samokhvalov");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Voices[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions[0].name(), "neutral");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Emotions[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders[0].name(), "male");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Genders[0].weight(), 1.0, 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->IsLegacy, false);
        }
        {
            auto speaker = FindVoicetechSpeaker("valtz.gpu");
            UNIT_ASSERT(speaker);
            UNIT_ASSERT_VALUES_EQUAL(int(speaker->Gender), int(TVoicetechSpeaker::GenderMale));
            UNIT_ASSERT_VALUES_EQUAL(speaker->SampleRate, 48000);
            UNIT_ASSERT_VALUES_EQUAL(speaker->DisplayName, "Valtz.GPU");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Lang, TVoicetechSpeaker::LANG_RU);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Mode, TVoicetechSpeaker::MODE_GPU_VALTZ);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Fallback, "valtz");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices[0].name(), "valtz");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Voices[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions[0].name(), "neutral");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Emotions[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders[0].name(), "male");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Genders[0].weight(), 1.0, 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->IsLegacy, false);
        }
        {
            auto speaker = FindVoicetechSpeaker("good_oksana");
            UNIT_ASSERT(speaker);
            UNIT_ASSERT_VALUES_EQUAL(int(speaker->Gender), int(TVoicetechSpeaker::GenderFemale));
            UNIT_ASSERT_VALUES_EQUAL(speaker->SampleRate, 48000);
            UNIT_ASSERT_VALUES_EQUAL(speaker->DisplayName, "Good Oksana");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Lang, "");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Mode, TVoicetechSpeaker::MODE_CPU);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Fallback, "");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices[0].name(), "oksana");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Voices[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions[0].name(), "good");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Emotions[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders[0].name(), "female");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Genders[0].weight(), 1.0, 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->IsLegacy, true);
        }
        {
            auto speaker = FindVoicetechSpeaker("dude");
            UNIT_ASSERT(speaker);
            UNIT_ASSERT_VALUES_EQUAL(int(speaker->Gender), int(TVoicetechSpeaker::GenderMale));
            UNIT_ASSERT_VALUES_EQUAL(speaker->SampleRate, 48000);
            UNIT_ASSERT_VALUES_EQUAL(speaker->DisplayName, "Dude");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Lang, "");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Mode, TVoicetechSpeaker::MODE_CPU);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Fallback, "");
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices.size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices[0].name(), "ermil");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Voices[0].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Voices[1].name(), "zahar");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Voices[1].weight(), 1., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Emotions[0].name(), "evil");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Emotions[0].weight(), 5., 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(speaker->Genders[0].name(), "male");
            UNIT_ASSERT_DOUBLES_EQUAL(speaker->Genders[0].weight(), 2.0, 0.00001);
            UNIT_ASSERT_VALUES_EQUAL(speaker->IsLegacy, true);
        }
    }

    void TestCreateVoicetechSpeaker() {
        auto speaker = CreateSpeaker({"shitova.gpu", "ru", "neutral"});
        UNIT_ASSERT_STRINGS_EQUAL(speaker->GetTtsBackendRequestItemType(), "tts_backend_request_ru_gpu_shitova");
        TTS::Generate req;
        speaker->FillGenerateRequest(req);
        UNIT_ASSERT_VALUES_EQUAL(req.Getvoices().size(), 1);
        UNIT_ASSERT_STRINGS_EQUAL(req.Getvoices()[0].Getname(), "shitova.gpu");
        UNIT_ASSERT_STRINGS_EQUAL(req.Getlang(), TVoicetechSpeaker::LANG_RU);
    }

    void TestCreateCloudSpeaker() {
        auto speaker = CreateSpeaker({"cloud_smth", "ru", "neutral"});
        UNIT_ASSERT_STRINGS_EQUAL(speaker->GetTtsBackendRequestItemType(), "tts_backend_request_cloud_synth");
        TTS::Generate req;
        speaker->FillGenerateRequest(req);
        UNIT_ASSERT_VALUES_EQUAL(req.Getvoices().size(), 1);
        UNIT_ASSERT_STRINGS_EQUAL(req.Getvoices()[0].Getname(), "cloud_smth");
    }

    void TestGetTtsBackendRequestItemTypeForLang() {
        {
            // Without predefined lang
            auto speaker = FindVoicetechSpeaker("dude");
            UNIT_ASSERT_VALUES_EQUAL(speaker->GetTtsBackendRequestItemTypeForLang("ru"), "tts_backend_request_ru_cpu_dude");
            UNIT_ASSERT_VALUES_EQUAL(speaker->GetTtsBackendRequestItemTypeForLang("en"), "tts_backend_request_en_cpu_dude");
            UNIT_ASSERT_VALUES_EQUAL(speaker->GetTtsBackendRequestItemTypeForLang("trash"), "tts_backend_request_trash_cpu_dude");
        }
        {
            // With predefined lang
            auto speaker = FindVoicetechSpeaker("shitova.gpu");
            UNIT_ASSERT_VALUES_EQUAL(speaker->GetTtsBackendRequestItemTypeForLang("ru"), "tts_backend_request_ru_gpu_shitova.gpu");
            UNIT_ASSERT_VALUES_EQUAL(speaker->GetTtsBackendRequestItemTypeForLang("en"), "tts_backend_request_ru_gpu_shitova.gpu");
            UNIT_ASSERT_VALUES_EQUAL(speaker->GetTtsBackendRequestItemTypeForLang("trash"), "tts_backend_request_ru_gpu_shitova.gpu");
        }
    }

    void TestAllSpeakersConfigsAreSame() {
        // TODO(chegoryu) fix or remove bad configs
        const TVector<TString> configPaths = {
            "/python_uniproxy_tts_development.json",
            // "/python_uniproxy_tts_local.json",
            // "/python_uniproxy_tts_rtc_alpha.json",
            "/python_uniproxy_tts_rtc_production.json",
            // "/python_uniproxy_tts_testing.json",
            // "/python_uniproxy_tts_ycloud.json",
            "/uniproxy2_tts_voices.json",
        };

        NJson::TJsonValue expectedVoices = GetSpeakersJsonPythonUniproxyFormat();
        UNIT_ASSERT(expectedVoices.IsMap());

        for (const auto& configPath : configPaths) {
            TString configStr = NResource::Find(configPath);

            NJson::TJsonValue config;
            UNIT_ASSERT_NO_EXCEPTION_C(NJson::ReadJsonTree(configStr, &config, /* throwOnError = */ true), "at " << configPath);
            NJson::TJsonValue voices = config["voices"];

            UNIT_ASSERT_C(voices.IsMap(), "at " << configPath);
            for (const auto& [voice, voiceConfig] : expectedVoices.GetMap()) {
                UNIT_ASSERT_C(voices.Has(voice), voice << " not found in " << configPath);
                for (auto field : {"host", "tts_balancer_handle"}) {
                    voices[voice].EraseValue(field);
                }
                UNIT_ASSERT_VALUES_EQUAL_C(voiceConfig, voices[voice], "at " << configPath << ", " << voice);
            }
            for (const auto& [voice, _] : voices.GetMap()) {
                UNIT_ASSERT_C(expectedVoices.Has(voice), voice << " not found in expected config at " << configPath);
            }

            // We can compare only expectedVoices and voices, but diff will be unreadable
            // However, let's do sanity check
            UNIT_ASSERT_VALUES_EQUAL_C(expectedVoices, voices, "at " << configPath);
        }
    }

    void TestTryNormalizeLang() {
        for (const TString prefix : {"ru", "en", "tr", "uk", "ar"}) {
            UNIT_ASSERT_VALUES_EQUAL(TryNormalizeLang(prefix), prefix);
            UNIT_ASSERT_VALUES_EQUAL(TryNormalizeLang(prefix + "_random"), prefix);
        }

        UNIT_ASSERT_VALUES_EQUAL(TryNormalizeLang(""), "ru");
        UNIT_ASSERT_VALUES_EQUAL(TryNormalizeLang("random"), Nothing());
    }

    void TestFormatToMime() {
        const TVector<NAliceProtocol::TVoiceOptions::EVoiceQuality> qualities = {
            NAliceProtocol::TVoiceOptions::DEFAULT_QUALITY,
            NAliceProtocol::TVoiceOptions::LOW,
            NAliceProtocol::TVoiceOptions::HIGH,
            NAliceProtocol::TVoiceOptions::ULTRAHIGH,
        };
        const TVector<TString> mimes = {
            "audio/opus",
            "audio/x-speex",
            "audio/x-wav",
            "audio/x-pcm;bit=16;rate=8000",
            "audio/x-pcm;bit=16;rate=16000",
            "audio/x-pcm;bit=16;rate=48000",
        };

        {
            // pcm
            UNIT_ASSERT_VALUES_EQUAL(FormatToMime("pcm", {}), "audio/x-pcm;bit=16;rate=16000");
            UNIT_ASSERT_VALUES_EQUAL(FormatToMime("pcm", NAliceProtocol::TVoiceOptions::DEFAULT_QUALITY), "audio/x-pcm;bit=16;rate=16000");

            UNIT_ASSERT_VALUES_EQUAL(FormatToMime("pcm", NAliceProtocol::TVoiceOptions::LOW), "audio/x-pcm;bit=16;rate=8000");
            UNIT_ASSERT_VALUES_EQUAL(FormatToMime("pcm", NAliceProtocol::TVoiceOptions::HIGH), "audio/x-pcm;bit=16;rate=16000");
            UNIT_ASSERT_VALUES_EQUAL(FormatToMime("pcm", NAliceProtocol::TVoiceOptions::ULTRAHIGH), "audio/x-pcm;bit=16;rate=48000");
        }

        // Quality does not affect the result of parse of other formats
        for (const auto quality : qualities) {
            UNIT_ASSERT_VALUES_EQUAL(FormatToMime("opus", quality), "audio/opus");
            UNIT_ASSERT_VALUES_EQUAL(FormatToMime("spx", quality), "audio/x-speex");
            UNIT_ASSERT_VALUES_EQUAL(FormatToMime("wav", quality), "audio/x-wav");

            // default
            UNIT_ASSERT_VALUES_EQUAL(FormatToMime("", quality), "audio/opus");
            UNIT_ASSERT_VALUES_EQUAL(FormatToMime("random", quality), "audio/opus");
        }

        // Python uniproxy legacy logic
        // Format can be mime already
        for (const auto& mime : mimes) {
            for (const auto quality : qualities) {
                UNIT_ASSERT_VALUES_EQUAL(FormatToMime(mime, quality), mime);
            }
        }
    }

    void TestVoiceQualityFromString() {
        UNIT_ASSERT_EQUAL(VoiceQualityFromString("low"), NAliceProtocol::TVoiceOptions::LOW);
        UNIT_ASSERT_EQUAL(VoiceQualityFromString("high"), NAliceProtocol::TVoiceOptions::HIGH);
        UNIT_ASSERT_EQUAL(VoiceQualityFromString("UltraHigh"), NAliceProtocol::TVoiceOptions::ULTRAHIGH);

        UNIT_ASSERT_EQUAL(VoiceQualityFromString(""), NAliceProtocol::TVoiceOptions::ULTRAHIGH);
        UNIT_ASSERT_EQUAL(VoiceQualityFromString("Qwerty12345"), NAliceProtocol::TVoiceOptions::ULTRAHIGH);
        // Check that conversion is case-insensitive
        UNIT_ASSERT_EQUAL(VoiceQualityFromString("HiGh"), NAliceProtocol::TVoiceOptions::HIGH);
    }

    void TestVoiceQualityToString() {
        UNIT_ASSERT_VALUES_EQUAL(VoiceQualityToString(NAliceProtocol::TVoiceOptions::DEFAULT_QUALITY), "UltraHigh");
        UNIT_ASSERT_VALUES_EQUAL(VoiceQualityToString(NAliceProtocol::TVoiceOptions::LOW), "Low");
        UNIT_ASSERT_VALUES_EQUAL(VoiceQualityToString(NAliceProtocol::TVoiceOptions::HIGH), "High");
        UNIT_ASSERT_VALUES_EQUAL(VoiceQualityToString(NAliceProtocol::TVoiceOptions::ULTRAHIGH), "UltraHigh");
    }
};

class TCuttlefishTtsUtilsCanonizeTest : public TTestBase {
    // Do not forget to add test to canonization runner (tests_canonize/ya.make)
    UNIT_TEST_SUITE(TCuttlefishTtsUtilsCanonizeTest);
    UNIT_TEST(TestGetVoiceList);
    UNIT_TEST(TestGetTtsBackendRequestItemTypeForLang);
    UNIT_TEST(TestGetSpeakersJson);
    UNIT_TEST(TestGetSpeakersJsonPythonUniproxyFormat);
    UNIT_TEST_SUITE_END();

public:
    void TestGetVoiceList() {
        auto voiceList = GetVoiceList();
        for (const auto& voice : voiceList) {
            Cout << voice << Endl;
        }
    }

    void TestGetTtsBackendRequestItemTypeForLang() {
        TVector<TString> allLangs = {
            TVoicetechSpeaker::LANG_EN,
            TVoicetechSpeaker::LANG_RU,
            TVoicetechSpeaker::LANG_TR,
            TVoicetechSpeaker::LANG_UK,
        };
        auto voiceList = GetVoiceList();

        TSet<TString> ttsBackendRequestItemTypes;
        for (const auto& voice : voiceList) {
            const auto* speaker = FindVoicetechSpeaker(voice);
            for (const auto& lang : allLangs) {
                ttsBackendRequestItemTypes.insert(speaker->GetTtsBackendRequestItemTypeForLang(lang));
            }
        }

        for (const auto& ttsBackendRequestItemType : ttsBackendRequestItemTypes) {
            Cout << ttsBackendRequestItemType << Endl;
        }
    }

    void TestGetSpeakersJson() {
        NJson::TJsonValue speakersJson = GetSpeakersJson();
        WriteJson(&Cout, &speakersJson, /* formatOutput = */ true, /* sortkeys = */ true, /* validateUtf8 = */ true);
    }

    void TestGetSpeakersJsonPythonUniproxyFormat() {
        NJson::TJsonValue speakersJson = GetSpeakersJsonPythonUniproxyFormat();
        WriteJson(&Cout, &speakersJson, /* formatOutput = */ true, /* sortkeys = */ true, /* validateUtf8 = */ true);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishTtsUtilsTest)
UNIT_TEST_SUITE_REGISTRATION(TCuttlefishTtsUtilsCanonizeTest)
