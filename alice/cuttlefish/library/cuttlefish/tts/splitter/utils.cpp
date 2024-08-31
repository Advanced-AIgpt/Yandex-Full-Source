#include "utils.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/tts/utils/speaker.h>
#include <alice/cuttlefish/library/cuttlefish/tts/utils/utils.h>

#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <library/cpp/digest/md5/md5.h>

#include <util/charset/wide.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/type.h>
#include <util/system/hostname.h>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/backends_tts/ttsutils.py?rev=r7923533#L59-66
// Speaker options
constexpr TStringBuf VOICE_PARAM_NAME = "voice";
constexpr TStringBuf LANG_PARAM_NAME = "lang";
constexpr TStringBuf VOLUME_PARAM_NAME = "volume";
constexpr TStringBuf SPEED_PARAM_NAME = "speed";
constexpr TStringBuf IS_WHISPER_PARAM_NAME = "is_whisper";
constexpr TStringBuf AUDIO_PARAM_NAME = "audio";
constexpr TStringBuf EFFECT_PARAM_NAME = "effect";
constexpr TStringBuf EMOTION_PARAM_NAME = "emotion";
constexpr TStringBuf S3_AUDIO_BACKET_PARAM_NAME = "s3_audio_bucket";

// Global options
constexpr TStringBuf BACKGROUND_AUDIO_PARAM_NAME = "background";

const THashSet<TStringBuf> PARAM_NAMES = {
    // Speaker options
    VOICE_PARAM_NAME,
    LANG_PARAM_NAME,
    VOLUME_PARAM_NAME,
    SPEED_PARAM_NAME,
    IS_WHISPER_PARAM_NAME,
    AUDIO_PARAM_NAME,
    EFFECT_PARAM_NAME,
    EMOTION_PARAM_NAME,
    S3_AUDIO_BACKET_PARAM_NAME,

    // Global options
    BACKGROUND_AUDIO_PARAM_NAME,
};

constexpr TStringBuf EMPTY_PARAMS = TStringBuf();

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/backends_tts/ttsutils.py?rev=r7934185#L132-145
constexpr TStringBuf DEFAULT_LANG = "ru";
constexpr TStringBuf DEFAULT_VOICE = "shitova";
constexpr double DEFAULT_SPEED = 1.0;
constexpr double DEFAULT_VOLUME = 1.0;

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/backends_tts/ttsutils.py?rev=r7996225#L211-218
constexpr TStringBuf VOICE_SHITOVA = "shitova";
constexpr TStringBuf VOICE_SHITOVA_GPU = "shitova.gpu";
constexpr TStringBuf VOICE_SHITOVA_WHISPER_GPU = "shitova_whisper.gpu";

constexpr TStringBuf SPEAKER_OPEN_TAG_ASCII = "<speaker";
constexpr TStringBuf SPEAKER_CLOSE_TAG_ASCII = ">";
const TUtf32String SPEAKER_OPEN_TAG = ASCIIToUTF32(SPEAKER_OPEN_TAG_ASCII);
const TUtf32String SPEAKER_CLOSE_TAG = ASCIIToUTF32(SPEAKER_CLOSE_TAG_ASCII);

constexpr size_t MD5_HEX_DIGEST_LENGTH = 32;

// WARNING: MD5 is flushed after conversion
// Do not reuse it
TString Md5ToString(MD5& md5) {
    TString result;
    result.ReserveAndResize(MD5_HEX_DIGEST_LENGTH);
    md5.End(result.begin());
    return result;
}

// WARNING: UTF32ToUTF8(tag).size() must be equal to tag.size()
size_t FindTagPos(const TStringBuf& text, const TUtf32String& tag, ssize_t pos) {
    TUtf32String buffer;
    buffer.reserve(tag.size());

    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(text.Data()) + pos;
    const unsigned char* end = reinterpret_cast<const unsigned char*>(text.Data()) + text.size();
    while (ptr != end) {
        wchar32 currentRune;
        if (ReadUTF8CharAndAdvance(currentRune, ptr, end) != RECODE_RESULT::RECODE_OK) {
            // TODO(chegoryu) log + sensor for error

            // However we must return npos here because original python uniproxy
            // ignore all parse errors
            return TString::npos;
        }

        if (buffer.size() == tag.size()) {
            buffer = buffer.substr(1, buffer.size() - 1);
        }
        buffer.push_back(currentRune);

        if (buffer == tag) {
            return ptr - reinterpret_cast<const unsigned char*>(text.Data()) - tag.size();
        }
    }

    return TString::npos;
}

const TStringBuf StripLeft(const TStringBuf& text) {
    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(text.Data());
    const unsigned char* end = reinterpret_cast<const unsigned char*>(text.Data()) + text.size();

    while (ptr != end) {
        wchar32 currentRune;
        const unsigned char* last = ptr;
        if (ReadUTF8CharAndAdvance(currentRune, ptr, end) != RECODE_RESULT::RECODE_OK) {
            // TODO(chegoryu) log + sensor for error

            // However we must return original here because original python uniproxy
            // ignore all parse errors
            return text;
        }

        if (!IsWhitespace(currentRune)) {
            return TStringBuf(reinterpret_cast<const char*>(last), end - last);
        }
    }

    // Empty result
    return TStringBuf();
}

const TStringBuf StripRight(const TStringBuf& text) {
    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(text.Data());
    const unsigned char* end = reinterpret_cast<const unsigned char*>(text.Data()) + text.size();

    const unsigned char* lastNotWhiteSpaceEnd = nullptr;
    while (ptr != end) {
        wchar32 currentRune;
        if (ReadUTF8CharAndAdvance(currentRune, ptr, end) != RECODE_RESULT::RECODE_OK) {
            // TODO(chegoryu) log + sensor for error

            // However we must return original here because original python uniproxy
            // ignore all parse errors
            return text;
        }

        if (!IsWhitespace(currentRune)) {
            lastNotWhiteSpaceEnd = ptr;
        }
    }

    if (lastNotWhiteSpaceEnd == nullptr) {
        // Empty result
        return TStringBuf();
    } else {
        return TStringBuf(text.Data(), reinterpret_cast<const char*>(lastNotWhiteSpaceEnd) - text.Data());
    }
}

const TStringBuf Strip(const TStringBuf& text) {
    return StripRight(StripLeft(text));
}

// We assume that params is correct ascii string
THashMap<TStringBuf, TStringBuf> GetParsedParamsMap(const TStringBuf& params) {
    THashMap<TStringBuf, TStringBuf> parsedParams;

    if (params.empty()) {
        return parsedParams;
    }

    if (!params.StartsWith(SPEAKER_OPEN_TAG_ASCII) || !params.EndsWith(SPEAKER_CLOSE_TAG_ASCII)) {
        // TODO(chegoryu) log + sensor for error
        return parsedParams;
    }

    TStringBuf paramsWithoutTags = TStringBuf(
        params.Data() + SPEAKER_OPEN_TAG_ASCII.size(),
        params.size() - SPEAKER_OPEN_TAG_ASCII.size() - SPEAKER_CLOSE_TAG.size()
    );
    for (ssize_t i = 0; i < static_cast<ssize_t>(paramsWithoutTags.size()) - 1; ++i) {
        if (paramsWithoutTags[i] == '=' && (paramsWithoutTags[i + 1] == '\'' || paramsWithoutTags[i + 1] == '"')) {
            TStringBuf name = "";
            TStringBuf value = "";
            for (ssize_t j = i - 1; j >= 0; --j) {
                if (paramsWithoutTags[j] == ' ') {
                    name = TStringBuf(paramsWithoutTags.data() + j + 1, i - j - 1);
                    break;
                }
                if (!j) {
                    name = TStringBuf(paramsWithoutTags.data(), i - j);
                    break;
                }
            }
            for (ssize_t j = i + 2; j < static_cast<ssize_t>(paramsWithoutTags.size()); ++j) {
                // Inherit python uniproxy logic
                // No escaping
                // No quotes type maching
                if (paramsWithoutTags[j] == '\'' || paramsWithoutTags[j] == '"') {
                    value = TStringBuf(paramsWithoutTags.data() + i + 2, j - i - 2);
                    i = j;
                    break;
                }
            }

            bool needSkip = false;
            if (!PARAM_NAMES.contains(name)) {
                // TODO(chegoryu) unknown params warning
                needSkip = true;
            }

            if (value.empty()) {
                // TODO(chegoryu) empty value warning
                needSkip = true;
            }

            if (parsedParams.contains(name)) {
                // TODO(chegoryu) name repetition warning
                needSkip = true;
            }

            if (needSkip) {
                continue;
            }

            parsedParams[name] = value;
        }
    }

    return parsedParams;
}

TString GetStringValue(
    const TStringBuf& defaultValue,
    const TMaybe<TStringBuf>& globalOptionsValue
) {
    if (globalOptionsValue.Defined()) {
        return TString(*globalOptionsValue);
    } else {
        return TString(defaultValue);
    }
}

double GetDoubleValue(
    const double defaultValue,
    const TMaybe<double>& globalOptionsValue
) {
    if (globalOptionsValue.Defined()) {
        // We have no choice and must convert double to string
        return *globalOptionsValue;
    } else {
        return defaultValue;
    }
}

// WARNING: This class does not own memory
// You must keep all constructor arguments alive for the rest of the life of the class
class TSpeakerTagsParser {
public:
    TSpeakerTagsParser(
        const TStringBuf& text,
        const NAliceProtocol::TAudioOptions& audioOptions,
        const NAliceProtocol::TVoiceOptions& voiceOptions,
        const TString& cacheKeyPrefix,
        bool replaceShitovaWithShitovaGpu,
        ui32 partialNumber,
        bool allowWhisper
    )
        : Text_(text)
        , AudioOptions_(audioOptions)
        , VoiceOptions_(voiceOptions)
        , CacheKeyPrefix_(cacheKeyPrefix)
        , ReplaceShitovaWithShitovaGpu_(replaceShitovaWithShitovaGpu)
        , PartialNumber_(partialNumber)
        , AllowWhisper_(allowWhisper)
    {
        Y_ENSURE(IsUtf(Text_), "Text is not a valid utf8 string");
    }

    void Parse() {
        size_t currentPos = 0;
        size_t speakerOpenTagPos = FindTagPos(Text_, SPEAKER_OPEN_TAG, currentPos);
        while (true) {
            if (speakerOpenTagPos == TString::npos) {
                AddNewAudioPart(
                    TStringBuf(Text_.Data() + currentPos, Text_.size() - currentPos),
                    EMPTY_PARAMS
                );
                break;
            }
            size_t speakerCloseTagPos = FindTagPos(Text_, SPEAKER_CLOSE_TAG, speakerOpenTagPos);
            if (speakerCloseTagPos == TString::npos) {
                AddNewAudioPart(
                    TStringBuf(Text_.Data() + currentPos, Text_.size() - currentPos),
                    EMPTY_PARAMS
                );
                break;
            }

            if (currentPos != speakerOpenTagPos) {
                AddNewAudioPart(
                    TStringBuf(Text_.Data() + currentPos, speakerOpenTagPos - currentPos),
                    EMPTY_PARAMS
                );
                currentPos = speakerOpenTagPos;
            }

            TStringBuf params = TStringBuf(Text_.Data() + currentPos, speakerCloseTagPos - currentPos + 1);
            currentPos = speakerCloseTagPos + SPEAKER_CLOSE_TAG.size();

            speakerOpenTagPos = FindTagPos(Text_, SPEAKER_OPEN_TAG, currentPos);
            if (speakerOpenTagPos == TString::npos) {
                AddNewAudioPart(
                    TStringBuf(Text_.Data() + currentPos, Text_.size() - currentPos),
                    params
                );
                break;
            } else {
                AddNewAudioPart(
                    TStringBuf(Text_.Data() + currentPos, speakerOpenTagPos - currentPos),
                    params
                );
                currentPos = speakerOpenTagPos;
            }
        }
    }

    TSplitBySpeakersTagsResult GetResult() const {
        return Result_;
    }

private:
    void AddNewAudioPart(
        const TStringBuf& text,
        const TStringBuf& params
    ) {
        NTts::TAudioPartToGenerate newAudioPart = CreateNewAudioPartTemplate();

        newAudioPart.SetPartialNumber(PartialNumber_);
        newAudioPart.SetSequenceNumber(Result_.AudioParts_.size());
        newAudioPart.SetText(TString(Strip(text)));

        ParseAndApplyParams(newAudioPart, params);

        // WARNING: We must ParseAndApplyParams first
        if (ReplaceShitovaWithShitovaGpu_ && newAudioPart.GetVoice().find(VOICE_SHITOVA) != TString::npos) {
            newAudioPart.SetVoice(TString(VOICE_SHITOVA_GPU));
        }

        if (newAudioPart.GetIsWhisper() && newAudioPart.GetVoice() == VOICE_SHITOVA_GPU) {
            newAudioPart.SetVoice(TString(VOICE_SHITOVA_WHISPER_GPU));
        }

        // Fill cache key after all other fields
        newAudioPart.SetCacheKey(GetCacheKey(CacheKeyPrefix_, newAudioPart));

        Result_.AudioParts_.push_back(std::move(newAudioPart));
    }

    void ParseAndApplyParams(
        NTts::TAudioPartToGenerate& currentAudioPart,
        const TStringBuf& params
    ) {
        THashMap<TStringBuf, TStringBuf> parsedParams = GetParsedParamsMap(params);

        {
            // Current part
            if (const auto ptr = parsedParams.FindPtr(VOICE_PARAM_NAME)) {
                currentAudioPart.SetVoice(TString(*ptr));
            }

            if (const auto ptr = parsedParams.FindPtr(LANG_PARAM_NAME)) {
                currentAudioPart.SetLang(TString(*ptr));
            }

            if (const auto ptr = parsedParams.FindPtr(VOLUME_PARAM_NAME)) {
                try {
                    double volume = FromString<double>(*ptr);
                    currentAudioPart.SetVolume(volume);
                } catch (...) {
                    // TODO(chegoryu) log + sensor for error
                }
            }

            if (const auto ptr = parsedParams.FindPtr(SPEED_PARAM_NAME)) {
                try {
                    double speed = FromString<double>(*ptr);
                    currentAudioPart.SetSpeed(speed);
                } catch (...) {
                    // TODO(chegoryu) log + sensor for error
                }
            }

            if (const auto ptr = parsedParams.FindPtr(IS_WHISPER_PARAM_NAME)) {
                if (AllowWhisper_) {
                    currentAudioPart.SetIsWhisper(IsTrue(*ptr));
                }
            }

            if (const auto ptr = parsedParams.FindPtr(EFFECT_PARAM_NAME)) {
                currentAudioPart.SetEffect(TString(*ptr));
            }

            if (const auto ptr = parsedParams.FindPtr(AUDIO_PARAM_NAME)) {
                currentAudioPart.SetAudio(TString(*ptr));
            }

            if (const auto ptr = parsedParams.FindPtr(S3_AUDIO_BACKET_PARAM_NAME)) {
                currentAudioPart.SetS3AudioBucket(TString(*ptr));
            }

            if (const auto ptr = parsedParams.FindPtr(EMOTION_PARAM_NAME)) {
                currentAudioPart.SetEmotion(TString(*ptr));
            }
        }

        {
            // Global state
            // TODO(VOICESERV-4090): do it in better way
            // Now it's only for mvp
            // in the future we need something like tag "global_settings" for such options
            if (const auto ptr = parsedParams.FindPtr(BACKGROUND_AUDIO_PARAM_NAME); ptr && !Result_.BackgroundAudioPathForS3_.Defined()) {
                Result_.BackgroundAudioPathForS3_ = *ptr;
            }
        }
    }

    NTts::TAudioPartToGenerate CreateNewAudioPartTemplate() {
        if (Result_.AudioParts_.empty()) {
            return CreateInitialAudioPartTemplate();
        } else {
            NTts::TAudioPartToGenerate newPart = Result_.AudioParts_.back();

            // https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/tts.py?rev=r8248332#L553-556
            // Python uniproxy apply patches sequentially
            // So new part is previous part (not initial part) with some patch
            // Yes, it's not obvious from code, but after some time
            // you'll understand this logic (tts cache lookup function patch payload that used for creation of next request)
            newPart.ClearText();
            newPart.ClearAudio();
            newPart.ClearS3AudioBucket();
            newPart.ClearCacheKey();

            return newPart;
        }
    }

    NTts::TAudioPartToGenerate CreateInitialAudioPartTemplate() {
        NTts::TAudioPartToGenerate initialPart;

        initialPart.SetVoice(
            GetStringValue(
                DEFAULT_VOICE,
                (VoiceOptions_.HasVoice() && VoiceOptions_.GetVoice()) ? TMaybe<TStringBuf>(VoiceOptions_.GetVoice()) : Nothing()
            )
        );

        initialPart.SetLang(
            GetStringValue(
                DEFAULT_LANG,
                (VoiceOptions_.HasLang() && VoiceOptions_.GetLang()) ? TMaybe<TStringBuf>(VoiceOptions_.GetLang()) : Nothing()
            )
        );

        {
            initialPart.SetVolume(
                GetDoubleValue(
                    DEFAULT_VOLUME,
                    (VoiceOptions_.HasVolume() && VoiceOptions_.GetVolume()) ? TMaybe<double>(VoiceOptions_.GetVolume()) : Nothing()
                )
            );
        }

        {
            initialPart.SetSpeed(
                GetDoubleValue(
                    DEFAULT_SPEED,
                    (VoiceOptions_.HasSpeed() && VoiceOptions_.GetSpeed()) ? TMaybe<double>(VoiceOptions_.GetSpeed()) : Nothing()
                )
            );
        }

        // No default or voice options value for effect

        // No default value for emotion
        initialPart.SetEmotion(VoiceOptions_.GetUnrestrictedEmotion());

        // Default value inside FormatToMime
        initialPart.SetMime(TString(NTtsUtils::FormatToMime(AudioOptions_.GetFormat(), VoiceOptions_.GetQuality())));

        return initialPart;
    }

private:
    const TStringBuf Text_;
    const NAliceProtocol::TAudioOptions& AudioOptions_;
    const NAliceProtocol::TVoiceOptions& VoiceOptions_;
    const TString& CacheKeyPrefix_;
    const bool ReplaceShitovaWithShitovaGpu_;
    const ui32 PartialNumber_;
    const bool AllowWhisper_;

    TSplitBySpeakersTagsResult Result_;
};

} // namespace

TSplitBySpeakersTagsResult SplitTextBySpeakerTags(
    const NTts::TRequest& ttsRequest,
    const NAliceProtocol::TAudioOptions& audioOptions,
    const NAliceProtocol::TVoiceOptions& voiceOptions,
    const TString& cacheKeyPrefix,
    bool allowWhisper
) {
    TSpeakerTagsParser speakerTagsParser(
        ttsRequest.GetText(),
        audioOptions,
        voiceOptions,
        cacheKeyPrefix,
        ttsRequest.GetReplaceShitovaWithShitovaGpu(),
        ttsRequest.GetPartialNumber(),
        allowWhisper
    );

    speakerTagsParser.Parse();
    return speakerTagsParser.GetResult();
}

TString GetCacheKey(
    const TString& cacheKeyPrefix,
    const NTts::TAudioPartToGenerate& audioPartToGenerate
) {
    static constexpr TStringBuf separator = "_";

    MD5 textMd5;
    {
        textMd5.Update(ToLowerUTF8(audioPartToGenerate.GetText()));
        textMd5.Update(separator);
    }

    MD5 paramsMd5;
    {
        TVector<TStringBuf> params = {
            audioPartToGenerate.GetLang(),
            audioPartToGenerate.GetVoice(),
            audioPartToGenerate.GetEffect(),
            audioPartToGenerate.GetEmotion(),
            audioPartToGenerate.GetMime(),
        };

        if (audioPartToGenerate.HasIsWhisper() && audioPartToGenerate.GetIsWhisper()) {
            params.push_back("whisper");
        }

        for (const TStringBuf& value : params) {
            for (const char c : value) {
                auto lowerC = AsciiToLower(c);
                paramsMd5.Update(&lowerC, 1);
            }
            paramsMd5.Update(separator);
        }

        for (const ui32 value : {
            (ui32)std::llround(audioPartToGenerate.GetSpeed() * 100),
            (ui32)std::llround(audioPartToGenerate.GetVolume() * 100),
        }) {
            paramsMd5.Update(&value, sizeof(value));
            paramsMd5.Update(separator);
        }
    }

    return TStringBuilder()
        << cacheKeyPrefix
        << separator
        << Md5ToString(textMd5)
        << separator
        << Md5ToString(paramsMd5)
    ;
}

TTtsBackendRequestInfo CreateTtsBackendRequestInfo(
    const NTts::TAudioPartToGenerate& audioPart,
    const TString& sessionId,
    const TString& requestId,
    bool enableTtsBackendTimings,
    bool doNotLogTexts,
    ui32 reqSeqNo,
    TString surface
) {
    TMaybe<TString> normalizedLang = NTtsUtils::TryNormalizeLang(audioPart.GetLang());
    if (!normalizedLang.Defined()) {
        ythrow yexception() << "Unsupported lang '" << audioPart.GetLang() << "'";
    }
    TString lang = *normalizedLang;

    NTtsUtils::TDefaultParams params{audioPart.GetVoice(), lang, audioPart.GetEmotion()};
    auto speaker = NTtsUtils::CreateSpeaker(params);
    if (!speaker) {
        ythrow yexception() << "Unsupported voice '" << audioPart.GetVoice() << "'";
    }

    TTtsBackendRequestInfo ttsBackendRequestInfo;
    ttsBackendRequestInfo.Request_.SetReqSeqNo(reqSeqNo);
    ttsBackendRequestInfo.Request_.SetDoNotLogTexts(doNotLogTexts);

    {
        auto& generateRequest = *ttsBackendRequestInfo.Request_.MutableGenerate();

        generateRequest.set_sessionid(sessionId);
        generateRequest.set_message_id(requestId);

        // A mistake was made due to which the concepts of "need" and "enbale" were mixed up
        // In fact "need" here is "enable"
        generateRequest.set_need_timings(enableTtsBackendTimings);

        generateRequest.set_text(audioPart.GetText());

        generateRequest.set_volume(audioPart.GetVolume());
        generateRequest.set_speed(audioPart.GetSpeed());
        generateRequest.set_is_whisper(audioPart.GetIsWhisper());

        if (!audioPart.GetEffect().empty()) {
            generateRequest.set_effect(audioPart.GetEffect());
        }

        speaker->FillGenerateRequest(generateRequest);

        // Force chunked mode for now
        // As I know from tests modern tts-server always use chunked mode
        // and ignore this option
        generateRequest.set_chunked(true);

        // TODO(VOICESERV-4078)
        // Fix bug in legacy cpu tts-server or copy hack with not_send_mime here

        // if (!speaker->IsLegacy) {
        generateRequest.set_mime(audioPart.GetMime());
        // }

        generateRequest.set_do_not_log(doNotLogTexts);

        {
            TString hostname = "unknown";
            try {
                hostname = FQDNHostName();
            } catch (...) {
                // ¯\_(ツ)_/¯
            }
            generateRequest.set_clienthostname(hostname);
        }

        generateRequest.set_surface(surface);
    }

    ttsBackendRequestInfo.ItemType_ = speaker->GetTtsBackendRequestItemType();

    return ttsBackendRequestInfo;
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
