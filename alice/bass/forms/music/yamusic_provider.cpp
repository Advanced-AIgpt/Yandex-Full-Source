#include "providers.h"

#include <alice/bass/forms/search/search.h>

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/music/defs.h>
#include <alice/library/util/system_time.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NBASS::NMusic {

namespace {

const size_t MAX_SUGGEST_NUM = 3;

} // end anon namespace

TYandexMusicProvider::TYandexMusicProvider(TContext& ctx)
    : TBaseMusicProvider(ctx)
    , ServiceAnswer(ctx.ClientFeatures())
    , SpecialPlaylist("")
{
}

bool TYandexMusicProvider::InitRequestParams(const NSc::TValue& slotData) {
    LOG(DEBUG) << "TYandexMusicProvider::InitRequestParams called" << Endl;
    if (DataGetHasKey(slotData, RESULT_SLOTS)) {
        return true;
    }

    TContext::TSlot* slotActions = Ctx.GetSlot("action_request");
    if (!IsSlotEmpty(slotActions) && slotActions->Value.GetString() == TStringBuf("autoplay")) {
        Autoplay = true;
    }

    if (DataGetHasKey(slotData, CUSTOM_SLOTS)) {
        TStringBuf sp = slotData[CUSTOM_SLOTS].Get("special_playlist").GetString();
        if (!sp.empty()) {
            SpecialPlaylist = sp;
            return true;
        }

        SpecialAnswerRawInfo = slotData[CUSTOM_SLOTS].Get(NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO).GetString();
        if (SpecialAnswerRawInfo) {
            return true;
        }
    }

    if (!DataGetHasKey(slotData, SEARCH_SLOTS) &&
        !DataGetHasKey(slotData, OTHER_FILTERS_SLOTS) &&
        !DataGetHasKey(slotData, MAIN_FILTERS_SLOTS) &&
        slotData["custom"].Has("novelty") && slotData["custom"]["novelty"].GetString() == TStringBuf("new")) {
        SpecialPlaylist = TStringBuf("new");
        return true;
    }

    SearchText = MergeTextFromSlots(Ctx, TString{Ctx.Meta().Utterance()}, /* alarm= */ false);
    LOG(DEBUG) << "MUSIC SEARCH TEXT: " << SearchText << Endl;
    if (!SearchText.empty()) {
        return true;
    }

    return false;
}

NHttpFetcher::THandle::TRef TYandexMusicProvider::CreateSearchHandler(NHttpFetcher::IMultiRequest::TRef multiRequest) const {
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("text"), SearchText);
    cgi.InsertUnescaped(TStringBuf("type"), TStringBuf("all"));
    cgi.InsertUnescaped(TStringBuf("page"), TStringBuf("0"));
    cgi.InsertUnescaped(TStringBuf("overrideExperiments"), TStringBuilder() << "{\"musicSearchFormula\": \"default\"}");
    cgi.InsertUnescaped(TStringBuf("nocorrect"), TStringBuf("false"));
    cgi.InsertUnescaped(TStringBuf("playlist-in-best"), TStringBuf("true"));

    return CreateRequestHandler(Ctx.GetSources().Music(TStringBuf("/search")), cgi, multiRequest);
}

TResultValue TYandexMusicProvider::Apply(NSc::TValue* out,
                                         NSc::TValue&& applyArguments) {
    if (auto* webAnswer = applyArguments.GetNoAdd(WEB_ANSWER)) {
        if (!webAnswer->IsNull()) {
            *out = std::move(*webAnswer);
            return TResultValue();
        }
    }

    if (SpecialPlaylist) {
        if (SpecialPlaylist == TStringBuf("alice")) {
            SpecialPlaylist = TStringBuf("origin");
        }
        if (!ServiceAnswer.AnswerWithSpecialPlaylist(SpecialPlaylist, Autoplay, out)) {
            return TError(
                TError::EType::MUSICERROR,
                TStringBuf("unknown_special_playlist")
            );
        }
        return TResultValue();
    }

    if (SpecialAnswerRawInfo) {
        if (!ServiceAnswer.MakeSpecialAnswer(SpecialAnswerRawInfo, Autoplay, out)) {
            return TError(
                TError::EType::MUSICERROR,
                TStringBuf("invalid_special_answer_info")
            );
        }
        return TResultValue();
    }

    NSc::TValue answer;

    NHttpFetcher::THandle::TRef searchHandler = CreateSearchHandler();
    if (!searchHandler) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("cannot_create_request")
        );
    }

    const auto startMillis = NAlice::SystemTimeNowMillis();
    TResultValue reqError = WaitAndParseResponse(searchHandler, &answer);
    Ctx.AddStatsCounter("MusicSearch_milliseconds", NAlice::SystemTimeNowMillis() - startMillis);
    if (reqError) {
        return reqError;
    }

    ServiceAnswer.InitWithSearchAnswer(answer.Get("result"), Autoplay);
    if (!ServiceAnswer.ConvertAnswerToOutputFormat(out)) {
        // if there is no music result switch to search
        if (TSearchFormHandler::SetAsResponse(Ctx, false)) {
            return Ctx.RunResponseFormHandler();
        } else {
            return TError(TError::EType::MUSICERROR, ERROR_MUSIC_NOT_FOUND);
        }
    }

    return TResultValue();
}

void TYandexMusicProvider::AddSuggest(const NSc::TValue& result) {
    TStringBuf resType = result.Get("type").GetString();
    if (!resType || !result.Has("id")) {
        return;
    }

    TStringBuilder path;
    path << '/';

    if (resType == TStringBuf("artist")) {
        // similar artists
        path << "artists/" << result.Get("id").ForceString() << "/brief-info";
        MakeSuggest(resType, path, TStringBuf("result/similarArtists"), TStringBuf(), TCgiParameters());
        return;
    } else if (resType == TStringBuf("track")) {
        if (0 && !Ctx.UserAuthorizationHeader().empty()) {
            // similar tracks for authorized users (We'll do it later!)
            path << "tracks/" << result.Get("id").ForceString() << "/similar";
            MakeSuggest(resType, path, TStringBuf("result/similarTracks"), TStringBuf(), TCgiParameters());
            return;
        } else {
            // similar artists for all
            path << "artists/" << result.TrySelect("artists/0/id").ForceString() << "/brief-info";
            MakeSuggest(TStringBuf("artist"), path, TStringBuf("result/similarArtists"), TStringBuf(), TCgiParameters());
            return;
        }
    } else if (resType == TStringBuf("album")) {
        const NSc::TValue& artist = result.TrySelect("artists/0");
        if (artist.Get("is_various").GetBool(false)) {
            // If there are many artists in album,
            // suggest best artists of this genre
            path << "genre-overview";
            TCgiParameters cgi;
            cgi.InsertEscaped("genre", result.Get("genre").GetString());
            cgi.InsertEscaped("tracks-count", "0");
            cgi.InsertEscaped("artists-count", ToString(MAX_SUGGEST_NUM));
            cgi.InsertEscaped("albums-count", "0");
            cgi.InsertEscaped("promotions-count", "0");
            MakeSuggest(TStringBuf("artist"), path, TStringBuf("result/artists"), TStringBuf(), cgi);
            return;
        } else {
            // albums of the first artist (excluding this album)
            path << "artists/" << artist.Get("id").ForceString() << "/brief-info";
            MakeSuggest(TStringBuf("album"), path, TStringBuf("result/albums"), result.Get("id").ForceString(), TCgiParameters());
            return;
        }
    }
}

void TYandexMusicProvider::MakeSuggest(TStringBuf section, TStringBuf path, TStringBuf jsonPath, TStringBuf id, const TCgiParameters& cgi) {
    NHttpFetcher::THandle::TRef handler = CreateRequestHandler(Ctx.GetSources().MusicSuggests(path), cgi);
    if (!handler) {
        return;
    }
    NSc::TValue json;

    const auto startMillis = NAlice::SystemTimeNowMillis();
    TResultValue reqError = WaitAndParseResponse(handler, &json);
    Ctx.AddStatsCounter("MusicSuggests_milliseconds", NAlice::SystemTimeNowMillis() - startMillis);
    if (reqError) {
        return;
    }

    bool autoplay = true;

    size_t counter = 0;
    for (const auto& j : json.TrySelect(jsonPath).GetArray()) {
        if (!id.empty() && id == j.Get("id").ForceString()) {
            continue;
        }
        TYandexMusicAnswer answer(Ctx.ClientFeatures());
        answer.InitWithRelatedAnswer(ToString(section), j, autoplay);
        NSc::TValue suggest;
        if (!answer.ConvertAnswerToOutputFormat(&suggest)) {
            continue;
        }

        NSc::TValue formUpdate;
        formUpdate["name"] = MUSIC_PLAY_FORM_NAME;
        formUpdate["resubmit"].SetBool(true);
        NSc::TValue& slot = formUpdate["slots"].SetArray().Push();
        slot["name"].SetString(TStringBuf("answer"));
        slot["type"].SetString(TStringBuf("music_result"));
        slot["optional"].SetBool(true);
        slot["value"] = suggest;
        Ctx.AddSuggest(TStringBuilder() << TStringBuf("music__suggest_") << section, suggest, formUpdate);

        ++counter;
        if (counter >= MAX_SUGGEST_NUM) {
            break;
        }
    }
}

void TYandexMusicProvider::AdjustCommandData(NSc::TValue& commandData) {
    if (Ctx.MetaClientInfo().IsYaAuto()) {
        commandData["background"] = true;
    }
}

} // NBASS::NMusic
