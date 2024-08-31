#include "audio_and_subtitle_helper.h"

#include <util/string/cast.h>

namespace NAlice::NVideoCommon {

TMaybe<TStringBuf> TryGetSuggest(
    const THashMap<TStringBuf, TStringBuf>& suggests,
    TStringBuf language)
{
    for (size_t length = language.size(); length > 0; length--) {
        if (const auto* suggest = suggests.FindPtr(language.SubString(0, length))) {
            return *suggest;
        }
    }
    return Nothing();
}

/* This should be consistent with
a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/video/kinopoisk_provider.cpp */
void FillAudioStreamsAndSubtitlesInfo(
    TVector<NAlice::TAudioStreamOrSubtitle>& audioStreams,
    TVector<NAlice::TAudioStreamOrSubtitle>& subtitles,
    NAlice::TPlayerRestrictionConfig& playerRestrictionConfig,
    const NJson::TJsonValue& audioAndSubtitlesOttInfo)
{
    const auto& payload = audioAndSubtitlesOttInfo["payload"];
    const auto& streams = GetSupportedStreams(payload);
    if (!streams) {
        return;
    }
    ParseAudioStreamsAndSubtitlesInfo(audioStreams, subtitles, streams.GetRef());
    ParsePlayerRestrictionConfig(playerRestrictionConfig, payload);
}

void ParseAudioStreamsAndSubtitlesInfo(
        TVector<NAlice::TAudioStreamOrSubtitle>& audioStreams,
        TVector<NAlice::TAudioStreamOrSubtitle>& subtitles,
        const NJson::TJsonValue& stream) {
    ui32 currentIndex = 1;
    if (const auto* audioInfos = stream.GetValueByPath("audio"); audioInfos && audioInfos->IsArray()) {
        for (const auto& audioInfo : audioInfos->GetArray()) {
            if (!audioInfo.Has("language")) {
                continue;
            }
            NAlice::TAudioStreamOrSubtitle audioStream;
            TString language = audioInfo["language"].GetString();
            language.to_lower();

            TStringBuf title = audioInfo.Has("title") ? audioInfo["title"].GetString() : language;
            audioStream.SetLanguage(language);
            audioStream.SetTitle(ToString(title));
            audioStream.SetIndex(currentIndex++);

            if (const TMaybe<TStringBuf>& suggest = TryGetSuggest(AUDIO_SUGGESTS, TStringBuf(language))) {
                audioStream.SetSuggest(ToString(*suggest));
            }

            audioStreams.push_back(std::move(audioStream));
        }
    }

    // Additional label "off" for subtitles
    {
        NAlice::TAudioStreamOrSubtitle subtitleOff;
        subtitleOff.SetLanguage(ToString(LANGUAGE_SUBTITLE_OFF));
        subtitleOff.SetTitle("Выключены");
        subtitleOff.SetIndex(currentIndex++);
        subtitleOff.SetSuggest(ToString(SUGGEST_SUBTITLES_TURN_OFF));
        subtitles.push_back(std::move(subtitleOff));
    }

    if (const auto* subtitlesInfos = stream.GetValueByPath("subtitles"); subtitlesInfos && subtitlesInfos->IsArray()) {
        for (const auto& subtitleInfo : subtitlesInfos->GetArray()) {
            if (!subtitleInfo.Has("language")) {
                continue;
            }
            NAlice::TAudioStreamOrSubtitle subtitle;
            TString language = subtitleInfo["language"].GetString();
            language.to_lower();

            TStringBuf title = subtitleInfo.Has("title") ? subtitleInfo["title"].GetString() : language;
            subtitle.SetLanguage(language);
            subtitle.SetTitle(ToString(title));
            subtitle.SetIndex(currentIndex++);

            if (subtitlesInfos->GetArray().size() == 1) {
                subtitle.SetSuggest(ToString(SUGGEST_SUBTITLES_TURN_ON));
            } else if (const TMaybe<TStringBuf> suggest = TryGetSuggest(SUBTITLES_SUGGESTS, TStringBuf(language))) {
                subtitle.SetSuggest(ToString(*suggest));
            }

            subtitles.push_back(std::move(subtitle));
        }
    }
}

void ParsePlayerRestrictionConfig(NAlice::TPlayerRestrictionConfig& playerRestrictionConfig,
                                  const NJson::TJsonValue& payload) {
    if (payload.Has(PLAYER_RESTRICTION_CONFIG) && payload[PLAYER_RESTRICTION_CONFIG].Has(SUBTITLES_BUTTON_ENABLE)) {
        bool isButtonEnable = payload[PLAYER_RESTRICTION_CONFIG][SUBTITLES_BUTTON_ENABLE].GetBoolean();
        playerRestrictionConfig.SetSubtitlesButtonEnable(isButtonEnable);
    } else if (payload.Has(PLAYER_RESTRICTION_CONFIG_) && payload[PLAYER_RESTRICTION_CONFIG_].Has(SUBTITLES_BUTTON_ENABLE_)) {
        bool isButtonEnable = payload[PLAYER_RESTRICTION_CONFIG_][SUBTITLES_BUTTON_ENABLE_].GetBoolean();
        playerRestrictionConfig.SetSubtitlesButtonEnable(isButtonEnable);
    } else {
        playerRestrictionConfig.SetSubtitlesButtonEnable(true);
    }
}

TVector<NAlice::TSkippableFragment> ParseSkippableFragments(const NJson::TJsonValue& skippableFragmentsInfo) {
    TVector<NAlice::TSkippableFragment> skippableFragments;
    if (!skippableFragmentsInfo.IsArray()) {
        return skippableFragments;
    }
    for (const auto& fragmentJson : skippableFragmentsInfo.GetArray()) {
        if (!fragmentJson["startTime"].IsDouble() || !fragmentJson["endTime"].IsDouble()) {
            continue;
        }
        NAlice::TSkippableFragment skippableFragment;
        skippableFragment.SetStartTime(fragmentJson["startTime"].GetDouble());
        skippableFragment.SetEndTime(fragmentJson["endTime"].GetDouble());
        skippableFragment.SetType(fragmentJson["type"].GetString());
        skippableFragments.emplace_back(std::move(skippableFragment));
    }
    SortBy(skippableFragments, [] (const auto& fragment) {
        return std::make_pair(fragment.GetStartTime(), fragment.GetEndTime());
    });
    return skippableFragments;
}

/* This should be consistent with
a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/video/change_track.cpp */
TMaybe<NAlice::TAudioStreamOrSubtitle> FindMostSuitableTrack(
    const TVector<NAlice::TAudioStreamOrSubtitle>& tracks,
    TStringBuf language)
{
    NAlice::TAudioStreamOrSubtitle result;

    for (const auto& track : tracks) {
        const TStringBuf currentLanguage = track.GetLanguage();

        if (currentLanguage.EndsWith(SUFFIX_LANG_18PLUS) &&
            language == LANGUAGE_18PLUS)
        {
            result.SetLanguage(TString{currentLanguage});
            result.SetTitle(track.GetTitle());
            return result;
        }

        if (currentLanguage.StartsWith(language) &&
            (result.GetLanguage().empty() || currentLanguage.size() < result.GetLanguage().size()))
        {
            result.SetLanguage(TString{currentLanguage});
            result.SetTitle(track.GetTitle());
        }
    }

    if (!result.GetLanguage().is_null()) {
        return result;
    }


    if (SHORT_LANGUAGE_FORMAT.contains(language)) {
        return FindMostSuitableTrack(tracks, SHORT_LANGUAGE_FORMAT.at(language));
    }

    return Nothing();
}

} // namespace NAlice::NVideoCommon
