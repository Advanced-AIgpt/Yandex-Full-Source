#include "calcers.h"

#include "search_hosts_features.h"

#include <alice/megamind/library/context/wizard_response.h>
#include <alice/megamind/library/search/search.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/factors/dcg.h>
#include <alice/library/factors/defs.h>
#include <alice/library/music/defs.h>
#include <alice/library/response_similarity/response_similarity.h>
#include <alice/library/search/defs.h>
#include <alice/library/video_common/defs.h>

#include <kernel/alice/begemot_nlu_factors_info/fill_factors/fill_nlu_factors.h>
#include <kernel/alice/begemot_query_factors_info/factors_gen.h>
#include <kernel/alice/direct_scenario_factors_info/fill_factors/fill_direct_scenario_factors.h>
#include <kernel/alice/gc_scenario_factors_info/fill_factors/fill_gc_scenario_factors.h>
#include <kernel/alice/music_scenario_factors_info/fill_factors/fill_music_scenario_factors.h>
#include <kernel/alice/search_scenario_factors_info/fill_factors/fill_search_scenario_factors.h>
#include <kernel/alice/video_scenario/fill_factors/video_scenario/fill_video_scenario_factors.h>
#include <kernel/begemot_query_factors_info/fill_factors/fill_begemot_query_factors.h>
#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/scheme/scheme.h>

#include <search/web/blender/factors_info/factors_gen.h>

#include <util/string/join.h>

namespace NAlice {

namespace {

const THashMap<TStringBuf, THashSet<TString>> MUSIC_PLAY_TRUSTED_SLOT_VALUES = {
    { NAlice::NMusic::SLOT_SPECIAL_PLAYLIST, {"never_heard", "recent_tracks", "missed_likes", "chart", "origin",
                                              "alice", "playlist_of_the_day", "family", "kinopoisk"}},
    { NAlice::NMusic::SLOT_STREAM, {"my-wave", "recent-tracks", "never-heard", "flashback", "collection", "hits"}}};
const THashMap<TStringBuf, THashSet<TString>> MUSIC_PLAY_IGNORED_SLOT_VALUES = {
    { NAlice::NMusic::SLOT_ACTION_REQUEST, {"autoplay"}}};

struct TBegemotScenarioFactors {
    TStringBuf ScenarioName;
    NAliceBegemotQueryFactors::EFactorId TolokaLSTMFactorIndex;
    NAliceBegemotQueryFactors::EFactorId ScenariosLSTMFactorIndex;
    NAliceBegemotQueryFactors::EFactorId IsGranetRecognizableFactorIndex;

    constexpr TBegemotScenarioFactors(
        const TStringBuf scenarioName,
        const NAliceBegemotQueryFactors::EFactorId tolokaLSTMFactorIndex,
        const NAliceBegemotQueryFactors::EFactorId scenariosLSTMFactorIndex,
        const NAliceBegemotQueryFactors::EFactorId isGranetRecognizableFactorIndex
    ) noexcept
        : ScenarioName{scenarioName}
        , TolokaLSTMFactorIndex{tolokaLSTMFactorIndex}
        , ScenariosLSTMFactorIndex{scenariosLSTMFactorIndex}
        , IsGranetRecognizableFactorIndex{isGranetRecognizableFactorIndex}
    {
    }
};

void FillBinaryClassifierQueryFactor(
    const ::google::protobuf::Map<TString, float>& probabilities,
    const TStringBuf scenarioName,
    const NAliceBegemotQueryFactors::EFactorId idx,
    const TFactorView view
) {
    if (idx == NAliceBegemotQueryFactors::FI_FACTOR_COUNT) {
        return;
    }
    const auto it = probabilities.find(scenarioName);
    if (it != probabilities.end()) {
        view[idx] = it->second;
        return;
    }
}

void FillGcClassifierQueryFactor(
    const TWizardResponse& response,
    const NAliceBegemotQueryFactors::EFactorId idx,
    const TFactorView view
) {
    if (idx == NAliceBegemotQueryFactors::FI_FACTOR_COUNT) {
        return;
    }
    if (response.GetProtoResponse().GetAliceGcDssmClassifier().HasScore()) {
        view[idx] = response.GetProtoResponse().GetAliceGcDssmClassifier().GetScore();
        return;
    }
}

void FillGranetQueryFactor(
    const TWizardResponse& response,
    const TBegemotScenarioFactors& scenarioFactors,
    const TFactorView view
) noexcept {
    if (scenarioFactors.IsGranetRecognizableFactorIndex == NAliceBegemotQueryFactors::FI_FACTOR_COUNT) {
        return;
    }
    view[scenarioFactors.IsGranetRecognizableFactorIndex] = static_cast<float>(
        response.HasGranetFrame(scenarioFactors.ScenarioName)
    );
}

void FillFstFactor(
    const ::google::protobuf::RepeatedPtrField<NNlu::TFstEntity>& fstEntities,
    const TStringBuf type,
    const NAliceBegemotQueryFactors::EFactorId idx,
    const TFactorView view
) noexcept {
    for (const auto& fstEntity : fstEntities) {
        if (fstEntity.GetType() == type) {
            view[idx] = 1.0f;
            return;
        }
    }
}

void FillCustomEntityFactor(
    const TWizardResponse& response,
    const TStringBuf type,
    const NAliceBegemotQueryFactors::EFactorId idx,
    const TFactorView view
) {
    for (const auto& entity : response.GetProtoResponse().GetCustomEntities().GetValues()) {
        for (const auto& item : entity.GetCustomEntityValues()) {
            if (item.GetType() == type) {
                view[idx] = 1.0f;
                return;
            }
        }
    }
}

void FillEntityFinderFactor(
    const TStringBuf entity,
    const TStringBuf type,
    const NAliceBegemotQueryFactors::EFactorId idx,
    const TFactorView view
) {
    if (entity.empty()) {
        return;
    }

    TStringBuf key, begin, end, ontoId, weight, wType, fbType, cType;
    Split(entity, '\t', key, begin, end, ontoId, weight, wType, fbType, cType);
    if (wType == type || (type.StartsWith("fb:") && fbType.find(type) != TString::npos)) {
        view[idx] = 1.0f;
        return;
    }
}

void FillEntityFinderFactor(
    const TWizardResponse& response,
    const TStringBuf type,
    const NAliceBegemotQueryFactors::EFactorId idx,
    const TFactorView view
) {
    for (const auto& winner : response.GetProtoResponse().GetEntityFinder().GetWinner()) {
        FillEntityFinderFactor(winner, type, idx, view);
    }
}

void FillFrameHasTrustedSlotValueFactor(
    const TWizardResponse& response,
    const TStringBuf expectedFrame,
    const THashMap<TStringBuf, THashSet<TString>>& trustedSlotVals,
    const THashMap<TStringBuf, THashSet<TString>>& ignoredSlotVals,
    const NAliceBegemotQueryFactors::EFactorId idx,
    const TFactorView view
) {
    if (const TSemanticFrame* frame = response.GetRequestFrame(expectedFrame); frame) {
        bool hasTrustedSlotValue = false;
        for (const auto& slot : frame->GetSlots()) {
            const auto trustedSlotIt = trustedSlotVals.find(slot.GetName());
            if (trustedSlotIt != trustedSlotVals.end() && trustedSlotIt->second.contains(slot.GetValue())) {
                hasTrustedSlotValue = true;
                continue;
            }
            const auto ignoredSlotIt = ignoredSlotVals.find(slot.GetName());
            if (ignoredSlotIt == ignoredSlotVals.end() || !ignoredSlotIt->second.contains(slot.GetValue())) {
                return;
            }
        }
        if (hasTrustedSlotValue) {
            view[idx] = 1.0f;
        }
    }
}

void FillSearchBegemotFactors(const TSearchResponse::TBgFactors* factors, const TFactorView view) {
    if (!factors) {
        return;
    }

    NBegemotQueryFactors::FillBegemotQueryFactors(*factors, view);
}

void FillSearchBlenderFactors(const TSearchResponse::TApplyBlenderFactors* factors, const TFactorView view) {
    if (!factors) {
        return;
    }

    const ui64 upTo = Min(factors->size(), view.Size());
    for (ui64 i = 0; i < upTo; ++i) {
        view[i] = (*factors)[i];
    }
}

void FillWebSearchScenarioFactors(const TStringBuf utterance, const NScenarios::TWebSearchDocs& docs, const TFactorView view) {
    using namespace NAliceSearchScenario;
    using namespace NResponseSimilarity;

    ui32 position = 0;
    THostsPositions hostsPositions;
    TVector<TSimilarity> titleSimilarities;
    TVector<TSimilarity> headlineSimilarities;

    for (const NScenarios::TWebSearchDoc& doc : docs.GetDocs()) {
        ++position;

        ProcessDocWebResult(doc, position, utterance, hostsPositions, titleSimilarities, headlineSimilarities);
        ProcessDocWizardResult(doc, CalcDCG(position), view);
        ProcessDocSnippetResult(doc, CalcDCG(position), view);
    }

    view[FI_MUSIC_HOSTS_COUNT] = hostsPositions.Music.size();
    view[FI_VIDEO_HOSTS_COUNT] = hostsPositions.Video.size();

    view[FI_MUSIC_HOSTS_DCG_AT_1] = CalcDCGAt(hostsPositions.Music, 1);
    view[FI_VIDEO_HOSTS_DCG_AT_1] = CalcDCGAt(hostsPositions.Video, 1);
    view[FI_MUSIC_HOSTS_DCG_AT_5] = CalcDCGAt(hostsPositions.Music, 5);
    view[FI_VIDEO_HOSTS_DCG_AT_5] = CalcDCGAt(hostsPositions.Video, 5);
    view[FI_MUSIC_HOSTS_DCG] = CalcDCGAt(hostsPositions.Music, hostsPositions.Music.size());
    view[FI_VIDEO_HOSTS_DCG] = CalcDCGAt(hostsPositions.Video, hostsPositions.Video.size());

    FillHostDCG(hostsPositions, view);

    const TSimilarity titleResult = AggregateSimilarity(titleSimilarities);
    const TSimilarity headlineResult = AggregateSimilarity(headlineSimilarities);

    const auto& queryInResponse = titleResult.GetQueryInResponse();
    view[FI_QUERY_IN_TITLE_RATIO_MAX] = queryInResponse.GetMax();
    view[FI_QUERY_IN_TITLE_RATIO_MEAN] = queryInResponse.GetMean();
    view[FI_QUERY_IN_TITLE_RATIO_MIN] = queryInResponse.GetMin();

    const auto& responseInQuery = titleResult.GetResponseInQuery();
    view[FI_TITLE_IN_QUERY_RATIO_MAX] = responseInQuery.GetMax();
    view[FI_TITLE_IN_QUERY_RATIO_MEAN] = responseInQuery.GetMean();
    view[FI_TITLE_IN_QUERY_RATIO_MIN] = responseInQuery.GetMin();

    const auto& prefix = titleResult.GetPrefix();
    view[FI_QUERY_IN_PREFIX_RATIO_MAX] = prefix.GetMax();
    view[FI_QUERY_IN_PREFIX_RATIO_MEAN] = prefix.GetMean();
    view[FI_QUERY_IN_PREFIX_RATIO_MIN] = prefix.GetMin();

    const auto& doublePrefix = titleResult.GetDoublePrefix();
    view[FI_QUERY_IN_DOUBLE_PREFIX_RATIO_MAX] = doublePrefix.GetMax();
    view[FI_QUERY_IN_DOUBLE_PREFIX_RATIO_MEAN] = doublePrefix.GetMean();
    view[FI_QUERY_IN_DOUBLE_PREFIX_RATIO_MIN] = doublePrefix.GetMin();

    const auto& headlineQueryInResponse = headlineResult.GetQueryInResponse();
    view[FI_QUERY_IN_HEADLINE_RATIO_MAX] = headlineQueryInResponse.GetMax();
    view[FI_QUERY_IN_HEADLINE_RATIO_MEAN] = headlineQueryInResponse.GetMean();
    view[FI_QUERY_IN_HEADLINE_RATIO_MIN] = headlineQueryInResponse.GetMin();

    const auto& headlineResponseInQuery = headlineResult.GetResponseInQuery();
    view[FI_HEADLINE_IN_QUERY_RATIO_MAX] = headlineResponseInQuery.GetMax();
    view[FI_HEADLINE_IN_QUERY_RATIO_MEAN] = headlineResponseInQuery.GetMean();
    view[FI_HEADLINE_IN_QUERY_RATIO_MIN] = headlineResponseInQuery.GetMin();

    const auto& headlinePrefix = headlineResult.GetPrefix();
    view[FI_QUERY_IN_HEADLINE_PREFIX_RATIO_MAX] = headlinePrefix.GetMax();
    view[FI_QUERY_IN_HEADLINE_PREFIX_RATIO_MEAN] = headlinePrefix.GetMean();
    view[FI_QUERY_IN_HEADLINE_PREFIX_RATIO_MIN] = headlinePrefix.GetMin();

    const auto& headlineDoublePrefix = headlineResult.GetDoublePrefix();
    view[FI_QUERY_IN_DOUBLE_HEADLINE_PREFIX_RATIO_MAX] = headlineDoublePrefix.GetMax();
    view[FI_QUERY_IN_DOUBLE_HEADLINE_PREFIX_RATIO_MEAN] = headlineDoublePrefix.GetMean();
    view[FI_QUERY_IN_DOUBLE_HEADLINE_PREFIX_RATIO_MIN] = headlineDoublePrefix.GetMin();
}

void FillSearchScenarioFactors(const TStringBuf utterance, const NSc::TArray& reportGrouping, const TFactorView view) {
    using namespace NAliceSearchScenario;
    using namespace NResponseSimilarity;

    for (const NSc::TValue& grouping : reportGrouping) {
        if (grouping["Attr"].GetString() != TStringBuf("d")) { // web grouping
            continue;
        }

        ui32 position = 0;
        THostsPositions hostsPositions;
        TVector<TSimilarity> titleSimilarities;
        TVector<TSimilarity> headlineSimilarities;
        for (const NSc::TValue& group : grouping["Group"].GetArray()) {
            ++position;
            for (const NSc::TValue& doc : group["Document"].GetArray()) {
                ProcessWebResult(doc, position, utterance, hostsPositions, titleSimilarities, headlineSimilarities);
                ProcessWizardResult(doc, CalcDCG(position), view);
            }
        }

        view[FI_MUSIC_HOSTS_COUNT] = hostsPositions.Music.size();
        view[FI_VIDEO_HOSTS_COUNT] = hostsPositions.Video.size();

        view[FI_MUSIC_HOSTS_DCG_AT_1] = CalcDCGAt(hostsPositions.Music, 1);
        view[FI_VIDEO_HOSTS_DCG_AT_1] = CalcDCGAt(hostsPositions.Video, 1);
        view[FI_MUSIC_HOSTS_DCG_AT_5] = CalcDCGAt(hostsPositions.Music, 5);
        view[FI_VIDEO_HOSTS_DCG_AT_5] = CalcDCGAt(hostsPositions.Video, 5);
        view[FI_MUSIC_HOSTS_DCG] = CalcDCGAt(hostsPositions.Music, hostsPositions.Music.size());
        view[FI_VIDEO_HOSTS_DCG] = CalcDCGAt(hostsPositions.Video, hostsPositions.Video.size());

        FillHostDCG(hostsPositions, view);

        const TSimilarity titleResult = AggregateSimilarity(titleSimilarities);
        const TSimilarity headlineResult = AggregateSimilarity(headlineSimilarities);

        const auto& queryInResponse = titleResult.GetQueryInResponse();
        view[FI_QUERY_IN_TITLE_RATIO_MAX] = queryInResponse.GetMax();
        view[FI_QUERY_IN_TITLE_RATIO_MEAN] = queryInResponse.GetMean();
        view[FI_QUERY_IN_TITLE_RATIO_MIN] = queryInResponse.GetMin();

        const auto& responseInQuery = titleResult.GetResponseInQuery();
        view[FI_TITLE_IN_QUERY_RATIO_MAX] = responseInQuery.GetMax();
        view[FI_TITLE_IN_QUERY_RATIO_MEAN] = responseInQuery.GetMean();
        view[FI_TITLE_IN_QUERY_RATIO_MIN] = responseInQuery.GetMin();

        const auto& prefix = titleResult.GetPrefix();
        view[FI_QUERY_IN_PREFIX_RATIO_MAX] = prefix.GetMax();
        view[FI_QUERY_IN_PREFIX_RATIO_MEAN] = prefix.GetMean();
        view[FI_QUERY_IN_PREFIX_RATIO_MIN] = prefix.GetMin();

        const auto& doublePrefix = titleResult.GetDoublePrefix();
        view[FI_QUERY_IN_DOUBLE_PREFIX_RATIO_MAX] = doublePrefix.GetMax();
        view[FI_QUERY_IN_DOUBLE_PREFIX_RATIO_MEAN] = doublePrefix.GetMean();
        view[FI_QUERY_IN_DOUBLE_PREFIX_RATIO_MIN] = doublePrefix.GetMin();

        const auto& headlineQueryInResponse = headlineResult.GetQueryInResponse();
        view[FI_QUERY_IN_HEADLINE_RATIO_MAX] = headlineQueryInResponse.GetMax();
        view[FI_QUERY_IN_HEADLINE_RATIO_MEAN] = headlineQueryInResponse.GetMean();
        view[FI_QUERY_IN_HEADLINE_RATIO_MIN] = headlineQueryInResponse.GetMin();

        const auto& headlineResponseInQuery = headlineResult.GetResponseInQuery();
        view[FI_HEADLINE_IN_QUERY_RATIO_MAX] = headlineResponseInQuery.GetMax();
        view[FI_HEADLINE_IN_QUERY_RATIO_MEAN] = headlineResponseInQuery.GetMean();
        view[FI_HEADLINE_IN_QUERY_RATIO_MIN] = headlineResponseInQuery.GetMin();

        const auto& headlinePrefix = headlineResult.GetPrefix();
        view[FI_QUERY_IN_HEADLINE_PREFIX_RATIO_MAX] = headlinePrefix.GetMax();
        view[FI_QUERY_IN_HEADLINE_PREFIX_RATIO_MEAN] = headlinePrefix.GetMean();
        view[FI_QUERY_IN_HEADLINE_PREFIX_RATIO_MIN] = headlinePrefix.GetMin();

        const auto& headlineDoublePrefix = headlineResult.GetDoublePrefix();
        view[FI_QUERY_IN_DOUBLE_HEADLINE_PREFIX_RATIO_MAX] = headlineDoublePrefix.GetMax();
        view[FI_QUERY_IN_DOUBLE_HEADLINE_PREFIX_RATIO_MEAN] = headlineDoublePrefix.GetMean();
        view[FI_QUERY_IN_DOUBLE_HEADLINE_PREFIX_RATIO_MIN] = headlineDoublePrefix.GetMin();

        break;
    }
}

} // namespace

void FillQueryFactors(const TWizardResponse& response, TFactorStorage& storage) {
    using namespace NAliceBegemotQueryFactors;

    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_BEGEMOT_QUERY_FACTORS);

    FillFstFactor(response.GetProtoResponse().GetFstNum().GetEntities(), "NUM", FI_HAS_NUM, view);
    FillFstFactor(response.GetProtoResponse().GetFstAlbum().GetEntities(), "ALBUM", FI_HAS_ALBUM, view);
    FillFstFactor(response.GetProtoResponse().GetFstSoft().GetEntities(), "SOFT", FI_HAS_SOFT, view);

    FillCustomEntityFactor(response, "video_action", FI_HAS_VIDEO_ACTION, view);
    FillCustomEntityFactor(response, "tv_genre", FI_HAS_TV_GENRE, view);
    FillCustomEntityFactor(response, "market_vendor", FI_HAS_MARKET_VENDOR, view);
    FillCustomEntityFactor(response, "tv_channel_suggest", FI_HAS_TV_CHANNEL_SUGGEST, view);
    FillCustomEntityFactor(response, "rewind_type", FI_HAS_REWIND_TYPE, view);
    FillCustomEntityFactor(response, "video_provider", FI_HAS_VIDEO_PROVIDER, view);
    FillCustomEntityFactor(response, "language", FI_HAS_LANGUAGE, view);
    FillCustomEntityFactor(response, "news_topic", FI_HAS_NEWS_TOPIC, view);
    FillCustomEntityFactor(response, "route_action_type", FI_HAS_ROUTE_ACTION_TYPE, view);
    FillCustomEntityFactor(response, "player_type", FI_HAS_PLAYER_TYPE, view);
    FillCustomEntityFactor(response, "video_film_genre", FI_HAS_VIDEO_FILM_GENRE, view);
    FillCustomEntityFactor(response, "ambient_sound", FI_HAS_AMBIENT_SOUND, view);

    FillEntityFinderFactor(response, "film", FI_HAS_ENTITY_FILM, view);
    FillEntityFinderFactor(response, "fb:film.film_subject", FI_HAS_ENTITY_FILM_SUBJECT, view);
    FillEntityFinderFactor(response, "fb:award.award_winning_work", FI_HAS_ENTITY_AWARD_WINNING_WORK, view);
    FillEntityFinderFactor(response, "fb:music.artist", FI_HAS_ENTITY_MUSIC_ARTIST, view);

    FillFrameHasTrustedSlotValueFactor(response, NAlice::NMusic::MUSIC_PLAY, MUSIC_PLAY_TRUSTED_SLOT_VALUES, MUSIC_PLAY_IGNORED_SLOT_VALUES, FI_MUSIC_PLAY_TRUSTED_SLOT_VALUE, view);

    FillGcClassifierQueryFactor(response, FI_GC_DSSM_CLASSIFIER, view);
}

void FillScenarioQueryFactors(const TWizardResponse& response, TFactorStorage& storage) {
    using namespace NAliceBegemotQueryFactors;

    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_BEGEMOT_QUERY_FACTORS);

    static const TVector<TBegemotScenarioFactors> BEGEMOT_SCENARIO_FACTORS = {{
        {NMusic::MUSIC_PLAY, FI_TOLOKA_MUSIC_WORD_LSTM, FI_SCENARIOS_MUSIC_WORD_LSTM, FI_FACTOR_COUNT},
        {NMusic::MUSIC_FAIRY_TALE, FI_TOLOKA_FAIRY_TALE_WORD_LSTM, FI_SCENARIOS_FAIRY_TALE_WORD_LSTM, FI_FAIRY_TALE_IS_RECOGNIZABLE},
        {NMusic::MUSIC_AMBIENT_SOUND, FI_TOLOKA_AMBIENT_SOUND_WORD_LSTM, FI_SCENARIOS_AMBIENT_SOUND_WORD_LSTM, FI_AMBIENT_SOUND_IS_RECOGNIZABLE},
        {NMusic::MUSIC_PODCAST, FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_MUSIC_PODCAST_IS_RECOGNIZABLE},
        {NMusic::RADIO_PLAY, FI_TOLOKA_RADIO_WORD_LSTM, FI_SCENARIOS_RADIO_WORD_LSTM, FI_FACTOR_COUNT},
        {NMusic::RADIO_PLAY_POST, FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_RADIO_IS_RECOGNIZABLE},
        {NVideoCommon::SEARCH_VIDEO, FI_TOLOKA_VIDEO_WORD_LSTM, FI_SCENARIOS_VIDEO_WORD_LSTM, FI_FACTOR_COUNT},
        {NVideoCommon::QUASAR_GOTO_VIDEO_SCREEN, FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GOTO_VIDEO_SCREEN_IS_RECOGNIZABLE},
        {NVideoCommon::QUASAR_PAYMENT_CONFIRMED, FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PAYMENT_CONFIRMED_IS_RECOGNIZABLE},
        {NVideoCommon::QUASAR_OPEN_CURRENT_VIDEO, FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_OPEN_CURRENT_VIDEO_IS_RECOGNIZABLE},
        {NSearch::LSTM_NAME, FI_TOLOKA_SEARCH_WORD_LSTM, FI_SCENARIOS_SEARCH_WORD_LSTM, FI_FACTOR_COUNT},
        {"personal_assistant.scenarios.alarm_general", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_ALARM_GENERAL_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.alarm_set_sound", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_ALARM_SET_SOUND_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.alarm_set_with_sound", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_ALARM_SET_WITH_SOUND_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.translate.translation", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_TRANSLATION_IS_RECOGNIZABLE},
        {"alice.market.how_much", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_HOW_MUCH_IS_RECOGNIZABLE},
        {TV_STREAM_FORM, FI_TOLOKA_TV_STREAM_WORD_LSTM, FI_SCENARIOS_TV_STREAM_WORD_LSTM, FI_TV_STREAM_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.player.rewind", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PLAYER_REWIND_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.player.shuffle", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PLAYER_SHUFFLE_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.player.replay", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PLAYER_REPLAY_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.get_news", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GET_NEWS_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.get_free_news", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GET_FREE_NEWS_IS_RECOGNIZABLE},
        {CREATE_REMINDER_FORM, FI_TOLOKA_CREATE_REMINDER_WORD_LSTM, FI_SCENARIOS_CREATE_REMINDER_WORD_LSTM, FI_CREATE_REMINDER_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.get_weather", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GET_WEATHER_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.select_video_by_number", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_SELECT_VIDEO_BY_NUM_IS_RECOGNIZABLE},
        {"alice.music.music_key_words", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_MUSIC_KEY_WORDS_DETECTED},
        {NMusic::MUSIC_SING_SONG, FI_TOLOKA_SING_SONG_WORD_LSTM, FI_SCENARIOS_SING_SONG_WORD_LSTM, FI_SING_SONG_IS_RECOGNIZABLE},
        {NSearch::FIND_POI_FORM, FI_TOLOKA_FIND_POI_WORD_LSTM, FI_SCENARIOS_FIND_POI_WORD_LSTM, FI_FACTOR_COUNT},
        {NSearch::OPEN_SITE_OR_APP_FORM, FI_TOLOKA_OPEN_SITE_OR_APP_WORD_LSTM, FI_SCENARIOS_OPEN_SITE_OR_APP_WORD_LSTM, FI_FACTOR_COUNT},
        {NSearch::HOW_MUCH_FORM, FI_TOLOKA_HOW_MUCH_WORD_LSTM, FI_SCENARIOS_HOW_MUCH_WORD_LSTM, FI_FACTOR_COUNT},
        {NSearch::TRANSLATE_FORM, FI_TOLOKA_TRANSLATE_WORD_LSTM, FI_SCENARIOS_TRANSLATE_WORD_LSTM, FI_FACTOR_COUNT},
        {NSearch::TV_BROADCAST_FORM, FI_TOLOKA_TV_BROADCAST_WORD_LSTM, FI_SCENARIOS_TV_BROADCAST_WORD_LSTM, FI_FACTOR_COUNT},
        {NSearch::CONVERT_FORM, FI_TOLOKA_CONVERT_WORD_LSTM, FI_SCENARIOS_CONVERT_WORD_LSTM, FI_FACTOR_COUNT},
        {NAlice::NMusic::PLAYER_DISLIKE, FI_FACTOR_COUNT, FI_SCENARIOS_PLAYER_DISLIKE_WORD_LSTM, FI_FACTOR_COUNT},
        {"personal_assistant.scenarios.player.dislike", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PLAYER_DISLIKE_IS_RECOGNIZABLE},
        {RECITE_A_POEM_FORM, FI_TOLOKA_RECITE_A_POEM_WORD_LSTM, FI_SCENARIOS_RECITE_A_POEM_WORD_LSTM, FI_FACTOR_COUNT},
        {"alice.microintents", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_MICROINTENTS_IS_RECOGNIZABLE},
        {"alice.movie_suggest.decline", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_MOOVIE_SUGGEST_DECLINE_IS_RECOGNIZABLE},
        {"alice.music.continue", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_MUSIC_CONTINUE_IS_RECOGNIZABLE},
        {"alice.music_discuss", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_MUSIC_DISCUSS_IS_RECOGNIZABLE},
        {"alice.quasar.video_play_text", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_QUASAR_VIDEO_PLAY_TEXT_IS_RECOGNIZABLE},
        {"alice.scenarios.get_weather_pressure__ellipsis", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GET_WEATHER_PRESSURE_ELLIPSIS_IS_RECOGNIZABLE},
        {"alice.scenarios.get_weather_temperature", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GET_WEATHER_TEMPERATURE_IS_RECOGNIZABLE},
        {"alice.scenarios.get_weather_temperature__ellipsis", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GET_WEATHER_TEMPERATURE_ELLIPSIS_IS_RECOGNIZABLE},
        {"alice.scenarios.get_weather_wind__ellipsis", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GET_WEATHER_WIND_ELLIPSIS_IS_RECOGNIZABLE},
        {"alice.search.related_agree", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_SEARCH_RELATED_AGREE_IS_RECOGNIZABLE},
        {"alice.time_capsule.interrupt_confirm", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_TIME_CAPSULE_INTERRUPT_CONFIRM_IS_RECOGNIZABLE},
        {"alice.video_rater.quit", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_VIDEO_RATER_QUIT_IS_RECOGNIZABLE},
        {"alice.video_rater.rate", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_VIDEO_RATER_RATE_IS_RECOGNIZABLE},
        {"alice.wiz_detection.shinyserp_unethical", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_WIZ_DETECTION_SHINYSERP_UNETHICAL_IS_RECOGNIZABLE},
        {"alice.zen_context_search", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_ZEN_CONTEXT_SEARCH_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.alarm_ask_time", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_ALARM_ASK_TIME_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.alarm_cancel__ellipsis", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_ALARM_CANCEL_ELLIPSIS_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.alarm_fallback", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_ALARM_FALLBACK_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.alarm_stop_playing", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_ALARM_STOP_PLAYING_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.fast_command.fast_continue", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_FAST_COMMAND_FAST_CONTINUE_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.fast_command.fast_next_track", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_FAST_COMMAND_FAST_NEXT_TRACK_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.get_news__previous", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_GET_NEWS_PREVIOUS_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.get_weather__ellipsis", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_GET_WEATHER_ELLIPSIS_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.morning_show_good_morning", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_MORNING_SHOW_GOOD_MORNING_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.player.continue", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_PLAYER_CONTINUE_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.player.like", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_PLAYER_LIKE_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.player.next_track", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_PLAYER_NEXT_TRACK_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.player.open_or_continue", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_PLAYER_OPEN_OR_CONTINUE_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.player.pause", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_PLAYER_PAUSE_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.player.repeat", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_PLAYER_REPEAT_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.sound.quiter_in_context", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_SOUND_QUITER_IN_CONTEXT_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.sound.louder_in_context", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_SOUND_LOUDER_IN_CONTEXT_IS_RECOGNIZABLE},
        {"personal_assistant.scenarios.sound.set_level_in_context", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_PA_SOUND_SET_LEVEL_IN_CONTEXT_IS_RECOGNIZABLE},
        {"alice.banned_direct", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_BANNED_DIRECT_IS_RECOGNIZABLE},
        {"alice.external_skill.flash_briefing.next", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_EXTERNAL_SKILL_FLASH_BRIEFING_NEXT_IS_RECOGNIZABLE},
        {"alice.fixlist.gc_request_banlist", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_FIXLIST_GC_REQUEST_BANLIST_IS_RECOGNIZABLE},
        {"alice.game_suggest.confirm", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GAME_SUGGEST_CONFIRM_IS_RECOGNIZABLE},
        {"alice.general_conversation.force_exit", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GENERAL_CONVERSATION_FORCE_EXIT_IS_RECOGNIZABLE},
        {"alice.generative_tale.stop", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GENERATIVE_TALE_STOP_IS_RECOGNIZABLE},
        {"alice.goods.best_prices_reask.tagger", FI_FACTOR_COUNT, FI_FACTOR_COUNT, FI_GOODS_BEST_PRICES_REASK_TAGGER_IS_RECOGNIZABLE},
    }};
    for (const TBegemotScenarioFactors& scenarioFactors : BEGEMOT_SCENARIO_FACTORS) {
        FillBinaryClassifierQueryFactor(response.GetProtoResponse().GetAliceTolokaWordLstm().GetProbabilities(),
                                        scenarioFactors.ScenarioName, scenarioFactors.TolokaLSTMFactorIndex, view);
        FillBinaryClassifierQueryFactor(response.GetProtoResponse().GetAliceScenariosWordLstm().GetProbabilities(),
                                        scenarioFactors.ScenarioName, scenarioFactors.ScenariosLSTMFactorIndex, view);

        FillGranetQueryFactor(response, scenarioFactors, view);
    }

    FillBinaryClassifierQueryFactor(response.GetProtoResponse().GetAliceBinaryIntentClassifier().GetProbabilities(),
                                    "alice.ontofacts", FI_FACTS_BINARY_CLASSIFIER, view);
}

void FillSearchFactors(const TStringBuf utterance, const TSearchResponse& response, TFactorStorage& storage) {
    const TFactorView bgView = storage.CreateViewFor(NFactorSlices::EFactorSlice::BEGEMOT_QUERY_FACTORS);
    FillSearchBegemotFactors(response.BgFactors(), bgView);

    const TFactorView blenderView = storage.CreateViewFor(NFactorSlices::EFactorSlice::BLENDER_PRODUCTION);
    FillSearchBlenderFactors(response.StaticBlenderFactors(), blenderView);

    // TODO(olegator@): should be moved to search scenario
    const TFactorView searchView = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_SEARCH_SCENARIO);

    if (response.Docs().GetDocs().empty()) {
        FillSearchScenarioFactors(utterance, response.GetReportGrouping(), searchView);
    } else {
        FillWebSearchScenarioFactors(utterance, response.Docs(), searchView);
    }
}

void FillScenarioFactors(const NMegamind::TScenarioFeatures& features, TFactorStorage& storage) {
    switch (features.GetFeaturesCase()) {
        case NMegamind::TScenarioFeatures::kGCFeatures: {
            const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_GC_SCENARIO);
            NAliceGCScenario::FillGCScenarioFactors(features.GetGCFeatures(), view);
            break;
        }
        case NMegamind::TScenarioFeatures::kMusicFeatures: {
            const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_MUSIC_SCENARIO);
            NAliceMusicScenario::FillMusicScenarioFactors(features.GetMusicFeatures(), view);
            break;
        }
        case NMegamind::TScenarioFeatures::kVideoFeatures: {
            const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_VIDEO_SCENARIO);
            NAliceVideoScenario::FillVideoScenarioFactors(features.GetVideoFeatures(), view);
            break;
        }
        case NMegamind::TScenarioFeatures::kVinsFeatures:
            break;
        case NMegamind::TScenarioFeatures::kSearchFeatures: {
            const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_SEARCH_SCENARIO);
            NAliceSearchScenario::FillSearchScenarioFactors(features.GetSearchFeatures(), view);

            const TFactorView directView = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DIRECT_SCENARIO);
            NAliceDirectScenario::FillDirectScenarioFactors(features.GetSearchFeatures(), directView);
            break;
        }
        case NMegamind::TScenarioFeatures::FEATURES_NOT_SET:
            break;
    }
}

void FillNluFactors(const TWizardResponse& response, TFactorStorage& storage) {
    if (!response.GetProtoResponse().HasAliceNluFeatures()) {
        return;
    }

    NNluFeatures::FillNluFactorsSlice(
        response.GetProtoResponse().GetAliceNluFeatures().GetFeatureContainer(),
        storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_BEGEMOT_NLU_FACTORS));
}

} // namespace NAlice
