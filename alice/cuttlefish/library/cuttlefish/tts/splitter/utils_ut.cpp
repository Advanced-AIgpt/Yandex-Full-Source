#include "utils.h"

#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <google/protobuf/util/message_differencer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/maybe.h>
#include <util/system/hostname.h>

using namespace NAlice::NCuttlefish::NAppHostServices;

namespace {

NAliceProtocol::TAudioOptions GetDefaultAudioOptions() {
    return {};
}

NAliceProtocol::TVoiceOptions GetDefaultVoiceOptions() {
    return {};
}

const TString DefaultCacheKeyPrefix() {
    return "default_cache_key_prefix";
}

struct TVoiceOptionsGenerator {
    TVoiceOptionsGenerator() {
        Result_ = GetDefaultVoiceOptions();
    }

    TVoiceOptionsGenerator& SetVolume(double volume) {
        Result_.SetVolume(volume);
        return *this;
    }

    TVoiceOptionsGenerator& SetSpeed(double speed) {
        Result_.SetSpeed(speed);
        return *this;
    }

    TVoiceOptionsGenerator& SetLang(const TString& lang) {
        Result_.SetLang(lang);
        return *this;
    }

    TVoiceOptionsGenerator& SetVoice(const TString& voice) {
        Result_.SetVoice(voice);
        return *this;
    }

    TVoiceOptionsGenerator& SetEmotion(const TString& emotion) {
        Result_.SetUnrestrictedEmotion(emotion);
        return *this;
    }

    TVoiceOptionsGenerator& SetQuality(const NAliceProtocol::TVoiceOptions::EVoiceQuality& quality) {
        Result_.SetQuality(quality);
        return *this;
    }

    NAliceProtocol::TVoiceOptions Get() {
        return Result_;
    }

    NAliceProtocol::TVoiceOptions Result_;
};

NTts::TAudioPartToGenerate GetExactlyOneResultFromSplitTextBySpeakerTags(
    const NTts::TRequest& ttsRequest,
    const NAliceProtocol::TAudioOptions& audioOptions,
    const NAliceProtocol::TVoiceOptions& voiceOptions,
    bool allowWhisper = true
) {
    auto result = SplitTextBySpeakerTags(ttsRequest, audioOptions, voiceOptions, DefaultCacheKeyPrefix(), allowWhisper);
    UNIT_ASSERT_VALUES_EQUAL_C(result.AudioParts_.size(), 1, "Only one part expected");
    UNIT_ASSERT_C(!result.BackgroundAudioPathForS3_.Defined(), "no background audio expected");

    return result.AudioParts_[0];
}

NTts::TAudioPartToGenerate GetDefaultAudioPartToGenerate() {
    NTts::TRequest ttsRequest;
    ttsRequest.SetText("");

    return GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, GetDefaultAudioOptions(), GetDefaultVoiceOptions());
}

struct TAudioPartToGenerateGenerator {
    TAudioPartToGenerateGenerator(ui32 partialNumber, ui32 sequenceNumber) {
        Result_ = GetDefaultAudioPartToGenerate();
        Result_.SetPartialNumber(partialNumber);
        Result_.SetSequenceNumber(sequenceNumber);
    }

    TAudioPartToGenerateGenerator& SetText(const TString& text) {
        Result_.SetText(text);
        return *this;
    }

    TAudioPartToGenerateGenerator& SetLang(const TString& lang) {
        Result_.SetLang(lang);
        return *this;
    }

    TAudioPartToGenerateGenerator& SetVoice(const TString& voice) {
        Result_.SetVoice(voice);
        return *this;
    }

    TAudioPartToGenerateGenerator& SetVolume(double volume) {
        Result_.SetVolume(volume);
        return *this;
    }

    TAudioPartToGenerateGenerator& SetSpeed(double speed) {
        Result_.SetSpeed(speed);
        return *this;
    }

    TAudioPartToGenerateGenerator& SetIsWhisper(bool isWhisper) {
        Result_.SetIsWhisper(isWhisper);
        return *this;
    }

    TAudioPartToGenerateGenerator& SetEffect(const TString& effect) {
        Result_.SetEffect(effect);
        return *this;
    }

    TAudioPartToGenerateGenerator& SetEmotion(const TString& emotion) {
        Result_.SetEmotion(emotion);
        return *this;
    }

    TAudioPartToGenerateGenerator& SetAudio(const TString& audio) {
        Result_.SetAudio(audio);
        return *this;
    }

    TAudioPartToGenerateGenerator& SetS3AudioBucket(const TString& s3AudioBucket) {
        Result_.SetS3AudioBucket(s3AudioBucket);
        return *this;
    }

    TAudioPartToGenerateGenerator& SetMime(const TString& mime) {
        Result_.SetMime(mime);
        return *this;
    }

    NTts::TAudioPartToGenerate Get(bool calcCacheKey = true) {
        if (calcCacheKey) {
            Result_.SetCacheKey(GetCacheKey(DefaultCacheKeyPrefix(), Result_));
        }
        return Result_;
    }

    NTts::TAudioPartToGenerate Result_;
};

NTts::TAudioPartToGenerate GetAudioPartToGenerateWithAllFields() {
    NTts::TAudioPartToGenerate audioPartToGenerate = GetDefaultAudioPartToGenerate();

    audioPartToGenerate.SetText("text");
    audioPartToGenerate.SetLang("lang");
    audioPartToGenerate.SetVoice("voice");
    audioPartToGenerate.SetVolume(0.5);
    audioPartToGenerate.SetSpeed(1.0);
    audioPartToGenerate.SetIsWhisper(false);
    audioPartToGenerate.SetEffect("effect");
    audioPartToGenerate.SetEmotion("emotion");
    audioPartToGenerate.SetAudio("audio");
    audioPartToGenerate.SetS3AudioBucket("tts-audio");
    audioPartToGenerate.SetMime("mime");

    audioPartToGenerate.SetCacheKey(GetCacheKey(DefaultCacheKeyPrefix(), audioPartToGenerate));

    return audioPartToGenerate;
}

struct TSplitBySpeakersTagsResultGenerator {
    TSplitBySpeakersTagsResultGenerator() {
    }

    TSplitBySpeakersTagsResultGenerator& AddAudioPart(const NTts::TAudioPartToGenerate& audioPart) {
        Result_.AudioParts_.push_back(audioPart);
        return *this;
    }

    TSplitBySpeakersTagsResultGenerator& SetBackgroundAudioPathForS3(const TMaybe<TString> backgroundAudioPathForS3) {
        Result_.BackgroundAudioPathForS3_ = backgroundAudioPathForS3;
        return *this;
    }

    TSplitBySpeakersTagsResult Get() {
        return Result_;
    }

    TSplitBySpeakersTagsResult Result_;
};

template<typename T>
TMaybe<TString> GetProtobufDiff(const T& a, const T& b) {
    TString diff;
    google::protobuf::util::MessageDifferencer messageDifferencer;
    messageDifferencer.ReportDifferencesToString(&diff);
    bool compareResult = messageDifferencer.Compare(a, b);
    return compareResult ? Nothing() : TMaybe<TString>(diff);
}

} // namespace

namespace NPythonUniproxyTests {

// Tests from https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/backends_tts/ut/ttsutils_ut.py?blame=true&rev=r7920855#L1
// Separated from other tests to make code more readable

TVector<std::pair<TString, TSplitBySpeakersTagsResult>> GetTests() {
    return {
        {
            // 0
            R"(Привет!)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(0, 0).SetText(R"(Привет!)").Get())
                .Get(),
        },
        {
            // 1
            R"(2 > 1)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(1, 0).SetText(R"(2 > 1)").Get())
                .Get(),
        },
        {
            // 2
            R"(<speaker voice="shitova">Яндекс.Новости: одна новость охренительнее другой!)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(2, 0).SetText(R"(Яндекс.Новости: одна новость охренительнее другой!)").SetVoice("shitova").Get())
                .Get(),
        },
        {
            // 3
            R"(Привет! <speaker voice='ermil'>Меня зовут Ермил!<speaker voice='shitova'> И я говорю голосом Шитовой.<speaker>А это дефолт.)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(3, 0).SetText(R"(Привет!)").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(3, 1).SetText(R"(Меня зовут Ермил!)").SetVoice("ermil").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(3, 2).SetText(R"(И я говорю голосом Шитовой.)").SetVoice("shitova").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(3, 3).SetText(R"(А это дефолт.)").Get())
                .Get(),
        },
        {
            // 4
            R"(рыба <[ mm aa s schwa | case=nom ]>)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(4, 0).SetText(R"(рыба <[ mm aa s schwa | case=nom ]>)").Get())
                .Get(),
        },
        {
            // 5
            R"(<speaker voice='oksana'>рыба <[ mm aa s schwa | case=nom ]>)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(5, 0).SetText(R"(рыба <[ mm aa s schwa | case=nom ]>)").SetVoice("oksana").Get())
                .Get(),
        },
        {
            // 6
            R"(<speaker audio="http://yandex.ru/something.wav">Яндекс.Новости: одна новость охренительнее другой!)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(6, 0).SetText(R"(Яндекс.Новости: одна новость охренительнее другой!)").SetAudio("http://yandex.ru/something.wav").Get())
                .Get(),
        },
        {
            // 7
            R"(<speaker audio="123">)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(7, 0).SetText(R"()").SetAudio("123").Get())
                .Get(),
        },
        {
            // 8
            R"(<speaker audio="123">456)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(8, 0).SetText(R"(456)").SetAudio("123").Get())
                .Get(),
        },
        {
            // 9
            R"(456<speaker audio="123">)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(9, 0).SetText(R"(456)").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(9, 1).SetText(R"()").SetAudio("123").Get())
                .Get(),
        },
        {
            // 10
            R"(<speaker audio="123"><speaker voice="oksana">456)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(10, 0).SetText(R"()").SetAudio("123").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(10, 1).SetText(R"(456)").SetVoice("oksana").Get())
                .Get(),
        },
        {
            // 11
            R"(<speaker audio="123"><speaker audio="456">22<speaker audio="789">)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(11, 0).SetText(R"()").SetAudio("123").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(11, 1).SetText(R"(22)").SetAudio("456").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(11, 2).SetText(R"()").SetAudio("789").Get())
                .Get(),
        },
        {
            // 12
            R"(<speaker lang="en">Who is mr<speaker voice="oksana">Путин?)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(12, 0).SetText(R"(Who is mr)").SetLang("en").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(12, 1).SetText(R"(Путин?)").SetVoice("oksana").SetLang("en").Get())
                .Get(),
        },
        {
            // 13
            R"(<speaker lang="en">Who is mr<speaker voice="oksana">Путин,<speaker voice="oksana" lang="en" volume="3.0">blyat?)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(13, 0).SetText(R"(Who is mr)").SetLang("en").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(13, 1).SetText(R"(Путин,)").SetVoice("oksana").SetLang("en").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(13, 2).SetText(R"(blyat?)").SetVoice("oksana").SetLang("en").SetVolume(3.0).Get())
                .Get(),
        },
        {
            // 14
            R"(<speaker lang="en">Who is mr<speaker voice="oksana">Путин,<speaker voice="oksana" lang="en" volume="high">blyat?)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(14, 0).SetText(R"(Who is mr)").SetLang("en").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(14, 1).SetText(R"(Путин,)").SetVoice("oksana").SetLang("en").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(14, 2).SetText(R"(blyat?)").SetVoice("oksana").SetLang("en").Get())
                .Get(),
        },
        {
            // 15
            R"(<speaker lang="en">Who is mr<speaker voice="oksana">Путин,<speaker voice="oksana" lang="en" volume="high" effect="translate_oksana_en">blyat?)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(15, 0).SetText(R"(Who is mr)").SetLang("en").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(15, 1).SetText(R"(Путин,)").SetVoice("oksana").SetLang("en").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(15, 2).SetText(R"(blyat?)").SetVoice("oksana").SetLang("en").SetEffect("translate_oksana_en").Get())
                .Get(),
        },
        {
            // 16
            R"(<speaker lang="en" speed="azaza">Who is mr<speaker voice="oksana" speed="2.0">Путин?)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(16, 0).SetText(R"(Who is mr)").SetLang("en").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(16, 1).SetText(R"(Путин?)").SetVoice("oksana").SetLang("en").SetSpeed(2.0).Get())
                .Get(),
        },
        {
            // 17
            R"(<speaker voice="shitova.gpu" is_whisper="true">Whisper text)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(17, 0).SetText(R"(Whisper text)").SetIsWhisper(true).SetVoice("shitova_whisper.gpu").Get())
                .Get(),
        },
        {
            // 18
            R"(<speaker audio="123" s3_audio_bucket="alice-time-capsule">22<speaker audio="456">42)",
            TSplitBySpeakersTagsResultGenerator()
                .AddAudioPart(TAudioPartToGenerateGenerator(18, 0).SetText(R"(22)").SetAudio("123").SetS3AudioBucket("alice-time-capsule").Get())
                .AddAudioPart(TAudioPartToGenerateGenerator(18, 1).SetText(R"(42)").SetAudio("456").Get())
                .Get(),
        }
    };
};

} // namespace NPythonUniproxyTests

class TCuttlefishTtsSplitterUtilsTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishTtsSplitterUtilsTest);
    UNIT_TEST(TestUTF8Encoding);
    UNIT_TEST(TestSplitTextBySpeakerTagsPythonUniproxyTests);
    UNIT_TEST(TestSplitTextBySpeakerTagsSequentiallyApplying);
    UNIT_TEST(TestSplitTextBySpeakerTagsWithBackgroundAudio);
    UNIT_TEST(TestSplitTextBySpeakerTagsBadUTF8);
    UNIT_TEST(TestSplitTextBySpeakerTagsDoNotAllowWhisper);
    UNIT_TEST(TestSplitTextBySpeakerTagsCornerCases);
    UNIT_TEST(TestSplitTextBySpeakerWithCustomMime);
    UNIT_TEST(TestSplitTextBySpeakerWithCustomVoiceOptions);
    UNIT_TEST(TestSplitTextBySpeakerWithReplaceShitovaWithShitovaGpu);
    UNIT_TEST(TestGetCacheKeyCanonizationAndDefaultValues);
    UNIT_TEST(TestGetCacheKeyChanged);
    UNIT_TEST(TestCreateTtsBackendRequestInfo);
    UNIT_TEST_SUITE_END();

public:
    void TestUTF8Encoding() {
        // Just sanity check
        UNIT_ASSERT_VALUES_EQUAL_C(TString("Привет!"), TString("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82!"), "This file must be in UTF8 encoding");
    }

    void TestSplitTextBySpeakerTagsPythonUniproxyTests() {
        CheckTests(NPythonUniproxyTests::GetTests());
    }

    void TestSplitTextBySpeakerTagsSequentiallyApplying() {
        const TVector<std::pair<TString, TSplitBySpeakersTagsResult>> tests = {
            {
                // 0
                R"(<speaker audio="audio">text with audio<speaker>audio must be empty in this part<speaker><speaker>after empty part)",
                TSplitBySpeakersTagsResultGenerator()
                    .AddAudioPart(TAudioPartToGenerateGenerator(0, 0).SetText(R"(text with audio)").SetAudio("audio").Get())
                    .AddAudioPart(TAudioPartToGenerateGenerator(0, 1).SetText(R"(audio must be empty in this part)").Get())
                    .AddAudioPart(TAudioPartToGenerateGenerator(0, 2).SetText(R"()").Get())
                    .AddAudioPart(TAudioPartToGenerateGenerator(0, 3).SetText(R"(after empty part)").Get())
                    .Get(),
            },
            {
                // 1
                R"(<speaker lang="en" voice="oksana" effect="rng" speed="1.5" volume="2.5">text0<speaker volume="bad_volume" speed="bad_speed">text1
                   <speaker voice="shitova.gpu" speed="0.9" volume="1.1" lang="ru" effect="other_rng">text2<speaker lang="tr">text3)",
                TSplitBySpeakersTagsResultGenerator()
                    .AddAudioPart(TAudioPartToGenerateGenerator(1, 0).SetText(R"(text0)").SetLang("en").SetVoice("oksana").SetSpeed(1.5).SetVolume(2.5).SetEffect("rng").Get())
                    .AddAudioPart(TAudioPartToGenerateGenerator(1, 1).SetText(R"(text1)").SetLang("en").SetVoice("oksana").SetSpeed(1.5).SetVolume(2.5).SetEffect("rng").Get())
                    .AddAudioPart(TAudioPartToGenerateGenerator(1, 2).SetText(R"(text2)").SetLang("ru").SetVoice("shitova.gpu").SetSpeed(0.9).SetVolume(1.1).SetEffect("other_rng").Get())
                    .AddAudioPart(TAudioPartToGenerateGenerator(1, 3).SetText(R"(text3)").SetLang("tr").SetVoice("shitova.gpu").SetSpeed(0.9).SetVolume(1.1).SetEffect("other_rng").Get())
                    .Get(),
            },
        };

        CheckTests(tests);
    }

    void TestSplitTextBySpeakerTagsWithBackgroundAudio() {
        const TVector<std::pair<TString, TSplitBySpeakersTagsResult>> tests = {
            {
                // 0
                R"(<speaker background="some_background">some text)",
                TSplitBySpeakersTagsResultGenerator()
                    .AddAudioPart(TAudioPartToGenerateGenerator(0, 0).SetText(R"(some text)").Get())
                    .SetBackgroundAudioPathForS3("some_background")
                    .Get(),
            },
            {
                // 1
                R"(<speaker background="some_background">some text<speaker background="other_audio">other text)",
                TSplitBySpeakersTagsResultGenerator()
                    .AddAudioPart(TAudioPartToGenerateGenerator(1, 0).SetText(R"(some text)").Get())
                    .AddAudioPart(TAudioPartToGenerateGenerator(1, 1).SetText(R"(other text)").Get())
                    // Second background must be ignored
                    .SetBackgroundAudioPathForS3("some_background")
                    .Get(),
            },
        };

        CheckTests(tests);
    }

    void TestSplitTextBySpeakerTagsBadUTF8() {
        NTts::TRequest ttsRequest;
        ttsRequest.SetPartialNumber(0);
        ttsRequest.SetText("<speaker audio=\"123\">\xD0\xD0\xF0\xB0\xD0");

        UNIT_ASSERT_EXCEPTION_CONTAINS(
            SplitTextBySpeakerTags(ttsRequest, GetDefaultAudioOptions(), GetDefaultVoiceOptions(), DefaultCacheKeyPrefix(), true),
            yexception,
            "Text is not a valid utf8 string"
        );
    }

    void TestSplitTextBySpeakerTagsDoNotAllowWhisper() {
        NTts::TRequest ttsRequest;
        ttsRequest.SetPartialNumber(0);
        ttsRequest.SetText("<speaker is_whisper=\"true\">I do not want to whisper");

        UNIT_ASSERT_VALUES_EQUAL(
            GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, GetDefaultAudioOptions(), GetDefaultVoiceOptions(), false).GetIsWhisper(),
            false
        );
    }

    void TestSplitTextBySpeakerTagsCornerCases() {
        const TVector<std::pair<TString, TSplitBySpeakersTagsResult>> tests = {
            {
                // 0
                R"(<speaker audio="aa" some text<)",
                TSplitBySpeakersTagsResultGenerator()
                    .AddAudioPart(TAudioPartToGenerateGenerator(0, 0).SetText(R"(<speaker audio="aa" some text<)").Get())
                    .Get(),
            },
            {
                // 1
                R"(<speaker audio="aa with spaces and =' some text>)",
                TSplitBySpeakersTagsResultGenerator()
                    .AddAudioPart(TAudioPartToGenerateGenerator(1, 0).SetText(R"()").SetAudio("aa with spaces and =").Get())
                    .Get(),
            },
            {
                // 2
                R"(<<<>>><speaker  audio='aa"  some text speed='123 df>  qwe   <speaker sklfjskj ="hfdsj" hgwer ghjdgsf yqigwe gd gg'' df'' df'=''><qw>)",
                TSplitBySpeakersTagsResultGenerator()
                    .AddAudioPart(TAudioPartToGenerateGenerator(2, 0).SetText(R"(<<<>>>)").Get())
                    .AddAudioPart(TAudioPartToGenerateGenerator(2, 1).SetText(R"(qwe)").SetAudio("aa").Get())
                    .AddAudioPart(TAudioPartToGenerateGenerator(2, 2).SetText(R"(<qw>)").Get())
                    .Get(),
            },
            {
                // 3
                R"(<speaker audio="audio1" audio="audio2" speed='10.6' speed='11' volume='12.5' volume='13' some text emotion="custom_emotion">test)",
                TSplitBySpeakersTagsResultGenerator()
                    .AddAudioPart(TAudioPartToGenerateGenerator(3, 0).SetText(R"(test)").SetAudio("audio1").SetEmotion("custom_emotion").SetSpeed(10.6).SetVolume(12.5).Get())
                    .Get(),
            },
        };

        CheckTests(tests);
    }

    void TestSplitTextBySpeakerWithCustomMime() {
        NTts::TRequest ttsRequest;
        ttsRequest.SetPartialNumber(0);
        ttsRequest.SetText("text");

        NAliceProtocol::TAudioOptions audioOptions;
        NAliceProtocol::TVoiceOptions voiceOptions;
        UNIT_ASSERT_VALUES_EQUAL(GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, audioOptions, voiceOptions).GetMime(), "audio/opus");

        audioOptions.SetFormat("bad");
        UNIT_ASSERT_VALUES_EQUAL(GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, audioOptions, voiceOptions).GetMime(), "audio/opus");

        audioOptions.SetFormat("pcm");
        UNIT_ASSERT_VALUES_EQUAL(GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, audioOptions, voiceOptions).GetMime(), "audio/x-pcm;bit=16;rate=16000");

        voiceOptions.SetQuality(NAliceProtocol::TVoiceOptions::ULTRAHIGH);
        UNIT_ASSERT_VALUES_EQUAL(GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, audioOptions, voiceOptions).GetMime(), "audio/x-pcm;bit=16;rate=48000");
    }

    void TestSplitTextBySpeakerWithCustomVoiceOptions() {
        const TVector<std::tuple<TString, TString, NAliceProtocol::TVoiceOptions, NTts::TAudioPartToGenerate, NTts::TAudioPartToGenerate>> tests = {
            {
                "volume",
                R"(<speaker volume="123">)",
                TVoiceOptionsGenerator().SetVolume(5.5).Get(),
                TAudioPartToGenerateGenerator(0, 0).SetText("").SetVolume(5.5).Get(),
                TAudioPartToGenerateGenerator(0, 0).SetText("").SetVolume(123).Get(),
            },
            {
                "speed",
                R"(<speaker speed="1234">)",
                TVoiceOptionsGenerator().SetSpeed(55.5).Get(),
                TAudioPartToGenerateGenerator(0, 0).SetText("").SetSpeed(55.5).Get(),
                TAudioPartToGenerateGenerator(0, 0).SetText("").SetSpeed(1234).Get(),
            },
            {
                "lang",
                R"(<speaker lang="ru">)",
                TVoiceOptionsGenerator().SetLang("en").Get(),
                TAudioPartToGenerateGenerator(0, 0).SetText("").SetLang("en").Get(),
                TAudioPartToGenerateGenerator(0, 0).SetText("").SetLang("ru").Get(),
            },
            {
                "voice",
                R"(<speaker voice="shitova">)",
                TVoiceOptionsGenerator().SetVoice("oksana").Get(),
                TAudioPartToGenerateGenerator(0, 0).SetText("").SetVoice("oksana").Get(),
                TAudioPartToGenerateGenerator(0, 0).SetText("").SetVoice("shitova").Get(),
            },
            {
                "emotion",
                R"(<speaker emotion="evil">)",
                TVoiceOptionsGenerator().SetEmotion("good").Get(),
                TAudioPartToGenerateGenerator(0, 0).SetText("").SetEmotion("good").Get(),
                TAudioPartToGenerateGenerator(0, 0).SetText("").SetEmotion("evil").Get(),
            },
        };

        for (const auto& [name, textWithOverride, voiceOptions, resultNoOverride, resultWithOverride] : tests) {
            NTts::TRequest ttsRequest;
            ttsRequest.SetPartialNumber(0);

            {
                // No override
                ttsRequest.SetText("");
                NTts::TAudioPartToGenerate result = GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, GetDefaultAudioOptions(), voiceOptions);
                auto diff = GetProtobufDiff(result, resultNoOverride);
                UNIT_ASSERT_C(!diff.Defined(), TStringBuilder() << "Failed to parse '" << name << "' from voice options. Protobuf diff: '" << *diff << "'");

            }

            {
                // With override
                ttsRequest.SetText(textWithOverride);
                NTts::TAudioPartToGenerate result = GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, GetDefaultAudioOptions(), voiceOptions);
                auto diff = GetProtobufDiff(result, resultWithOverride);
                UNIT_ASSERT_C(!diff.Defined(), TStringBuilder() << "Failed to parse '" << name << "' from voice options and override it with custom params. Protobuf diff: '" << *diff << "'");
            }
        }
    }

    void TestSplitTextBySpeakerWithReplaceShitovaWithShitovaGpu() {
        NTts::TRequest ttsRequest;
        ttsRequest.SetPartialNumber(0);
        ttsRequest.SetText("<speaker>text");

        UNIT_ASSERT_VALUES_EQUAL(GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, GetDefaultAudioOptions(), GetDefaultVoiceOptions()).GetVoice(), "shitova");

        ttsRequest.SetReplaceShitovaWithShitovaGpu(true);
        UNIT_ASSERT_VALUES_EQUAL(GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, GetDefaultAudioOptions(), GetDefaultVoiceOptions()).GetVoice(), "shitova.gpu");

        ttsRequest.SetText("<speaker voice='abc_shitova_abc'>text");
        UNIT_ASSERT_VALUES_EQUAL(GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, GetDefaultAudioOptions(), GetDefaultVoiceOptions()).GetVoice(), "shitova.gpu");

        ttsRequest.SetText("<speaker voice='random_voice'>text");
        UNIT_ASSERT_VALUES_EQUAL(GetExactlyOneResultFromSplitTextBySpeakerTags(ttsRequest, GetDefaultAudioOptions(), GetDefaultVoiceOptions()).GetVoice(), "random_voice");
    }

    void TestGetCacheKeyCanonizationAndDefaultValues() {
        static const TString warningMessage = "This commit change cache key value, probably some default values, are you sure that you do nothing wrong?";

        NTts::TAudioPartToGenerate audioPartToGenerate = GetDefaultAudioPartToGenerate();
        UNIT_ASSERT_VALUES_EQUAL_C(
            GetCacheKey(
                DefaultCacheKeyPrefix(),
                audioPartToGenerate
            ),
            "default_cache_key_prefix_b14a7b8059d9c055954c92674ce60032_e3e5e47fc38033744f84e6f630ec1f83",
            warningMessage
        );
        UNIT_ASSERT_VALUES_EQUAL_C(
            GetCacheKey(
                DefaultCacheKeyPrefix() + "_other",
                audioPartToGenerate
            ),
            "default_cache_key_prefix_other_b14a7b8059d9c055954c92674ce60032_e3e5e47fc38033744f84e6f630ec1f83",
            warningMessage
        );
    }

    void TestGetCacheKeyChanged() {
        NTts::TAudioPartToGenerate audioPartToGenerate = GetAudioPartToGenerateWithAllFields();

        TVector<std::pair<TString, std::function<NTts::TAudioPartToGenerate(const NTts::TAudioPartToGenerate&)>>> modificators = {
            {
                "text",
                [](const NTts::TAudioPartToGenerate& audioPartToGenerate) {
                    NTts::TAudioPartToGenerate audioPartToGenerateCopy;
                    audioPartToGenerateCopy.CopyFrom(audioPartToGenerate);
                    audioPartToGenerateCopy.SetText(audioPartToGenerate.GetText() + "_other");
                    return audioPartToGenerateCopy;
                },
            },
            {
                "lang",
                [](const NTts::TAudioPartToGenerate& audioPartToGenerate) {
                    NTts::TAudioPartToGenerate audioPartToGenerateCopy;
                    audioPartToGenerateCopy.CopyFrom(audioPartToGenerate);
                    audioPartToGenerateCopy.SetLang(audioPartToGenerate.GetLang() + "_other");
                    return audioPartToGenerateCopy;
                },
            },
            {
                "voice",
                [](const NTts::TAudioPartToGenerate& audioPartToGenerate) {
                    NTts::TAudioPartToGenerate audioPartToGenerateCopy;
                    audioPartToGenerateCopy.CopyFrom(audioPartToGenerate);
                    audioPartToGenerateCopy.SetVoice(audioPartToGenerate.GetVoice() + "_other");
                    return audioPartToGenerateCopy;
                },
            },
            {
                "volume",
                [](const NTts::TAudioPartToGenerate& audioPartToGenerate) {
                    NTts::TAudioPartToGenerate audioPartToGenerateCopy;
                    audioPartToGenerateCopy.CopyFrom(audioPartToGenerate);
                    audioPartToGenerateCopy.SetVolume(audioPartToGenerate.GetVolume() + 0.1);
                    return audioPartToGenerateCopy;
                },
            },
            {
                "speed",
                [](const NTts::TAudioPartToGenerate& audioPartToGenerate) {
                    NTts::TAudioPartToGenerate audioPartToGenerateCopy;
                    audioPartToGenerateCopy.CopyFrom(audioPartToGenerate);
                    audioPartToGenerateCopy.SetSpeed(audioPartToGenerate.GetSpeed() + 0.1);
                    return audioPartToGenerateCopy;
                },
            },
            {
                "effect",
                [](const NTts::TAudioPartToGenerate& audioPartToGenerate) {
                    NTts::TAudioPartToGenerate audioPartToGenerateCopy;
                    audioPartToGenerateCopy.CopyFrom(audioPartToGenerate);
                    audioPartToGenerateCopy.SetEffect(audioPartToGenerate.GetEffect() + "_other");
                    return audioPartToGenerateCopy;
                },
            },
            {
                "emotion",
                [](const NTts::TAudioPartToGenerate& audioPartToGenerate) {
                    NTts::TAudioPartToGenerate audioPartToGenerateCopy;
                    audioPartToGenerateCopy.CopyFrom(audioPartToGenerate);
                    audioPartToGenerateCopy.SetEmotion(audioPartToGenerate.GetEmotion() + "_other");
                    return audioPartToGenerateCopy;
                },
            },
            {
                "mime",
                [](const NTts::TAudioPartToGenerate& audioPartToGenerate) {
                    NTts::TAudioPartToGenerate audioPartToGenerateCopy;
                    audioPartToGenerateCopy.CopyFrom(audioPartToGenerate);
                    audioPartToGenerateCopy.SetMime(audioPartToGenerate.GetMime() + "_other");
                    return audioPartToGenerateCopy;
                },
            },
            {
                "is_whisper",
                [](const NTts::TAudioPartToGenerate& audioPartToGenerate) {
                    NTts::TAudioPartToGenerate audioPartToGenerateCopy;
                    audioPartToGenerateCopy.CopyFrom(audioPartToGenerate);
                    audioPartToGenerateCopy.SetIsWhisper(!audioPartToGenerate.GetIsWhisper());
                    return audioPartToGenerateCopy;
                },
            },
        };

        for (const auto& [name, modificator] : modificators) {
            UNIT_ASSERT_VALUES_UNEQUAL_C(
                GetCacheKey(DefaultCacheKeyPrefix(), audioPartToGenerate),
                GetCacheKey(DefaultCacheKeyPrefix(), modificator(audioPartToGenerate)),
                TStringBuilder() << "Cache key not changed after '" << name << "' modification"
            );
        }
    }

    void TestCreateTtsBackendRequestInfo() {
        constexpr double eps = 1e-9;

        {
            NTts::TAudioPartToGenerate audioPartToGenerate = TAudioPartToGenerateGenerator(0, 0)
                .SetText("test")
                .SetLang("en-EN")
                .SetVoice("shitova.gpu")
                .SetVolume(0.5)
                .SetSpeed(1.5)
                .SetIsWhisper(true)
                .SetEffect("translate_oksana_en")
                .SetEmotion("evil")
                .SetMime("audio/x-pcm;bit=16;rate=8000")
                .Get()
            ;

            TTtsBackendRequestInfo ttsBackendRequestInfo = CreateTtsBackendRequestInfo(
                audioPartToGenerate,
                "session_id",
                "message_id",
                true,
                true,
                42,
                "pp"
            );

            UNIT_ASSERT_VALUES_EQUAL(ttsBackendRequestInfo.ItemType_, "tts_backend_request_ru_gpu_shitova.gpu");
            UNIT_ASSERT_VALUES_EQUAL(ttsBackendRequestInfo.Request_.GetReqSeqNo(), 42);
            UNIT_ASSERT(ttsBackendRequestInfo.Request_.GetDoNotLogTexts());

            {
                auto& generateRequest = ttsBackendRequestInfo.Request_.GetGenerate();

                // Override by shitova.gpu voice
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.lang(), "ru");

                UNIT_ASSERT_VALUES_EQUAL(generateRequest.sessionid(), "session_id");
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.message_id(), "message_id");
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.need_timings(), true);

                UNIT_ASSERT_VALUES_EQUAL(generateRequest.text(), "test");
                UNIT_ASSERT_DOUBLES_EQUAL(generateRequest.volume(), 0.5, eps);
                UNIT_ASSERT_DOUBLES_EQUAL(generateRequest.speed(), 1.5, eps);
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.is_whisper(), true);
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.effect(), "translate_oksana_en");
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.chunked(), true);
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.mime(), "audio/x-pcm;bit=16;rate=8000");

                UNIT_ASSERT_VALUES_EQUAL(generateRequest.voices().size(), 1);
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.voices()[0].name(), "shitova");
                UNIT_ASSERT_DOUBLES_EQUAL(generateRequest.voices()[0].weight(), 1.0, eps);

                UNIT_ASSERT_VALUES_EQUAL(generateRequest.genders().size(), 1);
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.genders()[0].name(), "female");
                UNIT_ASSERT_DOUBLES_EQUAL(generateRequest.genders()[0].weight(), 1.0, eps);

                UNIT_ASSERT_VALUES_EQUAL(generateRequest.emotions().size(), 1);
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.emotions()[0].name(), "evil");
                UNIT_ASSERT_DOUBLES_EQUAL(generateRequest.emotions()[0].weight(), 1.0, eps);

                UNIT_ASSERT(!generateRequest.has_msd_threshold());
                UNIT_ASSERT(!generateRequest.lf0_postfilter());

                UNIT_ASSERT(generateRequest.do_not_log());

                UNIT_ASSERT_VALUES_EQUAL(generateRequest.clienthostname(), FQDNHostName());
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.surface(), "pp");
            }
        }

        {
            // Almost everything checked in previous part
            // Here only important asserts for smoky voice
            NTts::TAudioPartToGenerate audioPartToGenerate = TAudioPartToGenerateGenerator(0, 0)
                .SetLang("en-EN")
                .SetVoice("smoky")
                .SetMime("audio/opus")
                .Get()
            ;

            TTtsBackendRequestInfo ttsBackendRequestInfo = CreateTtsBackendRequestInfo(
                audioPartToGenerate,
                "session_id",
                "message_id",
                false,
                false,
                43,
                "navigator"
            );

            UNIT_ASSERT_VALUES_EQUAL(ttsBackendRequestInfo.ItemType_, "tts_backend_request_en_cpu_smoky");
            UNIT_ASSERT_VALUES_EQUAL(ttsBackendRequestInfo.Request_.GetReqSeqNo(), 43);
            UNIT_ASSERT(!ttsBackendRequestInfo.Request_.GetDoNotLogTexts());

            {
                auto& generateRequest = ttsBackendRequestInfo.Request_.GetGenerate();

                // No override by smoky voice
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.lang(), "en");

                UNIT_ASSERT_VALUES_EQUAL(generateRequest.sessionid(), "session_id");
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.message_id(), "message_id");
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.need_timings(), false);
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.mime(), "audio/opus");

                UNIT_ASSERT_VALUES_EQUAL(generateRequest.voices().size(), 1);
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.voices()[0].name(), "ermil");
                UNIT_ASSERT_DOUBLES_EQUAL(generateRequest.voices()[0].weight(), 1.0, eps);

                UNIT_ASSERT_VALUES_EQUAL(generateRequest.emotions().size(), 1);
                UNIT_ASSERT_VALUES_EQUAL(generateRequest.emotions()[0].name(), "good");
                UNIT_ASSERT_DOUBLES_EQUAL(generateRequest.emotions()[0].weight(), 1.0, eps);

                UNIT_ASSERT_DOUBLES_EQUAL(generateRequest.msd_threshold(), 1.0, eps);
                UNIT_ASSERT(!generateRequest.has_lf0_postfilter());

                UNIT_ASSERT(!generateRequest.do_not_log());
            }
        }

        {
            NTts::TAudioPartToGenerate audioPartToGenerate = TAudioPartToGenerateGenerator(0, 0).SetVoice("wrong_voice").Get();
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                CreateTtsBackendRequestInfo(audioPartToGenerate, "", "", false, false, 42, "navigator"),
                yexception,
                "Unsupported voice"
            );
        }

        {
            NTts::TAudioPartToGenerate audioPartToGenerate = TAudioPartToGenerateGenerator(0, 0).SetVoice("shitova").SetLang("wrong_lang").Get();
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                CreateTtsBackendRequestInfo(audioPartToGenerate, "", "", false, false, 42, "navigator"),
                yexception,
                "Unsupported lang"
            );
        }
    }

private:
    void CheckTests(const TVector<std::pair<TString, TSplitBySpeakersTagsResult>>& tests) {
        for (size_t i = 0; i < tests.size(); ++i) {
            const auto& [input, output] = tests[i];

            NTts::TRequest ttsRequest;
            ttsRequest.SetPartialNumber(i);
            ttsRequest.SetText(input);

            const auto& result = SplitTextBySpeakerTags(ttsRequest, GetDefaultAudioOptions(), GetDefaultVoiceOptions(), DefaultCacheKeyPrefix(), true);

            UNIT_ASSERT_VALUES_EQUAL_C(result.AudioParts_.size(), output.AudioParts_.size(), TStringBuilder() << "Wrong number of parts in test: '" << i << "'");
            for (size_t j = 0; j < result.AudioParts_.size(); ++j) {
                auto diff = GetProtobufDiff(result.AudioParts_[j], output.AudioParts_[j]);
                UNIT_ASSERT_C(!diff.Defined(), TStringBuilder() << "Test: '" << i << "', Part: '" << j << "', Diff: '" << *diff << "'");
            }

            UNIT_ASSERT_VALUES_EQUAL_C(result.BackgroundAudioPathForS3_, output.BackgroundAudioPathForS3_, TStringBuilder() << "Invalid background audio path for S3 in test: '" << i << "'");
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishTtsSplitterUtilsTest)
