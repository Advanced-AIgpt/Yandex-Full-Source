#include "catalog.h"

#include "answer.h"

#include <alice/library/client/client_info.h>
#include <alice/library/experiments/experiments.h>

#include <util/string/split.h>

namespace {

const TVector<TStringBuf> DEFAULT_FAIRY_TALE_GENRES = {
    TStringBuf("fairytales"),
    TStringBuf("forchildren"),
};

bool CheckMusicAnswerField(const NJson::TJsonValue& answer, const TStringBuf field, const TVector<TStringBuf>& expectedValues = {}) {
    const TVector<NJson::TJsonValue> values = {
        answer[field],
        answer["album"][field],
        answer["firstTrack"][field],
        answer["firstTrack"]["album"][field],
    };

    if (expectedValues.empty()) {
        return AnyOf(values, [](const NJson::TJsonValue& value) { return value.GetBoolean(); });
    }
    return AnyOf(values, [&expectedValues](const NJson::TJsonValue& value) { return  IsIn(expectedValues, value.GetString()); });
}

}

namespace NAlice::NMusic {


TVector<NJson::TJsonValue> ParseMusicCatalogResponseToMusicCatalogAnswers(
    const NJson::TJsonValue& musicCatalogJsonResponse
) {
    TVector<NJson::TJsonValue> results;
    auto musicWebCatalogResult = musicCatalogJsonResponse["result"];
    for (auto&& elem : musicWebCatalogResult.GetArray()) {
        if (const auto answerType = elem["request"]["type"].GetString()) {
            auto musicAnswer = elem["response"];

            // Copy from https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/catalog.cpp?rev=r9164626#L47-88
            auto value = musicAnswer.IsArray() ? musicAnswer[0] : musicAnswer;

            // FIXME (danibw@): Workaround for https://st.yandex-team.ru/MUSICBACKEND-5686.
            // Change 'while' to 'if' after the task is done.
            TMaybe<NJson::TJsonValue> allPartsContainer;
            while (value.Has(answerType)) {
                if (value.Has("allPartsContainer")) {
                    allPartsContainer = value["allPartsContainer"];
                }
                value = value[answerType];
            }

            if (allPartsContainer.Defined()) {
                value["allPartsContainer"] = allPartsContainer.GetRef();
            }

            if (answerType == "track") {
                if (!value["available"].GetBoolean()) {
                    continue;
                }
                value["id"] = value["realId"];
            } else {
                const auto deprecatedId =
                    ((answerType == "album")
                         ? value["deprecation"]["targetAlbumId"]
                         : ((answerType == "artist") ? value["deprecation"]["targetArtistId"] : NJson::TJsonValue(NJson::JSON_NULL)));

                if (deprecatedId.IsDefined()) {
                    value["id"] = deprecatedId;
                } else if (!value["available"].GetBoolean()) {
                    continue;
                }
            }

            value["type"] = answerType;

            results.emplace_back(value);
        }
    }

    return results;
}

NJson::TJsonValue ConvertMusicCatalogAnswerToMusicInfo(
    const TClientInfo& clientInfo,
    const bool supportsIntentUrls,
    const bool needAutoplay,
    const NJson::TJsonValue& musicCatalogAnswer
) {
    const TString& answerType = musicCatalogAnswer["type"].GetString();
    if (answerType == "track") {
        return MakeBestTrack(clientInfo, supportsIntentUrls, needAutoplay, musicCatalogAnswer);
    } else if (answerType == "album") {
        return MakeBestAlbum(clientInfo, supportsIntentUrls, needAutoplay, musicCatalogAnswer, /* firstTrack */ false);
    } else if (answerType == "artist") {
        return MakeBestArtist(clientInfo, supportsIntentUrls, needAutoplay, musicCatalogAnswer);
    } else if (answerType == "playlist") {
        return MakePlaylist(clientInfo, supportsIntentUrls, needAutoplay, musicCatalogAnswer);
    }

    return NJson::TJsonValue(NJson::JSON_NULL);
}

bool IsChildContent(
    const NJson::TJsonValue& musicInfo,
    const THashMap<TString, TMaybe<TString>>& expFlags
) {
    const auto& fairyTaleGenres = GetFairyTaleGenres(expFlags);

    return CheckMusicAnswerField(musicInfo, "genre", fairyTaleGenres) ||
        CheckMusicAnswerField(musicInfo, "subtype", {"fairy-tale"}) ||
        CheckMusicAnswerField(musicInfo, "childContent");
}

TVector<TStringBuf> GetFairyTaleGenres(
    const THashMap<TString, TMaybe<TString>>& expFlags
) {
    const auto genreStr = NAlice::GetExperimentValueWithPrefix(expFlags, "fairy_tale_genre_filter=");
    if (genreStr.Defined()) {
        return StringSplitter(genreStr.GetRef()).Split(',');
    }
    return {DEFAULT_FAIRY_TALE_GENRES.begin(), DEFAULT_FAIRY_TALE_GENRES.end()};
}

NAlice::TMusicPlaySemanticFrame ConstructMusicPlaySemanticFrame(
    const NJson::TJsonValue& musicInfo,
    const bool repeat
) {
    NAlice::TMusicPlaySemanticFrame musicPlaySemanticFrame;

    if (musicInfo.Has("id")) {
        musicPlaySemanticFrame.MutableObjectId()->SetStringValue(musicInfo["id"].GetString());
    }

    if (musicInfo.Has("type")) {
        TString type = musicInfo["type"].GetString();

        // Capitalize first letter because music Backend returns type as string consisting of lowercase letters
        // while MusicPlaySemanticFrame has ObjectType slot with enum values starting from capital letter
        type.to_upper(0, 1);

        NAlice::TMusicPlayObjectTypeSlot_EValue musicPlayObjectType;
        if (NAlice::TMusicPlayObjectTypeSlot_EValue_Parse(type, &musicPlayObjectType)) {
            musicPlaySemanticFrame.MutableObjectType()->SetEnumValue(musicPlayObjectType);
        }
    }

    if (musicInfo.Has("filters")) {
        if (musicInfo["filters"].Has("genre")) {
            musicPlaySemanticFrame.MutableGenre()->SetGenreValue(musicInfo["filters"]["genre"].GetString());
        }

        if (musicInfo["filters"].Has("mood")) {
            musicPlaySemanticFrame.MutableMood()->SetMoodValue(musicInfo["filters"]["mood"].GetString());
        }

        if (musicInfo["filters"].Has("activity")) {
            musicPlaySemanticFrame.MutableActivity()->SetActivityValue(musicInfo["filters"]["activity"].GetString());
        }

        if (musicInfo["filters"].Has("epoch")) {
            musicPlaySemanticFrame.MutableEpoch()->SetEpochValue(musicInfo["filters"]["epoch"].GetString());
        }

        if (musicInfo["filters"].Has("personality")) {
            musicPlaySemanticFrame.MutablePersonality()->SetPersonalityValue(musicInfo["filters"]["personality"].GetString());
        }

        if (musicInfo["filters"].Has("special_playlist")) {
            musicPlaySemanticFrame.MutableSpecialPlaylist()->SetSpecialPlaylistValue(musicInfo["filters"]["special_playlist"].GetString());
        }
    }

    musicPlaySemanticFrame.MutableDisableNlg()->SetBoolValue(true);

    if (repeat) {
        musicPlaySemanticFrame.MutableRepeat()->SetRepeatValue("All");
    }

    return musicPlaySemanticFrame;
}

} // NAlice::NMusic
