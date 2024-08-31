#include "radio.h"
#include "directives.h"

#include <alice/bass/forms/automotive/fm_radio.h>
#include <alice/bass/forms/automotive/media_control.h>
#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/music/music.h>
#include <alice/bass/forms/music/providers.h>
#include <alice/bass/forms/player/player.h>
#include <alice/bass/forms/search/search.h>

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/radio/fmdb.h>
#include <alice/bass/libs/radio/recommender.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/json/json.h>
#include <alice/library/music/defs.h>

#include <alice/protos/data/fm_radio_info.pb.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <util/string/split.h>
#include <util/random/shuffle.h>

#include <algorithm>
#include <chrono>

namespace NBASS {

namespace {
const TStringBuf CHILD_RADIO_ID = "detskoe";
const TStringBuf RADIO_PLAY_FORM_NAME = "personal_assistant.scenarios.radio_play";
const TStringBuf RADIO_PLAY_FORM_NAME_ELLIPSIS = "personal_assistant.scenarios.radio_play__ellipsis";

const TStringBuf QUASAR_RADIO_PLAY_OBJECT_ACTION_STUB_INTENT = NRadio::QUASAR_RADIO_PLAY_OBJECT_ACTION_NAME;

enum class EMapping {
    NameToRadioId,
    RadioIdToName
};

THashMap<TString, TString> InitFeasibleFMStations(EMapping mapping) {
    NSc::TValue radioStations;
    if (!NSc::TValue::FromJson(radioStations, NResource::Find("radio_stations"))) {
        ythrow yexception() << "Can't parse resource radio_stations" << Endl;
    }

    THashMap<TString, TString> feasibleStations;
    for (const auto& station : radioStations.GetDict()) {
        const auto& name = station.first;
        const auto& id = station.second.GetString();
        switch (mapping) {
            case EMapping::NameToRadioId:
                feasibleStations.emplace(name, id);
                break;
            case EMapping::RadioIdToName:
                feasibleStations.emplace(id, name);
                break;
        }
    }
    return feasibleStations;
}

THashMap<TString, TString> NameToRadioId = InitFeasibleFMStations(EMapping::NameToRadioId);
THashMap<TString, TString> RadioIdToName = InitFeasibleFMStations(EMapping::RadioIdToName);

constexpr TStringBuf RADIO_SEARCH_RESULTS = "radio_search_results";

void AddCommonSuggests(TContext& ctx) {
    ctx.AddSearchSuggest();
    ctx.AddOnboardingSuggest();
}

void AddOpenRadioUriCommand(TContext& ctx, TStringBuf uri) {
    NSc::TValue payload;
    payload["uri"].SetString(uri);
    ctx.AddCommand<TRadioAppPlayDirective>(TStringBuf("open_uri"), std::move(payload));
}

void AddUnsupportedRegionBlocks(TStringBuf attentionType, TContext& ctx ) {
    ctx.AddCountedAttention(attentionType);
    ctx.CreateSlot(RADIO_SEARCH_RESULTS, RADIO_SEARCH_RESULTS)->Value["unsupported_user_region"].SetBool(true);
}

bool HasChildContentSettings(const TContext& ctx) {
    return ctx.GetContentRestrictionLevel() == EContentRestrictionLevel::Safe;
}

bool IsChildMode(const TContext& ctx) {
    return ctx.GetIsClassifiedAsChildRequest() || HasChildContentSettings(ctx);
}

NAlice::NData::TFmRadioInfo ConstructFmRadioInfoFromSlot(const TContext::TSlot& slotFmRadioInfo) {
    NAlice::NData::TFmRadioInfo fmRadioInfo;
    const NJson::TJsonValue jsonValue = NAlice::JsonFromString(slotFmRadioInfo.Value.ToJson()); // convert NSc::TValue to NJson::TJsonProto
    Y_ENSURE(NAlice::JsonToProto(jsonValue, fmRadioInfo, /* validateUtf8 = */ false, /* ignoreUnknownFields = */ true).ok());
    return fmRadioInfo;
}

} // namespace

namespace NRadio {

TMaybe<TString> GetRadioByFreq(TContext& ctx, const double radioFreq, const NAutomotive::TFMRadioDatabase& radioDatabase) {
    i32 regionId = GetRegionId(ctx, radioDatabase);
    if (regionId == 0) {
        ctx.AddAttention(ATTENTION_NO_REGION_FM_DB, {});
        return Nothing();
    }
    TString radioFreqTrunc = ToString(trunc(radioFreq * 100));
    if (radioDatabase.HasRadioByRegion(regionId, radioFreqTrunc)) {
        return radioDatabase.GetRadioByRegion(regionId, radioFreqTrunc);
    }
    return Nothing();
}

TResultValue FallbackToDefaultRadio(TContext& ctx) {
    if (!ctx.MetaClientInfo().IsYaAuto()) {
        NMusic::TSearchMusicHandler::SetAsResponse(ctx, false);
        return ctx.RunResponseFormHandler();
    }
    return NAutomotive::HandleMediaControlSource(ctx, "fm");
}

TResultValue FallbackToSearchScenario(TContext& ctx) {
    if (ctx.HasExpFlag("radio_disable_search_change_form")) {
        ctx.AddAttention(ATTENTION_NO_STATION);
        return TResultValue();
    }
    if (TContext::TPtr newContext = TSearchFormHandler::SetAsResponse(ctx, false)) {
        newContext->CreateSlot(TStringBuf("query"), TStringBuf("string"), true, NSc::TValue(ctx.Meta().Utterance()));
        return ctx.RunResponseFormHandler();
    } else {
        return TError(TError::EType::SYSTEM, "Cannot change form to search");
    }
}

TResultValue HandleYaRadio(TContext& ctx) {
    const TContext::TSlot* yaRadioSlot = ctx.GetSlot(YA_RADIO_SLOT_NAME);
    auto yaRadio = yaRadioSlot->Value.GetString();
    TString slot, value;
    StringSplitter(yaRadio).Split('/').CollectInto(&slot, &value);
    TIntrusivePtr<TContext> newContext = NMusic::TSearchMusicHandler::SetAsResponse(ctx, false);
    newContext->CreateSlot(slot, slot, true, NSc::TValue(value));
    return ctx.RunResponseFormHandler();
}

TString GenerateRadioUrl(TStringBuf radioName) {
    TStringBuilder url;
    url << TStringBuf("http://music.yandex.ru/fm/") << radioName;
    UrlEscape(url);
    return url;
}

class TRadioHelper {
public:
    TRadioHelper(TContext& ctx, NHttpFetcher::IMultiRequest::TRef multirequest = nullptr)
        : Ctx(ctx)
        , MultiRequest(multirequest)
    {}

    bool TryFetchRequest() {
        Request = CreateRequest();
        if (!PrepareRequest()) {
            return false;
        }
        RequestHandle = Request->Fetch();
        return true;
    }

    TResultValue WaitAndParseResponse(TVector<NSc::TValue>& results) {
        NHttpFetcher::TResponse::TRef response = RequestHandle->Wait();
        if (response->IsError()) {
            if (response->Code == 451) {// 451 is for 'user region is not supported'. It's not an 'error'
                AddUnsupportedRegionBlocks(ATTENTION_COUNTRY_IS_NOT_SUPPORTED, Ctx);
                return TResultValue();
            } else if (response->Code == 404) {
                Ctx.AddCountedAttention(ATTENTION_NO_FM_STATION);
                return TResultValue();
            } else {
                return TError(TError::EType::SYSTEM,
                              TStringBuilder() << TStringBuf("radio stream request error: ") << response->GetErrorText());
            }
        }

        NSc::TValue responseJson = NSc::TValue::FromJson(response->Data);
        if (responseJson.IsNull()) {
            TStringBuilder errText;
            errText << "Can not parse radio stream response" << Endl;
            return TError(TError::EType::SYSTEM, errText);
        }

        return ParseResponse(responseJson, results);
    }

    TResultValue FetchRadioStreams(TVector<NSc::TValue>& results) {
        if (TryFetchRequest()) {
            return WaitAndParseResponse(results);
        }
        return TError(TError::EType::SYSTEM, TStringBuf("Failed to create radio streams request"));
    }

    virtual ~TRadioHelper()
    {}

protected:
    virtual NHttpFetcher::TRequestPtr CreateRequest() = 0;

    virtual bool PrepareRequest() {
        Request->SetMethod("GET");
        Request->AddCgiParam("ip", Ctx.UserIP());
        Request->AddCgiParam("lat", ToString(Ctx.Meta().Location().Lat()));
        Request->AddCgiParam("lon", ToString(Ctx.Meta().Location().Lon()));
        return true;
    }

    virtual TResultValue ParseResponse(NSc::TValue& responseJson, TVector<NSc::TValue>& results) = 0;

protected:
    TContext& Ctx;

    NHttpFetcher::TRequestPtr Request;
    NHttpFetcher::THandle::TRef RequestHandle;

    NHttpFetcher::IMultiRequest::TRef MultiRequest;
};

class TRadioStreamByIdHelper : public TRadioHelper {
public:
    TRadioStreamByIdHelper(TStringBuf radioId, TContext& ctx, NHttpFetcher::IMultiRequest::TRef multirequest = nullptr)
        : TRadioHelper(ctx, multirequest)
        , RadioId(radioId)
    {}

private:
    NHttpFetcher::TRequestPtr CreateRequest() override {
        if (MultiRequest) {
            Ctx.GetSources().RadioStream(RadioId).AttachRequest(MultiRequest);
        }
        return Ctx.GetSources().RadioStream(RadioId).Request();
    }

    TResultValue ParseResponse(NSc::TValue& responseJson, TVector<NSc::TValue>& results) override {
        const auto& resultValue = responseJson.TrySelect("result");
        if (!resultValue.IsNull()) {
            results.emplace_back(resultValue);
        } else {
            Ctx.AddCountedAttention(ATTENTION_NO_FM_STATION);
        }

        return TResultValue();
    }

private:
    TString RadioId;
};

class TRadioAvailableStationsHelper : public TRadioHelper {
public:
    TRadioAvailableStationsHelper(TContext& ctx, NHttpFetcher::IMultiRequest::TRef multirequest = nullptr)
        : TRadioHelper(ctx, multirequest)
    {}

private:
    NHttpFetcher::TRequestPtr CreateRequest() override {
        if (MultiRequest) {
            return Ctx.GetSources().RadioStreamAvailableStations().AttachRequest(MultiRequest);
        }
        return Ctx.GetSources().RadioStreamAvailableStations().Request();
    }

    TResultValue ParseResponse(NSc::TValue& responseJson, TVector<NSc::TValue>& results) override {
        const auto& resultValue = responseJson.TrySelect("result");
        if (resultValue.IsNull() || resultValue.GetArray().empty()) {
            // no stations were found at all
            AddUnsupportedRegionBlocks(ATTENTION_NO_FM_STATIONS_IN_REGION, Ctx);
        }

        for (const auto& station : resultValue.GetArray()) {
            results.emplace_back(station);
        }
        return TResultValue();
    }
};

class TRadioRecommendationsHelper : public TRadioHelper {
public:
    TRadioRecommendationsHelper(TContext& ctx, NHttpFetcher::IMultiRequest::TRef multirequest = nullptr)
        : TRadioHelper(ctx, multirequest)
    {}

private:
    NHttpFetcher::TRequestPtr CreateRequest() override {
        if (MultiRequest) {
            return Ctx.GetSources().RadioStreamRecommendations().AttachRequest(MultiRequest);
        }
        return Ctx.GetSources().RadioStreamRecommendations().Request();
    }

    bool PrepareRequest() override {
        TRadioHelper::PrepareRequest();
        TString passportUid;
        if (TPersonalDataHelper(Ctx).GetUid(passportUid)) {
            Request->AddCgiParam("__uid", passportUid);
            return true;
        }
        return false;
    }

    TResultValue ParseResponse(NSc::TValue& responseJson, TVector<NSc::TValue>& results) override {
        const auto& resultValue = responseJson.TrySelect("result");
        if (!resultValue.TrySelect("pumpkin").GetBool()) {
            const auto& radios = resultValue.TrySelect("radios");
            for (const auto& station : radios.GetArray()) {
                results.emplace_back(station);
            }
        }
        return TResultValue();
    }
};

TResultValue FetchRadioStreamById(TContext& ctx, TStringBuf radioId, TVector<NSc::TValue>& resultList) {
    return TRadioStreamByIdHelper(radioId, ctx).FetchRadioStreams(resultList);
}

TResultValue FetchRadioAvailableStations(TContext& ctx, TVector<NSc::TValue>& resultList) {
    return TRadioAvailableStationsHelper(ctx).FetchRadioStreams(resultList);
}

TResultValue FetchRadioRecommendations(TContext& ctx, TVector<NSc::TValue>& resultList) {
    return TRadioRecommendationsHelper(ctx).FetchRadioStreams(resultList);
}

TResultValue FetchOrderedRadioList(TContext& ctx, TVector<NSc::TValue>& resultList) {
    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();

    TRadioAvailableStationsHelper availableStationsHelper(ctx, multiRequest);
    TRadioRecommendationsHelper recommendationsHelper(ctx, multiRequest);

    availableStationsHelper.TryFetchRequest();
    const bool recomendationsFetched = recommendationsHelper.TryFetchRequest();

    multiRequest->WaitAll();

    TVector<NSc::TValue> availableStationsResults;
    if (auto err = availableStationsHelper.WaitAndParseResponse(availableStationsResults)) {
        return err;
    }

    TVector<NSc::TValue> recommendationsResults;
    if (recomendationsFetched) {
        if (auto err = recommendationsHelper.WaitAndParseResponse(recommendationsResults)) {
            // recommendations is not mandatory, will just log errors
            LOG(ERR) << "Error fetching recommendations request: " << err->Msg << Endl;
        }
    }

    TSet<TString> availableStations;
    for (const auto& station : availableStationsResults) {
        availableStations.emplace(station.TrySelect("radioId").GetString());
    }

    // add all available stations from recommendations response
    TSet<TString> recommendedStations;
    for (const auto& station : recommendationsResults) {
        TStringBuf radioId = station.TrySelect("radioId").GetString();
        if (availableStations.contains(radioId)) {
            resultList.emplace_back(station);
            recommendedStations.emplace(radioId);
        }
    }

    // then add all remaining available stations in random order
    Shuffle(availableStationsResults.begin(), availableStationsResults.end(), ctx.GetRandGeneratorInitializedWithEpoch());
    for (const auto& station : availableStationsResults) {
        if (recommendedStations.count(station.TrySelect("radioId").GetString()) == 0) {
            resultList.emplace_back(station);
        }
    }

    return TResultValue();
}

TMaybe<NSc::TValue> GetRecommendedRadioStream(TContext& ctx, const TVector<NSc::TValue>& fetchedRadioList, const THashSet<TStringBuf>& desiredRadioIds = {}) {
    if (fetchedRadioList.size() == 0) {
        return Nothing();
    }

    TContext::TSlot* slotFmRadioInfo = ctx.GetSlot(NRadio::FM_RADIO_INFO_SLOT_NAME);
    if (!IsSlotEmpty(slotFmRadioInfo)) {
        // raw json -> TRadioData
        TVector<TRadioRecommender::TRadioData> radioDatas;
        for (const auto& radioJson : fetchedRadioList) {
            radioDatas.push_back(TRadioRecommender::TRadioData{
                .RadioId = TString{radioJson["radioId"].GetString()},
                .Title = TString{radioJson["title"].GetString()}
            });
        }

        // find right TRadioData
        const NAlice::NData::TFmRadioInfo fmRadioInfo = ConstructFmRadioInfoFromSlot(*slotFmRadioInfo);

        auto recommender = TRadioRecommender{ctx.GetRng(), radioDatas}.SetDesiredRadioIds(desiredRadioIds);
        if (!fmRadioInfo.GetTrackId().Empty()) {
            recommender.UseTrackIdWeightedModel(fmRadioInfo.GetTrackId());
        }
        if (!fmRadioInfo.GetContentMetatag().Empty()) {
            recommender.UseMetatagWeightedModel(fmRadioInfo.GetContentMetatag());
        }
        const TRadioRecommender::TRadioData& recommendedRadioData = recommender.Recommend();

        // TRadioData -> raw json
        return fetchedRadioList.at(std::distance(radioDatas.cbegin(), &recommendedRadioData));
    }

    const NSc::TValue* desiredRadioStream = FindIfPtr(fetchedRadioList.begin(), fetchedRadioList.end(), [&desiredRadioIds](auto& x) -> bool {
        return desiredRadioIds.contains(x["radioId"].GetString())
                || desiredRadioIds.contains(x["title"].GetString());
    });

    if (desiredRadioStream != nullptr) {
        LOG(INFO) << "Found desired radio from " << fetchedRadioList.size() << " available" << Endl;
        return *desiredRadioStream;
    }
    LOG(INFO) << "Didn't found desired radio from " << fetchedRadioList.size() << " available, choose the first" << Endl;
    return fetchedRadioList.front();
}

TMaybe<size_t> GetRadioStreamIndexByRadioId(TStringBuf radioId, const TVector<NSc::TValue>& fetchedRadioList) {
    if (fetchedRadioList.size() == 0) {
        return Nothing();
    }

    for (size_t i = 0; i < fetchedRadioList.size(); ++i) {
        if (fetchedRadioList[i]["radioId"].GetString() == radioId) {
            return i;
        }
    }
    return Nothing();
}

TMaybe<NSc::TValue> GetRadioStreamByRadioId(TStringBuf radioId, const TVector<NSc::TValue>& fetchedRadioList) {
    if (fetchedRadioList.size() == 0) {
        return Nothing();
    }

    Y_ASSERT(fetchedRadioList.size() == 1);
    const NSc::TValue& fetchedRadio = fetchedRadioList.front();
    return fetchedRadio.TrySelect("radioId").GetString() == radioId ?
        TMaybe<NSc::TValue>(fetchedRadio) : Nothing();
}

TMaybe<NSc::TValue> GetNextRadioStreamByRadioId(TStringBuf radioId, const TVector<NSc::TValue>& fetchedRadioList, bool forward) {
    TMaybe<size_t> radioStreamIndex;
    if (!(radioStreamIndex = GetRadioStreamIndexByRadioId(radioId, fetchedRadioList))) {
        return Nothing();
    }

    const auto& lastStation = fetchedRadioList.back();
    const size_t stationIndex = *radioStreamIndex.Get();
    if (forward) {
        return fetchedRadioList[(stationIndex + 1) % fetchedRadioList.size()];
    }
    return (stationIndex > 0) ? fetchedRadioList[stationIndex - 1] : lastStation;
}

NSc::TValue FillRadioSearchResultsData(const NSc::TValue& streamData, TStringBuf alarmId = {}) {
    TStringBuf foundRadioId = streamData.Get("radioId").GetString();
    NSc::TValue result;
    result["fm_radio_url"] = NRadio::GenerateRadioUrl(foundRadioId);
    result["stream_data"] = streamData;
    if (alarmId) {
        result["stream_data"]["alarm_id"].SetString(alarmId);
    }
    return result;
}

TContext::TSlot* CreateRadioSearchResultsSlot(TContext& ctx, const NSc::TValue& streamData, TStringBuf alarmId = {}) {
    return ctx.CreateSlot(
        RADIO_SEARCH_RESULTS,
        RADIO_SEARCH_RESULTS,
        true /* optional */,
        FillRadioSearchResultsData(streamData, alarmId)
    );
}

void AddRadioPlayCommands(TContext& ctx) {
    TContext::TSlot *results = ctx.GetSlot(RADIO_SEARCH_RESULTS, RADIO_SEARCH_RESULTS);
    Y_ASSERT(!IsSlotEmpty(results));

    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        ctx.AddCommand<TRadioSmartSpeakerPlayDirective>(TStringBuf("radio_play"), results->Value["stream_data"]);
    } else {
        AddOpenRadioUriCommand(ctx, results->Value["fm_radio_url"]);
        AddCommonSuggests(ctx);
    }
}

TResultValue LaunchPersonalRadio(TContext& ctx) {
    if (ctx.MetaClientInfo().IsSmartSpeaker() && !NPlayer::IsRadioPaused(ctx)) {
        return TResultValue();
    }

    // first of all try to launch best recommended station
    TVector<NSc::TValue> radioRecommendations;
    if (auto err = FetchRadioRecommendations(ctx, radioRecommendations)) {
        LOG(ERR) << "Error while fetching recommendations request: " << err->Msg << Endl;
    } else if (TMaybe<NSc::TValue> streamData = GetRecommendedRadioStream(ctx, radioRecommendations)) {
        CreateRadioSearchResultsSlot(ctx, *streamData);
        AddRadioPlayCommands(ctx);
        ctx.AddAttention(NRadio::ATTENTION_STATION_NOT_FOUND_LAUNCH_RECOMMENDED, {});

        return TResultValue();
    }

    if (!ctx.MetaClientInfo().IsSmartSpeaker()) {
        AddCommonSuggests(ctx);
        return TResultValue();
    }

    ctx.AddStopListeningBlock();

    TPersonalDataHelper::TUserInfo userInfo;
    if (!TPersonalDataHelper(ctx).GetUserInfo(userInfo) || !userInfo.GetHasYandexPlus()) {
        return TResultValue();
    }

    ctx.AddAttention(NRadio::ATTENTION_STATION_NOT_FOUND_LAUNCH_PERSONAL, {});
    return NMusic::TSearchMusicHandler::DoWithoutCallback(ctx);
}

} // namespace NRadio

THolder<NAutomotive::TFMRadioDatabase> TRadioFormHandler::RadioDatabase;

TResultValue TRadioFormHandler::SearchRadioStream(TContext& ctx, const TStringBuf desiredRadioId, NRadio::ESelectionMethod selectionMethod, TMaybe<NSc::TValue>& streamData) {
    const THashSet<TStringBuf> desiredRadioIds{desiredRadioId};
    return SearchRadioStream(ctx, desiredRadioIds, selectionMethod, streamData);
}

TResultValue TRadioFormHandler::SearchRadioStream(TContext& ctx, const THashSet<TStringBuf>& desiredRadioIds, NRadio::ESelectionMethod selectionMethod, TMaybe<NSc::TValue>& streamData) {
    TVector<NSc::TValue> radioList;
    switch (selectionMethod) {
        case NRadio::ESelectionMethod::Any:
            if (auto err = NRadio::FetchOrderedRadioList(ctx, radioList)) {
                return err;
            }
            streamData = NRadio::GetRecommendedRadioStream(ctx, radioList, desiredRadioIds);
            break;
        case NRadio::ESelectionMethod::Current: {
            Y_ENSURE(desiredRadioIds.size() == 1);
            const TStringBuf radioId = *desiredRadioIds.begin();
            if (auto err = NRadio::FetchRadioStreamById(ctx, radioId, radioList)) {
                return err;
            }
            streamData = NRadio::GetRadioStreamByRadioId(radioId, radioList);
            break;
        }
        case NRadio::ESelectionMethod::Next:
        case NRadio::ESelectionMethod::Previous: {
            Y_ENSURE(desiredRadioIds.size() == 1);
            const TStringBuf radioId = *desiredRadioIds.begin();
            if (auto err = NRadio::FetchRadioAvailableStations(ctx, radioList)) {
                return err;
            }
            streamData = NRadio::GetNextRadioStreamByRadioId(radioId, radioList, selectionMethod == NRadio::ESelectionMethod::Next /* forward */);
            break;
         }
    }
    return TResultValue();
}

TResultValue TRadioFormHandler::HandleRadioStream(TContext& ctx, const TStringBuf desiredRadioId, NRadio::ESelectionMethod selectionMethod,
                                                  const bool overrideRadioIdIfChild, TStringBuf alarmId) {
    const THashSet<TStringBuf> desiredRadioIds{desiredRadioId};
    return HandleRadioStream(ctx, desiredRadioIds, selectionMethod, overrideRadioIdIfChild, alarmId);
}

TResultValue TRadioFormHandler::HandleRadioStream(TContext& ctx, const THashSet<TStringBuf>& desiredRadioIds, NRadio::ESelectionMethod selectionMethod,
                                                  const bool overrideRadioIdIfChild, TStringBuf alarmId) {
    TMaybe<NSc::TValue> streamData;

    static const THashSet<TStringBuf> CHILD_RADIO_IDS{CHILD_RADIO_ID};
    const THashSet<TStringBuf>& radioIdsCheckChild =
        (IsChildMode(ctx) && overrideRadioIdIfChild)
        ? CHILD_RADIO_IDS
        : desiredRadioIds;
    const NRadio::ESelectionMethod selectionMethodCheckChild =
        IsChildMode(ctx)
        ? NRadio::ESelectionMethod::Current
        : selectionMethod;

    if (auto searchError = SearchRadioStream(ctx, radioIdsCheckChild, selectionMethodCheckChild, streamData)) {
        return searchError;
    }

    if (!streamData) {
        if (alarmId) {
            return TError(TError::EType::RADIOERROR, TStringBuf("no_stream_data_for_requested_alarm_radio"));
        }
        return NRadio::LaunchPersonalRadio(ctx);
    }

    NRadio::CreateRadioSearchResultsSlot(ctx, *streamData, alarmId);

    const bool isActive = streamData->TrySelect("active").GetBool(false);
    const bool isAvailable = streamData->TrySelect("available").GetBool(false);

    if (isActive && isAvailable) {
        NRadio::AddRadioPlayCommands(ctx);
        return TResultValue();
    } else {
        if (alarmId) {
            return TError(TError::EType::RADIOERROR, TStringBuf("requested_alarm_radio_is_not_active_or_not_available"));
        }

        if (!isActive && isAvailable) {
            ctx.AddCountedAttention(NRadio::ATTENTION_FM_STATION_IS_TEMPORARY_UNAVAILABLE);
        } else {
            ctx.AddCountedAttention(NRadio::ATTENTION_FM_STATION_IS_UNAVAILABLE);
        }

        if (!ctx.MetaClientInfo().IsSmartSpeaker()) {
            TStringBuf officialSite = streamData->TrySelect("officialSiteUrl").GetString();
            if (!officialSite.empty()) {
                ctx.AddAttention(NRadio::ATTENTION_OPEN_RADIOSTATION_WEBSITE);
                AddOpenRadioUriCommand(ctx, officialSite);
                AddCommonSuggests(ctx);
                return TResultValue();
            }
        }
    }

    return NRadio::LaunchPersonalRadio(ctx);
}

TMaybe<TString> TRadioFormHandler::GetRadioName(TContext& ctx, const bool isFmRadio, const TContext::TSlot* slotFmRadio, const TContext::TSlot* slotFmRadioFreq, const NAutomotive::TFMRadioDatabase& radioDatabase) {
    return isFmRadio
        ? slotFmRadio->Value.ForceString()
        : NRadio::GetRadioByFreq(ctx, slotFmRadioFreq->Value.ForceNumber(), radioDatabase);
}

TMaybe<TStringBuf> TRadioFormHandler::GetRadioId(const TMaybe<TString>& namedRadioStation) {
    if (!namedRadioStation) {
        return Nothing();
    }
    if (const auto* radioIdPtr = NameToRadioId.FindPtr(*namedRadioStation.Get())) {
        return *radioIdPtr;
    }
    return Nothing();
}

TResultValue TRadioFormHandler::LaunchRadio(TContext& ctx, const TMaybe<TStringBuf>& radioId, NRadio::ESelectionMethod selectionMethod) {
    if (radioId) {
        return HandleRadioStream(ctx, radioId.GetRef(), selectionMethod, false /* overrideRadioIdIfChild */);
    }
    ctx.AddCountedAttention(NRadio::ATTENTION_FM_STATION_IS_UNRECOGNIZED);
    return NRadio::LaunchPersonalRadio(ctx);
}


TStringBuf TRadioFormHandler::GetCurrentlyPlayingRadioId(const NSc::TValue* radioState) {
    return radioState->TrySelect("currently_playing/radioId").GetString();
}

// https://wiki.yandex-team.ru/automotive/serverdevelopment/alice4auto/radiointent/
TResultValue TRadioFormHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::RADIO);

    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        ctx.AddAttention(NRadio::ATTENTION_USE_LONG_INTRO);
    }

    if (const TResultValue authorizationError = NMusic::AddAuthorizationSuggest(ctx)) {
        return authorizationError;
    }

    TContext::TSlot* slotFmRadioInfo = ctx.GetSlot(NRadio::FM_RADIO_INFO_SLOT_NAME);
    if (!IsSlotEmpty(slotFmRadioInfo)) {
        const NAlice::NData::TFmRadioInfo fmRadioInfo = ConstructFmRadioInfoFromSlot(*slotFmRadioInfo);

        if (fmRadioInfo.GetSimpleNlu()) {
            ctx.AddAttention(NRadio::ATTENTION_SIMPLE_NLU);
        }

        THashSet<TStringBuf> desiredRadioIds;
        for (const auto& radioId : fmRadioInfo.GetFmRadioIds()) {
            desiredRadioIds.insert(radioId);
        }

        return HandleRadioStream(ctx, desiredRadioIds, NRadio::ESelectionMethod::Any, true /* overrideRadioIdIfChild */);
    }

    TContext::TSlot* slotFormIsCallback = ctx.GetSlot("this_form_is_callback");
    if (!IsSlotEmpty(slotFormIsCallback)) {
        ctx.DeleteSlot("callback_form");
        ctx.DeleteSlot("this_form_is_callback");
        return TResultValue();
    }

    if (!IsSlotEmpty(ctx.GetSlot(NRadio::YA_RADIO_SLOT_NAME))) {
        // launch Yandex.Radio station
        return NRadio::HandleYaRadio(ctx);
    }

    TContext::TSlot* slotFmRadio = ctx.GetSlot(NRadio::FM_RADIO_SLOT_NAME);
    TContext::TSlot* slotFmRadioFreq = ctx.GetSlot(NRadio::FM_RADIO_FREQ_SLOT_NAME);

    const bool isEmptyForm = IsSlotEmpty(slotFmRadio) && IsSlotEmpty(slotFmRadioFreq);
    const bool supportsRadio = ctx.ClientFeatures().SupportsFMRadio();

    if (isEmptyForm) {
        if (!supportsRadio) {
            return NRadio::FallbackToDefaultRadio(ctx);
        }
        if (ctx.MetaClientInfo().IsSmartSpeaker() && !ctx.Meta().DeviceState().Radio().IsNull()) {
            const NSc::TValue* radioState = ctx.Meta().DeviceState().Radio().GetRawValue();
            return HandleRadioStream(ctx, GetCurrentlyPlayingRadioId(radioState), NRadio::ESelectionMethod::Current, true /* overrideRadioIdIfChild */);
        }
        const static THashSet<TStringBuf> emptyDesiredRadioIdsSet;
        return HandleRadioStream(ctx, emptyDesiredRadioIdsSet, NRadio::ESelectionMethod::Any, true /* overrideRadioIdIfChild */);
    }

    if (ctx.MetaClientInfo().IsYaAuto() && ctx.HasExpFlag("auto_fm_radio_off")) {
        ctx.AddAttention(NRadio::ATTENTION_NO_STATION, {});
        return TResultValue();
    }

    const bool isFmRadio = !IsSlotEmpty(slotFmRadio) && slotFmRadio->Type == NRadio::TYPE_KNOWN_RADIO;
    const bool isFmRadioFreq = !IsSlotEmpty(slotFmRadioFreq) && slotFmRadioFreq->Type == NRadio::TYPE_KNOWN_RADIO_FREQ;

    if (isFmRadio || isFmRadioFreq) {
        if (ctx.MetaClientInfo().IsYaAuto()) {
            return NAutomotive::HandleFMRadio(ctx, *RadioDatabase);
        }
        if (!supportsRadio) {
            return NRadio::FallbackToSearchScenario(ctx);
        }

        const TMaybe<TString> radioName = GetRadioName(ctx, isFmRadio, slotFmRadio, slotFmRadioFreq, *RadioDatabase);
        const TMaybe<TStringBuf> radioId = GetRadioId(radioName);
        if (HasChildContentSettings(ctx) && radioId && radioId != CHILD_RADIO_ID) {
            ctx.AddAttention(NRadio::ATTENTION_RESTRICTED_BY_CHILD_CONTENT_SETTINGS, {});
            return TResultValue();
        } else if (IsChildMode(ctx) && (!radioId || radioId == CHILD_RADIO_ID)) {
            return HandleRadioStream(ctx, CHILD_RADIO_ID, NRadio::ESelectionMethod::Current, true /* overrideRadioIdIfChild */);
        }
        // Launch known FM radio station as search URL
        return LaunchRadio(ctx, radioId, NRadio::ESelectionMethod::Current);
    }

    Y_ASSERT(!IsSlotEmpty(slotFmRadio) || !IsSlotEmpty(slotFmRadioFreq));

    if (ctx.MetaClientInfo().IsYaAuto()) {
        ctx.AddAttention(NRadio::ATTENTION_NO_STATION, {});
        return TResultValue();
    }

    if (!IsSlotEmpty(slotFmRadio)) {
        auto searchText = slotFmRadio->Value.GetString();
        TIntrusivePtr<TContext> newContext = NMusic::TSearchMusicHandler::SetAsResponse(ctx, false);
        newContext->CreateSlot("search_text", "string", true, searchText, searchText);
        newContext->AddCountedAttention(NRadio::ATTENTION_FM_STATION_IS_UNRECOGNIZED);
        return ctx.RunResponseFormHandler();
    }
    // assert !IsSlotEmpty(slotFmRadioFreq)
    if (!supportsRadio) {
        return NRadio::FallbackToSearchScenario(ctx);
    }
    return NRadio::LaunchPersonalRadio(ctx);
}

void TRadioFormHandler::Register(THandlersMap* handlers) {
    RadioDatabase = MakeHolder<NAutomotive::TFMRadioDatabase>();

    handlers->emplace(RADIO_PLAY_FORM_NAME, []() { return MakeHolder<TRadioFormHandler>(); });
    handlers->emplace(RADIO_PLAY_FORM_NAME_ELLIPSIS, []() { return MakeHolder<TRadioFormHandler>(); });
}

TContext::TPtr TRadioFormHandler::SetAsResponse(TContext& ctx, bool callbackSlot) {
    TContext::TPtr newCtx = ctx.SetResponseForm(RADIO_PLAY_FORM_NAME, callbackSlot);
    Y_ENSURE(newCtx);

    newCtx->GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::RADIO);

    return newCtx;
}


void TRadioPlayObjectActionHandler::Register(THandlersMap* handlers) {
    handlers->RegisterActionHandler(NRadio::QUASAR_RADIO_PLAY_OBJECT_ACTION_NAME, []() { return MakeHolder<TRadioPlayObjectActionHandler>(); });
}

TResultValue TRadioPlayObjectActionHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    auto& analyticsInfoBuilder = ctx.GetAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::RADIO);
    analyticsInfoBuilder.SetIntentName(TString{QUASAR_RADIO_PLAY_OBJECT_ACTION_STUB_INTENT});

    const NSc::TValue& actionData = ctx.InputAction().Get()->Data;
    const TStringBuf alarmId = actionData["alarm_id"].GetString();
    if (alarmId) {
        ctx.AddSilentResponse();
    }

    return TRadioFormHandler::HandleRadioStream(ctx, actionData["object"]["radioId"], NRadio::ESelectionMethod::Current, true /* overrideRadioIdIfChild */, alarmId);
}

} // namespace NBASS
