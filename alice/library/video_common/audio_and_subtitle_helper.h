#pragma once

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/data/video/video.pb.h>

#include <library/cpp/json/writer/json_value.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>



namespace NAlice::NVideoCommon {

inline constexpr TStringBuf LANGUAGE_SUBTITLE_OFF = "off";
inline constexpr TStringBuf LANGUAGE_18PLUS = "18plus";
inline constexpr TStringBuf SUFFIX_LANG_18PLUS = "18";

inline constexpr TStringBuf SUGGEST_SUBTITLES_TURN_ON = "Включи субтитры";
inline constexpr TStringBuf SUGGEST_SUBTITLES_TURN_OFF = "Выключи субтитры";

inline constexpr TStringBuf PLAYER_RESTRICTION_CONFIG = "playerRestrictionConfig";
inline constexpr TStringBuf SUBTITLES_BUTTON_ENABLE = "subtitlesButtonEnable";
inline constexpr TStringBuf PLAYER_RESTRICTION_CONFIG_ = "player_restriction_config";
inline constexpr TStringBuf SUBTITLES_BUTTON_ENABLE_ = "subtitles_button_enable";


inline const THashMap<TStringBuf, TStringBuf> AUDIO_SUGGESTS {
    {"rus", "Включи русскую озвучку"},
    {"eng", "Включи английскую озвучку"},
    {"fra", "Включи французскую озвучку"},
    {"deu", "Включи немецкую озвучку"},
    {"rus-x-sndk", "Включи озвучку Сыендука"},
    {"rus-x-kubik", "Включи озвучку \"Кубик в кубе\""}
};

inline const THashMap<TStringBuf, TStringBuf> SUBTITLES_SUGGESTS {
    {"rus", "Включи русские субтитры"},
    {"eng", "Включи английские субтитры"},
    {"fra", "Включи французские субтитры"},
    {"deu", "Включи немецкие субтитры"},
    {"rus-x-kubik", "Включи субтитры \"Кубик в кубе\""}
};

inline const THashMap<TStringBuf, TStringBuf> SHORT_LANGUAGE_FORMAT {
    {"rus", "ru"},
    {"eng", "en"},
    {"fra", "fr"},
    {"deu", "de"}
};

TMaybe<TStringBuf> TryGetSuggest(
    const THashMap<TStringBuf, TStringBuf>& suggests,
    TStringBuf language);

void FillAudioStreamsAndSubtitlesInfo(
    TVector<NAlice::TAudioStreamOrSubtitle>& audioStreams,
    TVector<NAlice::TAudioStreamOrSubtitle>& subtitles,
    NAlice::TPlayerRestrictionConfig& playerRestrictionConfig,
    const NJson::TJsonValue& audioAndSubtitlesOttInfo);

void ParseAudioStreamsAndSubtitlesInfo(
        TVector<NAlice::TAudioStreamOrSubtitle>& audioStreams,
        TVector<NAlice::TAudioStreamOrSubtitle>& subtitles,
        const NJson::TJsonValue& stream);
void ParsePlayerRestrictionConfig(NAlice::TPlayerRestrictionConfig& playerRestrictionConfig,
                                  const NJson::TJsonValue& payload);

TVector<NAlice::TSkippableFragment> ParseSkippableFragments(const NJson::TJsonValue& skippableFragmentsInfo);

/* This should be consistent with
a.yandex-team.ru/arc/trunk/arcadia/smart_devices/android/quasar-app/src/main/java/ru/yandex/quasar/app/video/players/configurators/OttPlayerConfigurator.java */
template <typename T>
TMaybe<T> GetSupportedStreams(const T& payload) {

    if (payload.Has("allStreams")) {
        for (const auto& stream : payload["allStreams"].GetArray()) {
            if (!stream.Has("drmType") &&
                stream.Has("streamType") &&
                (stream["streamType"].GetString() == "HLS" || stream["streamType"].GetString() == "DASH"))
            {
                return stream;
            }
        }

        for (const auto& stream : payload["allStreams"].GetArray()) {
            if (stream.Has("drmType") && stream["drmType"].GetString() == "widevine" &&
                stream.Has("drmConfig") &&
                stream.Has("streamType") && stream["streamType"].GetString() == "DASH")
            {
                return stream;
            }
        }
    }

    if (payload.Has("streams")) {
        return payload["streams"];
    }

    if (payload.Has("allStreams") || !payload["allStreams"].GetArray().empty()) {
        return payload["allStreams"].GetArray().front();
    }

    return Nothing();
}

TMaybe<NAlice::TAudioStreamOrSubtitle> FindMostSuitableTrack(
    const TVector<NAlice::TAudioStreamOrSubtitle>& tracks,
    TStringBuf language);

} // namespace NAlice::NVideoCommon
