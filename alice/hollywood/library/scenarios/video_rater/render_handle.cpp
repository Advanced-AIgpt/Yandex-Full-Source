#include "render_handle.h"

#include "common.h"
#include "entity_search_adapter.h"

#include <alice/hollywood/library/datasync_adapter/datasync_adapter.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/suggesters/common/utils.h>
#include <alice/hollywood/library/scenarios/video_rater/proto/video_rater_state.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/proto.h>
#include "alice/megamind/protos/scenarios/directives.pb.h"

#include "util/generic/fwd.h"
#include <util/generic/variant.h>
#include <util/random/shuffle.h>
#include <util/string/cast.h>

using namespace NAlice::NScenarios;
using namespace NAlice::NHollywood;

namespace NAlice::NHollywood::NVideoRater {

namespace {

constexpr uint MAX_REASK_COUNT = 2;
const uint MAX_IDLE_SECONDS = 10 * 60;

const TString RATE_2_ACTION = "rate_2";
const TString RATE_3_ACTION = "rate_3";
const TString RATE_4_ACTION = "rate_4";
const TString RATE_5_ACTION = "rate_5";
const TString SKIP_ACTION = "skip";
const TString STOP_ACTION = "stop";

const THashMap<TString, TString> UGC_2_RU = {
    { "film", "фильм" },
    { "anim", "мультфильм" },
    { "anim-series", "мультфильм" },
    { "series", "сериал" },
};

TMaybe<TFrame> ChoosePhase(const TScenarioInputWrapper& input, bool isContinuing, bool isNewSession, EScenarioPhase& phase) {
    auto rawFrame = input.FindSemanticFrame(INIT_FRAME);
    if (rawFrame) {
        phase = EScenarioPhase::Init;
        return TFrame::FromProto(*rawFrame);
    }
    rawFrame = input.FindSemanticFrame(IRRELEVANT_FRAME);
    // New session should always start with init frame, otherwise it is a false activation
    if (rawFrame || !isContinuing || isNewSession) {
        phase = EScenarioPhase::Irrelevant;
        return Nothing();
    }
    rawFrame = input.FindSemanticFrame(QUIT_FRAME);
    if (rawFrame) {
        phase = EScenarioPhase::Quit;
        return TFrame::FromProto(*rawFrame);
    }
    rawFrame = input.FindSemanticFrame(RATE_FRAME);
    if (rawFrame) {
        phase = EScenarioPhase::Rate;
        return TFrame::FromProto(*rawFrame);
    }
    phase = EScenarioPhase::DontUnderstand;
    return Nothing();
}

NJson::TJsonValue MakeJsonRating(
    const TString& kinopoiskId,
    const i64 score,
    const TString& textScore,
    ui64 timestamp,
    const TString& timezone
) {
    return NJson::TJsonMap({
        { "kinopoisk_id", NJson::TJsonValue(kinopoiskId) },
        { "score", NJson::TJsonValue(score) },
        { "text_score", NJson::TJsonValue(textScore) },
        { "timestamp", NJson::TJsonValue(timestamp) },
        { "timezone", NJson::TJsonValue(timezone) },
    });
}

void LogRatedVideoEvent(TAnalyticsInfo& analyticsInfo, const TString& kinopoiskId, const i64 score, const TString& textScore) {
    auto* event = analyticsInfo.AddEvents();
    auto* ratedVideoEvent = event->MutableRatedVideoEvent();
    ratedVideoEvent->SetProviderItemId(kinopoiskId);
    ratedVideoEvent->SetRating(score);
    ratedVideoEvent->SetTextRating(textScore);
}

TMaybe<TVideoRaterItem> SampleUnratedItem(
    TScenarioHandleContext& ctx,
    const TVector<TVideoRaterItem>& movies,
    const NJson::TJsonValue& ratings
) {
    if (!movies) {
        LOG_ERR(ctx.Ctx.Logger()) << "Cannot find unrated movie: no movies available";
        return Nothing();
    }

    THashSet<TString> savedIdsSet;
    for (const auto& savedRating : ratings.GetArray()) {
        savedIdsSet.insert(savedRating["kinopoisk_id"].GetString());
    }

    for (const auto& movie : movies) {
        if (!savedIdsSet.contains(movie.GetKinopoiskId())) {
            return movie;
        }
    }

    LOG_ERR(ctx.Ctx.Logger()) << "Cannot find unrated movie: all movies are rated";
    return Nothing();
}

void GetMoviesFromEntitySearchResponse(TVector<TVideoRaterItem>& movies, const NJson::TJsonValue& responseBody) {
    const auto moviesRaw = responseBody["cards"][0]["parent_collection"]["object"].GetArray();
    for (const auto& movieDesc : moviesRaw) {
        const auto id = movieDesc["ids"]["kinopoisk"].GetString();
        const auto title = movieDesc["title"].GetString();
        const auto ugc_type = movieDesc["ugc_type"].GetString();

        if (id.empty() || title.empty() || !UGC_2_RU.contains(ugc_type)) {
            continue;
        }
        TVideoRaterItem item;
        item.SetName(title);
        item.SetKinopoiskId(id);
        item.SetRuType(
            UGC_2_RU.at(ugc_type)
        );
        item.SetJoke(Default<TString>());
        movies.push_back(std::move(item));
    }
}

NJson::TJsonValue GetRatingsFromDataSyncResponse(const NJson::TJsonValue& responseBody) {
    return JsonFromString(responseBody["value"].GetString());
}

TUpdateDatasyncDirective MakeUpdateDatasyncDirective(const TString& value) {
    TUpdateDatasyncDirective directive;
    directive.SetKey(TString{DATASYNC_KV_PATH});
    directive.SetStringValue(value);
    directive.SetMethod(TUpdateDatasyncDirective::Put);

    return directive;
}

}  // namespace

void TVideoRaterRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    TAnalyticsInfo analyticsInfo;

    TVector<TVideoRaterItem> movies;
    GetMoviesFromEntitySearchResponse(
        movies,
        RetireEntitySearchResponseItems(ctx)
    );
    LOG_INFO(ctx.Ctx.Logger()) << "Retried movie candidates from entity search";

    auto ratedMovies = GetRatingsFromDataSyncResponse(
        RetireDataSyncResponseItemsSafe(ctx).GetOrElse(NJson::TJsonValue("{\"value\": []}"))
    );

    LOG_INFO(ctx.Ctx.Logger()) << "Saved retrieved DataSync ratings to commit arguments";

    TVideoRaterState state;
    const auto& rawState = requestProto.GetBaseRequest().GetState();
    bool isContinuing = false;
    if (rawState.Is<TVideoRaterState>() && !request.IsNewSession()) {
        rawState.UnpackTo(&state);

        isContinuing = (
            !state.GetHasFinished()
            && (!state.GetLastRequestTimestamp() || request.ClientInfo().Epoch - state.GetLastRequestTimestamp() < MAX_IDLE_SECONDS)
        );
        state.SetLastRequestTimestamp(request.ClientInfo().Epoch);

        LOG_INFO(ctx.Ctx.Logger()) << "Video Rater state was found; HasFinished is " << state.GetHasFinished();
    } else if (request.IsNewSession()) {
        LOG_INFO(ctx.Ctx.Logger()) << "Video Rater state was dropped due to a new session";
    } else {
        LOG_INFO(ctx.Ctx.Logger()) << "Video Rater state was not found";
    }

    EScenarioPhase phase;
    TMaybe<TFrame> maybeFrame = ChoosePhase(request.Input(), isContinuing, request.IsNewSession(), phase);
    LOG_INFO(ctx.Ctx.Logger()) << "Video Rater scenario is in phase " << phase;

    TNlgData nlgData{ctx.Ctx.Logger(), request};
    NJson::TJsonValue attentions;
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    TString phrase = "undefined";
    switch (phase) {
        case EScenarioPhase::Irrelevant:
            builder.SetIrrelevant();
            break;
        case EScenarioPhase::Init:
            attentions["intro"] = true;
            state.SetHasFinished(false);
            break;
        case EScenarioPhase::Rate:
            if (const auto ratingPtr = maybeFrame->FindSlot(TStringBuf("rating"))) {
                const i64 score = ratingPtr->Value.As<i64>().GetOrElse(-1);
                const TVideoRaterItem& currentVideo = state.GetCurrentVideo();

                nlgData.Context["prev_video_title"] = currentVideo.GetName();
                nlgData.Context["prev_video_type"] = currentVideo.GetRuType();
                nlgData.Context["prev_video_rating"] = score;
                if (score >= 4) {
                    nlgData.Context["joke"] = currentVideo.GetJoke();
                }
                if (score == 0 || score == -1) {  // 0 means "did not watch"; -1 means "just skip"; -2 means "undefined rating"
                    attentions["prev_video_was_skipped"] = true;
                } else {
                    attentions["has_current_rating"] = true;
                }
                ratedMovies.AppendValue(
                    MakeJsonRating(
                        currentVideo.GetKinopoiskId(),
                        score,
                        request.Input().Utterance(),
                        request.ClientInfo().Epoch,
                        request.ClientInfo().Timezone
                    )
                );
                LogRatedVideoEvent(analyticsInfo, currentVideo.GetKinopoiskId(), score, request.Input().Utterance());
            }
            break;
        case EScenarioPhase::Quit:
            phrase = "goodbye";
            state.SetHasFinished(true);
            break;
        case EScenarioPhase::DontUnderstand:
            phrase = "dont_understand";
            state.SetReaskCount(state.GetReaskCount() + 1);
            if (state.GetReaskCount() > MAX_REASK_COUNT) {
                LOG_DEBUG(ctx.Ctx.Logger()) << "Video Rater scenario cannot parse phrase for the " << state.GetReaskCount() << " time. Quitting." ;
                builder.SetIrrelevant();
                state.SetHasFinished(true);
            }
            break;
    }

    if (phase == EScenarioPhase::Rate || phase == EScenarioPhase::Init) {
        if (auto maybeMovie = SampleUnratedItem(ctx, movies, ratedMovies)) {
            *state.MutableCurrentVideo() = *maybeMovie;
            phrase = "ask_current_film";
        } else {
            phrase = "no_more_questions";
            state.SetHasFinished(true);
        }
    }

    if (phase != EScenarioPhase::DontUnderstand) {
        state.SetReaskCount(0);
    }

    LOG_INFO(ctx.Ctx.Logger()) << "Video Rater phrase: " << phrase;

    nlgData.Context["attentions"] = attentions;
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    bodyBuilder.CreateAnalyticsInfoBuilder(analyticsInfo);
    auto response = std::move(builder).BuildResponse();
    auto& responseBody = *response->MutableResponseBody();
    responseBody.MutableState()->PackFrom(state);

    if (phase == EScenarioPhase::Rate) {
        const TString updateDatasyncValue = request.ExpFlags().contains(EXP_HW_VIDEO_RATER_CLEAR_HISTORY)
            ? "[]"
            : JsonToString(ratedMovies);

        *responseBody.AddServerDirectives()->MutableUpdateDatasyncDirective() = MakeUpdateDatasyncDirective(updateDatasyncValue);
    }

    if (phase == EScenarioPhase::Rate || phase == EScenarioPhase::Init || phase == EScenarioPhase::DontUnderstand) {
        if (!state.GetHasFinished()) {
            LOG_INFO(ctx.Ctx.Logger()) << "Video Rater: adding suggest buttons";

            responseBody.SetExpectsRequest(true);

            AddTypeTextSuggest("Пропустить", SKIP_ACTION, bodyBuilder);
            AddTypeTextSuggest("Лайк", RATE_4_ACTION, bodyBuilder);
            AddTypeTextSuggest("Дизлайк", RATE_2_ACTION, bodyBuilder);
            AddTypeTextSuggest("Хватит", STOP_ACTION, bodyBuilder);
            nlgData.Context["video_title"] = state.GetCurrentVideo().GetName();
            nlgData.Context["video_type"] = state.GetCurrentVideo().GetRuType();
        }
        responseBody.MutableLayout()->SetShouldListen(true);
    }
    bodyBuilder.AddRenderedTextWithButtonsAndVoice("video_rater", phrase, /* buttons = */ {}, nlgData);

    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);

    LOG_INFO(ctx.Ctx.Logger()) << "Video Rater: successfully added run response";
}

} // namespace NAlice::NHollywood::NVideoRater
