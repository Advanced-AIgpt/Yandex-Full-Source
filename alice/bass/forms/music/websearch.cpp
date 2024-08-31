#include "websearch.h"

#include "answers.h"
#include "catalog.h"
#include "common_headers.h"
#include "fairy_tales.h"
#include "providers.h"

#include <alice/bass/forms/fairy_tales.h>
#include <alice/bass/forms/search/search.h>
#include <alice/bass/forms/search/serp.h>
#include <alice/bass/forms/vins.h>

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/library/analytics/common/names.h>
#include <alice/library/experiments/flags.h>

#include <kernel/alice/music_scenario/web_url_canonizer/lib/web_url_canonizer.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/neh/http_common.h>

#include <util/digest/multi.h>
#include <util/string/builder.h>
#include <util/string/split.h>

using namespace NAlice::NMusic;

namespace NBASS::NMusic {

namespace {

static const int MAX_WEB_RESULTS_COUNT = 5;

constexpr TStringBuf FLAG_ANALYTICS_MUSIC_WEB_RESPONSES = "analytics.music.add_web_responses";
constexpr TStringBuf FLAG_FILTER_STUPID_ANSWERS = "web_music_filter_stupid_answers";

// When this flag is active, we disable some of the logs to avoid canonization issues
constexpr TStringBuf FLAG_FOR_TESTS = "music_for_tests";

constexpr std::array DEFAULT_FAIRY_TALE_GENRES = {
    TStringBuf("fairytales"),
    TStringBuf("forchildren"),
};

TVector<TStringBuf> GetFairyTaleGenres(const TContext& ctx) {
    const auto genreStr = ctx.GetValueFromExpPrefix("fairy_tale_genre_filter=");
    if (genreStr.Defined()) {
        return StringSplitter(genreStr.GetRef()).Split(',');
    }
    return {DEFAULT_FAIRY_TALE_GENRES.begin(), DEFAULT_FAIRY_TALE_GENRES.end()};
}

bool CheckMusicAnswerField(const NSc::TValue& answer, const TStringBuf field, const TVector<TStringBuf>& expectedValues = {}) {
    const TVector<NSc::TValue> values = {
        answer[field],
        answer["album"][field],
        answer["firstTrack"][field],
        answer["firstTrack"]["album"][field],
    };
    if (expectedValues.empty()) {
        return AnyOf(values, [](const NSc::TValue& value) { return value.GetBool(); });
    }
    return AnyOf(values, [&expectedValues](const NSc::TValue& value) { return  IsIn(expectedValues, value.GetString()); });
}

// Used to match requests and responses.
struct TBulkRequestIdentity {
    TStringBuf Type;
    TStringBuf Id;
    TStringBuf UserId;
    TStringBuf Kind;
    bool RichTracks = false;

    bool operator==(const TBulkRequestIdentity& other) const {
        return Type == other.Type &&
               Id == other.Id &&
               UserId == other.UserId &&
               Kind == other.Kind &&
               RichTracks == other.RichTracks;
    }

    static TBulkRequestIdentity FromValue(const NSc::TValue& value) {
        return TBulkRequestIdentity {
            .Type = value[TYPE_FIELD].GetString(),
            .Id = value[ID_FIELD].GetString(),
            .UserId = value[USER_ID_FIELD].GetString(),
            .Kind = value[KIND_FIELD].GetString(),
            .RichTracks = value[RICH_TRACKS_FIELD].GetBool(),
        };
    }

    struct THash {
        bool operator()(const TBulkRequestIdentity& value) const {
            return MultiHash(value.Type, value.Id, value.UserId, value.Kind, value.RichTracks);
        }
    };
};

class TMusicOverWebSearch {
public:
    explicit TMusicOverWebSearch(TContext& ctx,
                                 const NAlice::TClientFeatures& clientFeatures,
                                 NAlice::NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
                                 const TInstant requestStartTime,
                                 const bool richTracks)
        : Ctx(ctx)
        , CommonHeaders(CreateCommonHeaders(ctx))
        , ClientFeatures(clientFeatures)
        , AnalyticsInfoBuilder(analyticsInfoBuilder)
        , RequestStartTime(requestStartTime)
        , CatalogRequests(NHttpFetcher::WeakMultiRequest())
        , RichTracks(richTracks)
    {
    }

    NHttpFetcher::THandle::TRef CreateRequestHandler(TStringBuf searchText, NHttpFetcher::IMultiRequest::TRef multiRequest = nullptr);
    TResultValue WaitAndParseResponse(NHttpFetcher::THandle::TRef handler, NSc::TValue* answer, NSc::TValue* factorsData = nullptr);

    int GetStatusCode() const {
        return StatusCode;
    }

    const TString& GetSearchText() const {
        return SearchText;
    }

    const TString& GetEncodedAliceMeta() const {
        return EncodedAliceMeta;
    }

    TMaybe<i64> GetWebSearchMilliseconds() const {
        if (StartMilliseconds && WebSearchEndMilliseconds) {
            return *WebSearchEndMilliseconds - *StartMilliseconds;
        }
        return Nothing();
    }

    TMaybe<i64> GetCatalogMilliseconds() const {
        if (CatalogStartMilliseconds && CatalogEndMilliseconds) {
            return *CatalogEndMilliseconds - *CatalogStartMilliseconds;
        }
        return Nothing();
    }

    TMaybe<i64> GetTotalMilliseconds() const {
        if (StartMilliseconds && TotalEndMilliseconds) {
            return *TotalEndMilliseconds - *StartMilliseconds;
        }
        return Nothing();
    }

    using TIterateCatalogTimingsCallback = std::function<void(
        const TString& /* path */,
        const TString& /* type */,
        const i64 /* timeMilliseconds */
    )>;
    void IterateCatalogTimings(TIterateCatalogTimingsCallback callback) {
        for (const auto& [path, timings] : CatalogTimings) {
            if (timings.StartMilliseconds && timings.EndMilliseconds) {
                callback(path, timings.Type, *timings.EndMilliseconds - *timings.StartMilliseconds);
            }
        }
    }

private:
    struct TWebResult {
        NSc::TValue WebData;
        NSc::TValue MusicAnswer;
    };

private:
    TResultValue ParseResponse(NHttpFetcher::TResponse::TConstRef response, NSc::TValue* answer,
                               NSc::TValue* factorsData = nullptr);
    void FetchBulkCatalogRequest(TMusicCatalogBulk& catalogHandler, const NSc::TValue& requestBody);

    void WaitCatalogRequests();
    void ProcessBulkRequest();
    NSc::TValue GetMusicDataFromDocUrl(TStringBuf url) const;

private:
    TContext& Ctx;
    TCommonHeaders CommonHeaders;
    const NAlice::TClientFeatures& ClientFeatures;
    NAlice::NScenarios::IAnalyticsInfoBuilder& AnalyticsInfoBuilder;
    TInstant RequestStartTime;
    TVector<TWebResult> Results;
    NHttpFetcher::IMultiRequest::TRef CatalogRequests;
    bool RichTracks = false;
    std::unique_ptr<TMusicCatalogBulk> BulkCatalogHandler;

    int StatusCode = 0;
    TString SearchText;
    TString EncodedAliceMeta;

    TMaybe<i64> StartMilliseconds;
    TMaybe<i64> WebSearchEndMilliseconds;
    TMaybe<i64> CatalogStartMilliseconds;
    TMaybe<i64> CatalogEndMilliseconds;
    TMaybe<i64> TotalEndMilliseconds;

    struct TCatalogTiming {
        TMaybe<i64> StartMilliseconds;
        TMaybe<i64> EndMilliseconds;
        TString Type;
    };

    THashMap<TString, TCatalogTiming> CatalogTimings;  // stores timings of parallel catalog requests, indexed by path
};

void StripFactorsData(NSc::TValue& data) {
    // RandomLog is about 90% of the factors data, and we don't need it at all
    static constexpr TStringBuf chonk = "RandomLog";

    if (data.IsDict()) {
        auto& dict = data.GetDictMutable();
        dict.erase(chonk);
        for (auto& [key, value] : dict) {
            StripFactorsData(value);
        }
        return;
    }

    if (data.IsArray()) {
        for (auto& value : data.GetArrayMutable()) {
            StripFactorsData(value);
        }
    }
}

struct TSearchMusicResult {
    TResultValue Result;
    int StatusCode = 0;
    TString SearchText;
    TString EncodedAliceMeta;
    NSc::TValue WebAnswer = NSc::TValue::Null();
    NSc::TValue FactorsData = NSc::TValue::Null();
    TMaybe<i64> TotalMilliseconds;
    TMaybe<i64> WebSearchMilliseconds;
    TMaybe<i64> CatalogMilliseconds;

    TVector<TCheckAndSearchMusicResult::TCatalogTiming> CatalogTimings;
};

// TODO(a-square): purge the context before moving to Hollywood
TSearchMusicResult SearchMusic(TContext& ctx,
                               const NAlice::TClientFeatures& clientFeatures,
                               NAlice::NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
                               const TInstant requestStartTime,
                               const TStringBuf text,
                               const bool richTracks) {
    TSearchMusicResult result;

    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();
    TMusicOverWebSearch searchWeb(ctx, clientFeatures, analyticsInfoBuilder, requestStartTime, richTracks);

    auto handle = searchWeb.CreateRequestHandler(text, multiRequest);
    if (!handle) {
        result.Result = TError(TError::EType::SYSTEM, "Failed to create music search handle");
        return result;
    }

    TResultValue res = searchWeb.WaitAndParseResponse(handle, &result.WebAnswer, &result.FactorsData);
    result.StatusCode = searchWeb.GetStatusCode();
    result.SearchText = searchWeb.GetSearchText();
    result.EncodedAliceMeta = searchWeb.GetEncodedAliceMeta();
    result.TotalMilliseconds = searchWeb.GetTotalMilliseconds();
    result.WebSearchMilliseconds = searchWeb.GetWebSearchMilliseconds();
    result.CatalogMilliseconds = searchWeb.GetCatalogMilliseconds();
    searchWeb.IterateCatalogTimings([&result](const TString& path, const TString& type, i64 timeMilliseconds) {
        result.CatalogTimings.push_back({path, type, timeMilliseconds});
    });
    if (res) {
        result.Result = res;
        return result;
    }

    StripFactorsData(result.FactorsData);

    return result;
}

NHttpFetcher::THandle::TRef TMusicOverWebSearch::CreateRequestHandler(const TStringBuf searchText, NHttpFetcher::IMultiRequest::TRef multiRequest) {
    TCgiParameters cgi;
    if (ClientFeatures.HasExpFlag(FLAG_ANALYTICS_MUSIC_WEB_RESPONSES)) {
        NSerp::AddTemplateDataCgi(cgi);
    }

    cgi.InsertEscaped("wizextra", "request_type=music");

    SearchText = TString::Join(searchText, TStringBuf(" host:music.yandex.ru"));
    const auto result = NSerp::PrepareMusicSearchRequest(
        SearchText,
        Ctx,
        cgi,
        multiRequest,
        EncodedAliceMeta
    )->Fetch();
    StartMilliseconds = NAlice::SystemTimeNowMillis();
    return result;
}

TResultValue TMusicOverWebSearch::WaitAndParseResponse(NHttpFetcher::THandle::TRef handler, NSc::TValue* answer, NSc::TValue* factorsData) {
    NHttpFetcher::TResponse::TRef response = handler->Wait();
    StatusCode = response->Code;
    WebSearchEndMilliseconds = NAlice::SystemTimeNowMillis();
    const auto result = ParseResponse(response, answer, factorsData);
    TotalEndMilliseconds = NAlice::SystemTimeNowMillis();
    return result;
}

TResultValue TMusicOverWebSearch::ParseResponse(NHttpFetcher::TResponse::TConstRef response, NSc::TValue* answer,
                                                NSc::TValue* factorsData) {
    NSc::TValue searchResult;
    if (auto error = NSerp::ParseSearchResponse(response, &searchResult)) {
        return error;
    }

    if (ClientFeatures.HasExpFlag(NAlice::NExperiments::TUNNELLER_ANALYTICS_INFO) && searchResult.IsDict() &&
        searchResult.Has(NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE)) {
        AnalyticsInfoBuilder.AddTunnellerRawResponse(
            TString{searchResult[NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE].GetString()});
    }

    NSc::TArray& docs = searchResult["searchdata"]["docs"].GetArrayMutable();
    if (docs.empty()) {
        return TError(TError::EType::MUSICERROR, ERROR_MUSIC_NOT_FOUND);
    }

    TSet<TStringBuf> seenPaths;

    const bool filterStupidAnswers = ClientFeatures.HasExpFlag(FLAG_FILTER_STUPID_ANSWERS);
    for (size_t i = 0, count = docs.size(); i < count && i < MAX_WEB_RESULTS_COUNT; ++i) {
        // Skip music stupid docs
        if (filterStupidAnswers && docs[i]["markers"]["WebMusicStupidAnswer"].ForceIntNumber()) {
            continue;
        }
        // Parse music data from web doc url
        TWebResult res;
        const auto docurl = docs[i]["url"].GetString();
        NSc::TValue data = GetMusicDataFromDocUrl(docurl);
        if (data.IsNull()) {
            continue;
        }
        // For analytics
        data["docurl"].SetString(docurl);
        data["docpos"].SetIntNumber(docs[i]["num"].ForceIntNumber(i));
        // Do not duplicate requests
        if (seenPaths.insert(data["path"].GetString()).second) {
            res.WebData = std::move(data);
            Results.push_back(std::move(res));
        }
    }

    NSc::TValue requestBody;

    for (auto& r : Results) {
        if (!r.WebData.IsNull()) {
            auto& bulkField = r.WebData[BULK_DATA_FIELD];
            if (bulkField["type"] == "track") {
                bulkField["withAllPartsContainer"].SetBool(true);
            }
            requestBody["ids"].Push(bulkField);
        }
    }

    if (!requestBody.IsNull()) {
        BulkCatalogHandler = std::make_unique<TMusicCatalogBulk>(
            CommonHeaders, ClientFeatures, /* autoplay= */ true,
            Ctx.HasExpFlag(NAlice::NExperiments::EXP_MUSIC_LOG_CATALOG_RESPONSE));
        FetchBulkCatalogRequest(*BulkCatalogHandler, requestBody);
        ProcessBulkRequest();
    }

    const bool isFairyTaleFilterGenre = IsFairyTaleFilterGenre(Ctx);
    const auto fairyTaleGenres = GetFairyTaleGenres(Ctx);
    for (const auto& r : Results) {
        if (r.MusicAnswer.IsNull()) {
            continue;
        }

        const bool isChildContent = CheckMusicAnswerField(r.MusicAnswer, "genre", fairyTaleGenres) ||
            CheckMusicAnswerField(r.MusicAnswer, "subtype", {"fairy-tale"}) ||
            CheckMusicAnswerField(r.MusicAnswer, "childContent");

        if (isFairyTaleFilterGenre && !fairyTaleGenres.empty() && !isChildContent) {
            continue;
        }

        (*answer) = r.MusicAnswer;
        (*answer)["is_child_content"].SetBool(isChildContent);
        if (!r.WebData.IsNull()) {
            AnalyticsInfoBuilder
                .AddSelectedWebDocumentEvent(RequestStartTime, "music_web_search")
                    ->SetRequestId(ClientFeatures.HasExpFlag(FLAG_FOR_TESTS) ? "" : TLogging::ReqInfo.Get().ReqId())
                    .SetSearchRequestId(searchResult["reqdata"]["reqid"].ForceString())
                    .SetDocumentUrl(r.WebData["docurl"].ForceString())
                    .SetDocumentPos(r.WebData["docpos"].GetIntNumber())
                    .SetCatalogUrl(ClientFeatures.HasExpFlag(FLAG_FOR_TESTS) ? "" : r.WebData["catalogurl"].ForceString())
                    .SetAnswerUrl(r.MusicAnswer["uri"].ForceString());
        }
        break;
    }

    if (answer->IsNull()) {
        return TError(TError::EType::MUSICERROR, ERROR_MUSIC_NOT_FOUND);
    }

    if (factorsData) {
        FillSearchFactorsData(ClientFeatures, searchResult, factorsData);
    }
    return TResultValue();
}

NSc::TValue TMusicOverWebSearch::GetMusicDataFromDocUrl(const TStringBuf url) const {
    NSc::TValue obj;
    NSc::TValue bulkData;

    const TParseMusicUrlResult parseMusicUrlResult = ParseMusicDataFromDocUrl(url);

    Y_ASSERT(parseMusicUrlResult.Type == EMusicUrlType::None || parseMusicUrlResult.Parts.size() >= 1);

    for (const TParseMusicUrlResultPart& part : parseMusicUrlResult.Parts) {
        obj[part.PartName] = part.PartValue;
    }

    switch (parseMusicUrlResult.Type) {
        case EMusicUrlType::None: {
            return NSc::TValue::Null();
        }
        case EMusicUrlType::Album: {
            Y_ASSERT(parseMusicUrlResult.Parts.size() == 1);
            const TParseMusicUrlResultPart& firstPart = parseMusicUrlResult.Parts.front();
            obj[TYPE_FIELD].SetString(firstPart.PartName);
            obj[PATH_FIELD].SetString(TString::Join("/albums/", firstPart.PartValue));
            bulkData[ID_FIELD].SetString(firstPart.PartValue);
            break;
        }
        case EMusicUrlType::Track: {
            Y_ASSERT(parseMusicUrlResult.Parts.size() <= 2);
            obj[TYPE_FIELD].SetString(parseMusicUrlResult.Parts.back().PartName);
            obj[PATH_FIELD].SetString(TString::Join("/tracks/", parseMusicUrlResult.Parts.back().PartValue));
            bulkData[ID_FIELD].SetString(parseMusicUrlResult.Parts.back().PartValue);
            break;
        }
        case EMusicUrlType::Playlist: {
            Y_ASSERT(parseMusicUrlResult.Parts.size() == 2);
            const TStringBuf firstPartId = parseMusicUrlResult.Parts[0].PartValue;
            const TStringBuf secondPartId = parseMusicUrlResult.Parts[1].PartValue;
            TString userLogin(firstPartId);
            Quote(userLogin, "");
            obj[TYPE_FIELD].SetString("playlist");
            obj[PATH_FIELD].SetString(
                TString::Join("/users/", userLogin, "/playlists/", secondPartId,
                              "?rich-tracks=", (RichTracks ? "true" : "false")));
            bulkData[USER_ID_FIELD].SetString(userLogin);
            bulkData[KIND_FIELD].SetString(secondPartId);
            bulkData[RICH_TRACKS_FIELD].SetBool(RichTracks);
            break;
        }
        case EMusicUrlType::Artist: {
            Y_ASSERT(parseMusicUrlResult.Parts.size() == 1);
            const TParseMusicUrlResultPart& firstPart = parseMusicUrlResult.Parts.front();
            obj[TYPE_FIELD].SetString(firstPart.PartName);
            obj[PATH_FIELD].SetString(TString::Join("/artists/", firstPart.PartValue, "/similar"));
            bulkData[ID_FIELD].SetString(firstPart.PartValue);
            break;
        }
    }

    bulkData[TYPE_FIELD] = obj[TYPE_FIELD];
    obj[BULK_DATA_FIELD] = std::move(bulkData);
    return obj;
}

void TMusicOverWebSearch::FetchBulkCatalogRequest(TMusicCatalogBulk& catalogHandler, const NSc::TValue& requestBody) {
    auto request = Ctx.GetSources().MusicCatalogBulk("/bulk-info").MakeOrAttachRequest(CatalogRequests);
    catalogHandler.CreateRequestHandler(std::move(request), requestBody);
}

void TMusicOverWebSearch::WaitCatalogRequests() {
    CatalogStartMilliseconds = NAlice::SystemTimeNowMillis();
    CatalogRequests->WaitAll();
    CatalogEndMilliseconds = NAlice::SystemTimeNowMillis();
}

void TMusicOverWebSearch::ProcessBulkRequest() {
    Y_STATS_SCOPE_HISTOGRAM("music_catalog_bulk");
    WaitCatalogRequests();

    NSc::TValue res;
    if (!BulkCatalogHandler->ProcessRequest(&res)) {
        return;
    }

    THashMap<TBulkRequestIdentity, NSc::TValue*, TBulkRequestIdentity::THash> responseMap;
    for (auto& value : res.GetArrayMutable()) {
        responseMap[TBulkRequestIdentity::FromValue(value[REQUEST_FIELD])] = &value[RESPONSE_FIELD];
    }

    for (auto& r : Results) {
        const auto& key = r.WebData[BULK_DATA_FIELD];
        if (auto* value = responseMap.FindPtr(TBulkRequestIdentity::FromValue(key))) {
            r.MusicAnswer.Swap(**value);
        }
    }
}

} // namespace

[[nodiscard]] TCheckAndSearchMusicResult CheckAndSearchMusic(const TCheckAndSearchMusicParams& params)
{
    TCheckAndSearchMusicResult result;

    NSc::TValue webAnswer;
    bool selectedWebSource = false;

    if (params.HasSearch) {
        if (params.TextQuery.empty()) {
            result.Result = TError(TError::EType::SYSTEM, TStringBuf("search_request_parse_error"));
            return result;
        }

        NSc::TValue factorsData;

        {
            auto searchResult = SearchMusic(params.Ctx,
                                            params.ClientFeatures,
                                            params.AnalyticsInfoBuilder,
                                            params.RequestStartTime,
                                            params.TextQuery,
                                            params.RichTracks);

            params.AnalyticsInfoBuilder
                .AddRequestSourceEvent(params.RequestStartTime, "music_web_search")
                    ->SetResponseCode(searchResult.StatusCode, !searchResult.Result)
                    .AddHeader(TString{NAlice::NNetwork::HEADER_X_YANDEX_ALICE_META_INFO}, searchResult.EncodedAliceMeta)
                    .AddCgiParam("text", searchResult.SearchText)
                    .Build();

            result.Result = std::move(searchResult.Result);
            result.WebAnswer = std::move(searchResult.WebAnswer);
            factorsData = std::move(searchResult.FactorsData);
            result.TotalMilliseconds = searchResult.TotalMilliseconds;
            result.WebSearchMilliseconds = searchResult.WebSearchMilliseconds;
            result.CatalogMilliseconds = searchResult.CatalogMilliseconds;
            result.CatalogTimings = searchResult.CatalogTimings;
        }

        if (result.Result) {
            LOG(ERR) << "WEB SEARCH (music) error: " << result.Result->Msg << Endl;
            return result;
        }

        LOG(DEBUG) << "WEB SEARCH (music) answer: " << result.WebAnswer.ToJson() << Endl;

        result.WebAnswer["source"].SetString(SEARCH_SOURCE_WEB);
        selectedWebSource = true;

        if (result.WebAnswer["id"].IsNull() || result.WebAnswer["type"].IsNull()) {
            result.Result = TError(TError::EType::MUSICERROR, ERROR_MUSIC_NOT_FOUND);
            return result;
        }

        auto& featuresData = result.FeaturesData;
        featuresData["search_text"] = params.TextQuery;

        auto& searchResult = featuresData["search_result"];
        if (const auto* title = result.WebAnswer.GetNoAdd("title")) {
            searchResult["title"] = *title;
        }
        if (const auto* album = result.WebAnswer.GetNoAdd("album")) {
            searchResult["album"] = *album;
        }
        if (const auto* artists = result.WebAnswer.GetNoAdd("artists")) {
            searchResult["artists"] = *artists;
        }

        searchResult["factorsData"] = std::move(factorsData);
    }

    params.AnalyticsInfoBuilder.AddSelectedSourceEvent(
        params.RequestStartTime,
        TString{selectedWebSource ? SEARCH_SOURCE_WEB : SEARCH_SOURCE_MUSIC});

    return result;
}

} // namespace NBASS::NMusic
