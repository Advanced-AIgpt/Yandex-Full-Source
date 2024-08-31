#include "search.h"

#include "direct_gallery.h"
#include "film_gallery.h"
#include "serp.h"
#include "serp_gallery.h"

#include <alice/bass/forms/continuations.h>
#include <alice/bass/forms/market.h>
#include <alice/bass/forms/poi.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/forms/vins.h>
#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/external_skill/discovery.h>
#include <alice/bass/forms/external_skill/dj_entry_point.h>
#include <alice/bass/forms/external_skill/fwd.h>
#include <alice/bass/forms/general_conversation/general_conversation.h>
#include <alice/bass/forms/music/music.h>
#include <alice/bass/forms/navigation/navigation.h>
#include <alice/bass/forms/navigator/map_search_intent.h>
#include <alice/bass/forms/translate/translate.h>
#include <alice/bass/forms/video/video.h>

#include <alice/bass/libs/app_host/context.h>
#include <alice/bass/libs/app_host/runner.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/facts/app_host.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/push_notification/create_callback_data.h>
#include <alice/bass/libs/push_notification/request.h>
#include <alice/bass/libs/source_request/handle.h>
#include <alice/bass/setup/setup.h>

#include <alice/library/analytics/common/names.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/app_navigation/navigation.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/music/defs.h>
#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>
#include <alice/library/skill_discovery/common.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/util/utils.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/threading/future/async.h>
#include <library/cpp/threading/future/future.h>

#include <search/idl/meta.pb.h>
#include <search/session/compression/report.h>

#include <util/charset/wide.h>
#include <util/datetime/parser.h>
#include <util/string/ascii.h>
#include <util/string/cast.h>
#include <util/string/subst.h>
#include <util/system/file.h>
#include <util/thread/pool.h>

#include <cmath>

namespace NBASS {

namespace {

using namespace NSerpSnippets;

constexpr TStringBuf FORM_NAME = "personal_assistant.scenarios.search";
constexpr TStringBuf FORM_NAME_ANAPHORIC = "personal_assistant.scenarios.search__anaphoric";
constexpr TStringBuf FORM_NAME_ANAPHORIC_NEW = "personal_assistant.scenarios.search_anaphoric";
// This is a child intent "where is it located" after a search request. We show the map url or change form to find_poi in this case.
constexpr TStringBuf FORM_NAME_SHOW_ON_MAP = "personal_assistant.scenarios.search__show_on_map";
constexpr TStringBuf FORM_NAME_RELATED = "personal_assistant.scenarios.search__related";
constexpr TStringBuf FORM_NAME_RELEVANT_SKILLS = "personal_assistant.scenarios.search__skills_discovery";
constexpr TStringBuf FORM_NAME_IMAGE_GALLERY = "personal_assistant.scenarios.image_gallery";

constexpr TStringBuf IMAGES_CARD_NAME = "images_search_gallery_div1";

const int MAX_MARKET_POS = 5;

const int MAX_MEDIA_WIZARD_POS = 5;
const int MAX_TV_PROGRAM_POS = 5;
const int MAX_TRANSLATE_WIZARD_POS = 5;

constexpr TStringBuf EXP_FLAG_FORCE_FACTS = "bass_force_facts";
constexpr TStringBuf FLAG_ANALYTICS_SEARCH_WEB_RESPONSES = "analytics.search.add_old_web_response";
constexpr TStringBuf FACTS_GTA =
    "results[0]/binary/SUGGESTFACTS2/data/Grouping[0]/Group[0]/Document[0]/ArchiveInfo/GtaRelatedAttribute"sv
;

constexpr TStringBuf EXP_WEB_SEARCH_PUSH = "web_search_push";
constexpr TStringBuf ATTENTION_WEB_SEARCH_PUSH = "search__push_sent";

struct TDocWithSnippet {
    const NSc::TValue* Document;
    const NSc::TValue* Snippet;
    ui8 pos;

    constexpr explicit operator bool() const noexcept {
        return Document != nullptr;
    }
};

const TDocWithSnippet NOT_FOUND{nullptr, nullptr, 0};

const THashSet<TStringBuf> FACTOID_SOURCES_WHITELIST = {
    "object_facts",
    "big_mediawiki",
    "znatoki",
    "yandex_q_child",
    "fresh_console_fact",
    "wiki-facts-objects"
};

const THashSet<TStringBuf> FACTOID_HOSTNAMES_WHITELIST = {
    "ru.wikipedia.org",
    "ru.m.wikipedia.org"
};

TDocWithSnippet FindDocBySnippetType(NSc::TArray& docs, TStringBuf snippetType, ui8 maxPos, ESnippetSection section) {
    for (size_t i = 0, count = docs.size(); i < count && i < maxPos; ++i) {
        const NSc::TValue& snippet = FindSnippet(docs[i], snippetType, section);
        if (!snippet.IsNull())
            return {&docs[i], &snippet, static_cast<ui8>(i)};
    }
    return NOT_FOUND;
}

int FindFirstNonWebPos(const NSc::TArray& docs, ui8 maxPos) {
    for (size_t i = 0, count = docs.size(); i < count && i < maxPos; ++i) {
        if (docs[i]["server_descr"].GetString() != TStringBuf("WEB")) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

TDocWithSnippet FindDocBySnippetType(NSc::TArray& docs, TStringBuf serverDescr, TStringBuf snippetType, ui8 maxPos,
                                     ESnippetSection section) {
    for (size_t i = 0, count = docs.size(); i < count && i < maxPos; ++i) {
        NSc::TValue& doc = docs[i];
        if (doc["server_descr"].GetString() != serverDescr)
            continue;
        const NSc::TValue& snippet = FindSnippet(doc, snippetType, section);
        if (!snippet.IsNull())
            return {&docs[i], &snippet, static_cast<ui8>(i)};
    }
    return NOT_FOUND;
}

i32 FindDocPosition(NSc::TArray& docs, TStringBuf serverDescr, TStringBuf snippetType, ESnippetSection snippetSection) {
    for (size_t i = 0, count = docs.size(); i < count; ++i) {
        NSc::TValue& doc = docs[i];
        if (serverDescr && doc["server_descr"].GetString() != serverDescr)
            continue;
        const NSc::TValue& snippet = FindSnippet(doc, snippetType, snippetSection);
        if (!snippet.IsNull())
            return static_cast<int>(i);
    }
    return -1;
}

constexpr TStringBuf WIZARD_MUSIC = "musicplayer";
constexpr TStringBuf WIZARD_VIDEO = "videowiz";

i32 FindEntitySearchPosition(NSc::TArray& docs) {
    return FindDocPosition(docs, TStringBuf(), TStringBuf("entity_search"), ESS_SNIPPETS_FULL);
}

i32 FindMusicPlayerPosition(NSc::TArray& docs) {
    return FindDocPosition(docs, "YMUSIC", WIZARD_MUSIC, ESS_SNIPPETS_FULL);
}

i32 FindVideoWizardPosition(NSc::TArray& docs) {
    return FindDocPosition(docs, TStringBuf(""), "videowiz", ESS_CONSTRUCT);
}

i32 FindPoiWizardPosition(NSc::TArray& docs) {
    return FindDocPosition(docs, TStringBuf(""), "companies", ESS_SNIPPETS_FULL);
}

void AppendFormattedTime(i32 value, TStringBuf* names, TStringBuilder& dst) {
    dst << value;
    if (value > 10 && value < 20) {
        dst << names[2];
    } else {
        switch (value % 10) {
            case 1:
                dst << names[0];
                break;
            case 2:
            case 3:
            case 4:
                dst << names[1];
                break;
            default:
                dst << names[2];
        }
    }
}

TString FormatTimeDifference(i32 diff, TStringBuf tld) {
    i32 absDiff = abs(diff);
    i32 hours = absDiff / 60;
    i32 minutes = absDiff % 60;

    if (tld != TStringBuf("ru")) {
        return Sprintf("%.2d:%.2d", (diff < 0 ? -hours : hours), minutes);
    }

    if (diff == 0)
        return "Нет разницы во времени";

    static TStringBuf Hours[] = {" час" , " часа", " часов"};
    static TStringBuf Minutes[] = {" минута" , " минуты", " минут"};

    TStringBuilder text;

    if (diff < 0)
        text << "-";

    if (hours > 0)
        AppendFormattedTime(hours, Hours, text);

    if (minutes > 0) {
        if (hours > 0)
            text << " ";
        AppendFormattedTime(minutes, Minutes, text);
    }

    return text;
}

constexpr TStringBuf SEARCH_RESULTS = "search_results";
constexpr TStringBuf QUERY_SLOT = "query";
constexpr TStringBuf CHANGE_FORM_SLOT = "form_was_changed";

NSc::TValue GetWizardFactorsData(const TDocWithSnippet& doc) {
    const NSc::TValue& src = *(doc.Document);

    NSc::TValue dst;
    dst["show"].SetBool(true);
    NSc::TValue& document = dst["document"];

    NScUtils::CopyField(src, document, TStringBuf("markers"));
    NScUtils::CopyField(src, document, TStringBuf("relevance"));
    document["doctitle"].SetString(RemoveHiLight(src["doctitle"].GetString()));

    if (src.Has("snippets") && src["snippets"].Has("full")) {
        const auto& snippet = src["snippets"]["full"];
        if (snippet.Has("track_lyrics")) {
            dst["track_lyrics"].SetString(snippet["track_lyrics"].GetString());
        }
        if (snippet.Has("track_name")) {
            dst["track_name"].SetString(snippet["track_name"].GetString());
        }
        if (snippet.Has("alb_name")) {
            dst["alb_name"].SetString(snippet["alb_name"].GetString());
        }
        if (snippet.Has("grp")) {
            dst["grp"] = snippet["grp"];
        }
        if (snippet.Has("grp_similar_names")) {
            dst["grp_similar_names"] = snippet["grp_similar_names"];
        }

    }
    dst["pos"] = doc.pos;

    return dst;
}

void FillWizardsFactorsData(NSc::TArray& docs, TStringBuf snippetType, NSc::TValue* factorsData) {
    if (TDocWithSnippet doc = FindDocBySnippetType(docs, snippetType, /*maxPos*/ 20, ESS_ALL)) {
        (*factorsData)["wizards"][snippetType] = GetWizardFactorsData(doc);
    }
}

NSc::TValue GetDocumentFactorsData(const NSc::TValue& doc, ui8 pos) {
    NSc::TValue dst;

    dst["pos"] = pos;
    dst["doctitle"].SetString(RemoveHiLight(doc["doctitle"].GetString()));

    if (doc.Has("snippets") && doc["snippets"].Has("main")) {
        const auto& snippet = doc["snippets"]["main"];
        if (snippet.Has("passages")) {
            dst["passages"].SetString(snippet["passages"].GetString());
            dst["passages_hilighted"].SetString(snippet["passages"].GetString());
        }
    }

    NScUtils::CopyField(doc, dst, TStringBuf("url"));
    NScUtils::CopyField(doc, dst, TStringBuf("host"));
    NScUtils::CopyField(doc, dst, TStringBuf("markers"));
    NScUtils::CopyField(doc, dst, TStringBuf("relevance"));

    return dst;
}

void FillDocumentsFactorsData(NSc::TArray& docs, NSc::TValue* factorsData, ui8 maxDocuments) {
    NSc::TValue& documentsFactors = (*factorsData)["documents"].SetArray();
    for (size_t i = 0, count = 0; i < docs.size(); ++i) {
        if (docs[i]["server_descr"].GetString() == TStringBuf("WEB")) {
            documentsFactors.Push(GetDocumentFactorsData(docs[i], i));
            if (++count >= maxDocuments) {
                break;
            }
        }
    }
}

class TParallelFindPoi {
public:
    TParallelFindPoi(TRequestHandler& r, IThreadPool& threadPool)
        : RequestHandler_{r}
    {
        TContext& ctx = r.Ctx();

        auto promise = NThreading::NewPromise<TFuture::value_type>();
        auto cb = [promise, &ctx]() mutable {
            try {
                ctx.UpdateLoggingReqInfo();
                // XXX not sure its ok
                TLogging::ReqInfo.Get().AppendToReqId("-parallel-findpoi");
                TResult result{.Context = ctx.Clone()};

                std::visit(result, TPoiFormHandler::TryToHandle(*result.Context, ctx.GetSlot(QUERY_SLOT)->Value.GetString()));

                promise.SetValue(std::move(result));
            } catch (...) {
                promise.SetException(CurrentExceptionMessage());
            }
        };
        if (threadPool.AddFunc(cb)) {
            Future_ = promise.GetFuture();
        }
    }

    ~TParallelFindPoi() {
        if (IsStarted()) {
            Future_.Wait();
        }
    }

    bool IsStarted() const {
        return Future_.Initialized();
    }

    bool IsSuitable() {
        try {
            const TResult& result = Future_.GetValueSync();
            return result.IsSuitable;
        } catch (...) {
            LOG(ERR) << "Exception during handle ParallelFindPoi request" << CurrentExceptionMessage() << Endl;
        }

        return false;
    }

    // FIXME prevent running it twice
    TResultValue Process() {
        try {
            TResult result = Future_.GetValueSync();
            if (result.IsSuitable) {
                RequestHandler_.SwapContext(result.Context);
                return result.Result;
            }
        } catch (...) {
            LOG(ERR) << "Exception during handle ParallelFindPoi request" << Endl;
        }
        return Nothing();
    }

private:
    struct TResult {
        TContext::TPtr Context;
        TResultValue Result;
        bool IsSuitable = false;

        void operator()(IParallelHandler::ETryResult resultType) {
            switch (resultType) {
                case IParallelHandler::ETryResult::Success:
                    IsSuitable = true;
                    break;
                case IParallelHandler::ETryResult::NonSuitable:
                    IsSuitable = false;
                    break;
            }
        }

        void operator()(TError&& e) {
            IsSuitable = true;
            Result = std::move(e);
        }
    };
    using TFuture = NThreading::TFuture<TResult>;

private:
    TRequestHandler& RequestHandler_;
    TFuture Future_;
};

size_t GetNumTopsToConsiderInImageSearch(const TContext& ctx) {
    if (ctx.HasExpFlag("search_images_among_top_1")) {
        return 1;
    } else if (ctx.HasExpFlag("search_images_among_top_3")) {
        return 3;
    } else if (ctx.HasExpFlag("search_images_among_top_4")) {
        return 4;
    } else if (ctx.HasExpFlag("search_images_among_any")) {
        return std::numeric_limits<size_t>::max();
    } else {
        return 2;
    }
}

bool SendWebSearchPush(TContext* ctx, TStringBuf query, TStringBuf url) {
    if (query.empty() || url.empty()) {
        return false;
    }
    NSc::TValue serviceData;
    serviceData["query"] = query;
    serviceData["url"] = url;
    ctx->SendPushRequest("web_search", "web_search", Nothing(), serviceData);
    return true;
}

} // namespace

NSc::TValue TSearchFormHandler::Stubs;

TSearchFormHandler::TSearchFormHandler(IThreadPool& threadPool)
    : ThreadPool{threadPool}
{
}

TString TSearchFormHandler::NormalizedQuery() {
    if (!Context->HasExpFlag("source_normalized_query")) {
        return TString();
    }
    NHttpFetcher::TRequestPtr req = Context->GetSources().NormalizedQuery().Request();
    TCgiParameters cgi;
    cgi.InsertUnescaped("part", Query);
    cgi.InsertUnescaped("lr", ToString(Context->UserRegion()));
    cgi.InsertUnescaped("tld", Context->Meta().Tld());
    cgi.InsertUnescaped("uil", Context->MetaLocale().Lang);
    req->AddCgiParams(cgi);
    NHttpFetcher::TResponse::TRef resp = req->Fetch()->Wait();
    if (resp->IsError()) {
        LOG(DEBUG) << "Fetching error: " << resp->GetErrorText() << Endl;
        return TString();
    }

    NMetaProtocol::TReport report;
    if (!report.ParseFromString(resp->Data)) {
        LOG(DEBUG) << "Unable to parse protobuf" << Endl;
        return TString();
    }
    NMetaProtocol::Decompress(report);

    for (size_t i = 0; i < report.GroupingSize(); ++i) {
        const NMetaProtocol::TGrouping& grouping = report.GetGrouping(i);
        if (!grouping.HasAttr() || grouping.GetAttr() != TStringBuf("suggestfacts2")) {
            continue;
        }
        for (size_t j = 0; j < grouping.GroupSize(); ++j) {
            const NMetaProtocol::TGroup& group = grouping.GetGroup(j);
            if (!group.HasCategoryName() || group.GetCategoryName() != TStringBuf("suggestfacts2")) {
                continue;
            }
            for (size_t k = 0; k < group.DocumentSize(); ++k) {
                const NMetaProtocol::TDocument& document = group.GetDocument(k);
                const NMetaProtocol::TArchiveInfo& archiveInfo = document.GetArchiveInfo();
                for (size_t g = 0; g < archiveInfo.GtaRelatedAttributeSize(); ++g) {
                    const NMetaProtocol::TPairBytesBytes& gta = archiveInfo.GetGtaRelatedAttribute(g);
                    if (TStringBuf("_SerpData") == gta.GetKey()) {
                        NSc::TValue responseJson = NSc::TValue::FromJson(gta.GetValue());
                        if (responseJson.IsNull()) {
                            LOG(DEBUG) << "Serpdata parse error: " << resp->Data;
                            return TString();
                        }
                        TString newQuery(responseJson["headline"].GetString());
                        LOG(DEBUG) << "Normalized query: " << newQuery << Endl;
                        return newQuery;
                    }
                }
            }
        }
    }
    return TString();
}

NHttpFetcher::THandle::TRef FetchSearchRequest(const TStringBuf query, TContext& ctx, NHttpFetcher::IMultiRequest::TRef multiRequest) {

    TCgiParameters cgi;
    cgi.InsertUnescaped("noreask", "1");

    // ASSISTANT-1224
    // TODO (a-sidorin@): This flag is already passed via headers. Can we remove CGI?
    constexpr TStringBuf entityAsFactFlag = "scheme_Local/Facts/Create/EntityAsFactFlag=1";
    cgi.InsertUnescaped("rearr", entityAsFactFlag);

    // DIALOG-5161
    cgi.InsertUnescaped("rearr", "scheme_Local/ImgSerpData/EnableAlice=1");

    if (TMaybe<TStringBuf> flag = ctx.ExpFlag(TStringBuf("bass_search_intent_rearr"))) {
        cgi.InsertUnescaped("rearr", *flag);
    }

    if (ctx.HasExpFlag(FLAG_ANALYTICS_SEARCH_WEB_RESPONSES)) {
        NSerp::AddTemplateDataCgi(cgi);
    }

    auto request = NSerp::PrepareSearchRequest(query, ctx, cgi, NAlice::TWebSearchBuilder::EService::Bass,
                                               multiRequest);
    LOG(DEBUG) << "search request: " << request->Url() << Endl;
    LOG(DEBUG) << "search request headers: "  << Endl;
    auto headers = request->GetHeaders();
    for (const auto& header: headers) {
        if (AsciiHasPrefixIgnoreCase(header, NAlice::NNetwork::HEADER_COOKIE)) {
            LOG(RESOURCES) << "    " << header << Endl;
        } else {
            LOG(DEBUG) << "    " << header << Endl;
        }
    }

    return request->Fetch();
}

class TSearchRequest : public TJsonSetupRequest {
public:
    TSearchRequest(TContext& ctx, TStringBuf query)
        : TJsonSetupRequest("search")
        , Ctx(ctx)
        , Query(query)
    {
    }

    NHttpFetcher::THandle::TRef Fetch(NHttpFetcher::IMultiRequest::TRef multiRequest) override {
        return FetchSearchRequest(Query, Ctx, multiRequest);
    }

    TResultValue Parse(NHttpFetcher::TResponse::TConstRef response, NSc::TValue* searchResult, NSc::TValue* factorsData) override {
        TResultValue err = NSerp::ParseSearchResponse(response, searchResult);

        if (!err && factorsData) {
            FillSearchFactorsData(Ctx.MetaClientInfo(), *searchResult, factorsData);
        }

        return err;
    }

private:
    TContext& Ctx;
    TString Query;
};

namespace {
using TServiceResponseSetupRequest = TSetupRequest<NExternalSkill::TServiceResponse>;
}
class TSkillsDiscoveryRequest : public TServiceResponseSetupRequest {
public:
    TSkillsDiscoveryRequest(TContext& ctx, TStringBuf query)
        : TServiceResponseSetupRequest("skills_discovery")
        , Ctx(ctx)
        , Query(query)
    {
    }

    NHttpFetcher::THandle::TRef Fetch(NHttpFetcher::IMultiRequest::TRef multiRequest) override {
        return NExternalSkill::FetchSkillsDiscoveryRequest(Query, Ctx, multiRequest);
    }

    TResultValue Parse(NHttpFetcher::TResponse::TConstRef response, NExternalSkill::TServiceResponse* answer, NSc::TValue* /* factorsData */) override {
        if (NExternalSkill::ParseResponse(Ctx, response, *answer)) {
            return TResultValue();
        }
        return TError(TError::EType::SKILLSDISCOVERYERROR, response->GetErrorText());
    }

private:
    TContext& Ctx;
    TString Query;
};

TResultValue TSearchFormHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    auto& analyticsInfoBuilder = ctx.GetAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::SEARCH);
    TContext::TSlot* slotQuery = ctx.GetSlot(QUERY_SLOT);
    if (IsSlotEmpty(slotQuery)) {
        return TError(TError::EType::INVALIDPARAM, "query");
    }

    TMaybe<TParallelFindPoi> parallelFindPoi;
    if (ctx.GetConfig().HasSearchThreads() && !ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_PARALLEL_SEARCH)) {
        LOG(DEBUG) << "Parallel find poi search enable" << Endl;
        parallelFindPoi.ConstructInPlace(r, ThreadPool);
        if (!parallelFindPoi->IsStarted()) {
            return TError{TError::EType::SYSTEM, "Unable to run FindPoi in parallel (no threads)"};
        }
    }

    Query = slotQuery->Value.GetString();
    Context = &r.Ctx();
    const bool disableFacts = ctx.HasExpFlag(NAlice::NExperiments::DISABLE_BASS_FACTS) ||
        (ctx.HasExpFlag(NAlice::NExperiments::DISABLE_BASS_FACTS_ON_ONEWORD) && !Query.Contains(' '));

    if (r.Ctx().HasExpFlag(EXP_FLAG_FORCE_FACTS) && !disableFacts) {
        Answer = Context->CreateSlot(SEARCH_RESULTS, SEARCH_RESULTS);

        TFactsAppHostSource factsSource(Query,
                                        Context->Meta().Tld(),
                                        Context->MetaLocale().Lang);

        TAppHostInitContext appHostInitContext;
        appHostInitContext.AddSourceInit(factsSource);

        TAppHostRunner appHostRunner(Context->GetSources().AliceGraph());
        TAppHostResultContext appHostResultContext = appHostRunner.Fetch(appHostInitContext);

        TMaybe<NSc::TValue> factsAnswer = appHostResultContext.GetItemRef("SUGGESTFACTS2_APPHOST");
        if (!factsAnswer.Defined()) {
            return NothingFound(false /* addAttention */,
                                TError(TError::EType::SYSTEM, TStringBuf("Cannot get answer from AppHost.")));
        }

        const NSc::TValue& factsGta = factsAnswer.Get()->TrySelect(FACTS_GTA);

        NSc::TValue factSerpData;
        for (const auto& item : factsGta.GetArray()) {
            if (item["Key"].GetString() == TStringBuf("_SerpData")) {
                factSerpData = NSc::TValue::FromJson(item["Value"].GetString());
                break;
            }
        }

        bool hasAnswer = AddFactoidAppHost(factSerpData);
        return hasAnswer ? TResultValue() : NothingFound(false /*addAttention*/);
    }

    TContext::TSlot* slotDisableChangeIntent = r.Ctx().GetSlot(TStringBuf("disable_change_intent"));

    // for elliptic show-on-map queries, trigger find_poi if there is no map
    if (Context->FormName() == FORM_NAME_SHOW_ON_MAP) {
        Answer = Context->GetSlot(SEARCH_RESULTS);
        // if the answer has the map url, just return the form as is, so that VINS could show the map
        if (!IsSlotEmpty(Answer) && Answer->Value.Has("map_search_url") && !Answer->Value["map_search_url"].IsNull()) {
            return TResultValue();
        }

        // otherwise, change the form to find_poi and execute it
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search__show_on_map_switch_to_find_poi");
        TPoiFormHandler::SetAsResponse(r.Ctx(), Query);
        const auto result = Context->RunResponseFormHandler();
        analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::NAVI_COMMANDS);
        return result;
    }

    Answer = Context->CreateSlot(SEARCH_RESULTS, SEARCH_RESULTS);
    DisableChangeIntent = IsSlotEmpty(slotDisableChangeIntent) ? false : slotDisableChangeIntent->Value.GetBool();
    IsAndroid = Context->MetaClientInfo().IsAndroid();
    IsIos = Context->MetaClientInfo().IsIOS();
    IsTouch = Context->MetaClientInfo().IsTouch();
    IsSmartSpeaker = Context->MetaClientInfo().IsSmartSpeaker();
    IsElariWatch = Context->MetaClientInfo().IsElariWatch();
    IsTouchSearch = NSerp::IsTouchSearch(Context->MetaClientInfo());
    IsTvPluggedIn = Context->Meta().DeviceState().IsTvPluggedIn();
    IsYaAuto = Context->MetaClientInfo().IsYaAuto();

    if (Context->ClientFeatures().SupportsNavigator()) {
        if (parallelFindPoi.Defined()) {
            if (!DisableChangeIntent && parallelFindPoi->IsSuitable()) {
                Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_poi");
                return parallelFindPoi->Process();
            }
        } else if (!DisableChangeIntent && TPoiFormHandler::TryToHandleSimple(*Context, Query)) {
            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_poi");
            return TResultValue();
        }

        TMapSearchNavigatorIntent navigatorIntent(*Context, Query, InitGeoPositionFromLocation(Context->Meta().Location()));
        TString intentString = navigatorIntent.MakeIntentString();
        Answer->Value["map_search_url"] = intentString;
        Context->AddSuggest(TStringBuf("search__show_on_map"));
    }

    if (ApplyFixedAnswers()) {
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_fixlist");
        return TResultValue();
    }

    //TODO: why is it here?
    TString newQuery = NormalizedQuery();
    if (newQuery) {
        Query = newQuery;
    }
    auto multiRequest = NHttpFetcher::WeakMultiRequest();
    TContext::TSlot* slotActivation = Context->GetSlot(NExternalSkill::ACTIVATION_SLOT_NAME);
    auto sourceIntent = IsSlotEmpty(slotActivation) ? NExternalSkill::EDiscoverySourceIntent::Search : NExternalSkill::EDiscoverySourceIntent::ExternalSkill;
    bool skipDiscovery = sourceIntent != NExternalSkill::EDiscoverySourceIntent::ExternalSkill && IsCommercialQuery(ctx, Query);

    std::unique_ptr<IRequestHandle<NSc::TValue>> searchRequestHandler =
            FetchSetupRequest<NSc::TValue>(THolder(new TSearchRequest(*Context, Query)), *Context, multiRequest);
    std::unique_ptr<IRequestHandle<NExternalSkill::TServiceResponse>> skillsRequestHandler =
        Context->HasExpFlag(EXPERIMENTAL_FLAG_ENABLE_SKILLS_DISCOVERY) && !skipDiscovery
        ? FetchSetupRequest<NExternalSkill::TServiceResponse>(THolder(new TSkillsDiscoveryRequest(*Context, Query)), *Context, multiRequest)
        : nullptr;
    multiRequest->WaitAll();

    NSc::TValue searchResult;
    if (TResultValue err = searchRequestHandler->WaitAndParseResponse(&searchResult)) {
        return NothingFound(false /* addAttention */, err, disableFacts);
    }

    if (r.Ctx().HasExpFlag(NAlice::NExperiments::TUNNELLER_ANALYTICS_INFO) && searchResult.IsDict() &&
        searchResult.Has(NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE)) {
        Context->GetAnalyticsInfoBuilder().AddTunnellerRawResponse(
                TString{searchResult[NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE].GetString()});
    }

    if (r.Ctx().HasExpFlag(NAlice::NExperiments::FLAG_FACT_SNIP_ADDITIONAL_DATA)) {
        NSc::TArray &wizplaces = searchResult["wizplaces"]["important"].GetArrayMutable();
        if (!wizplaces.empty()) {
            ctx.GetAnalyticsInfoBuilder().AddRequestSourceEvent(ctx.GetRequestStartTime(), "fact_snip_additional_data")
                ->SetResponseCode(200, true)
                .SetResponseBody(wizplaces[0]["construct"][0].ToJsonSafe());
        }
    }

    Tld = searchResult.TrySelect("/reqdata/tld").GetString("ru");
    NSc::TArray& docs = searchResult["searchdata"]["docs"].GetArrayMutable();
    NSc::TArray& docsRight = searchResult["searchdata"]["docs_right"].GetArrayMutable();

    if (docs.empty()) {
        return NothingFound(true /* addAttention */);
    }

    bool porno = NSerp::IsPornoSerp(searchResult);
    if (porno) {
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_porno");
    }

    if ((Context->GetContentRestrictionLevel() == EContentRestrictionLevel::Children) && porno) {
        return NothingFound(true /* addAttention */);
    }

    // wizplaces.important or docs[0]
    bool hasAnswer = false;
    if (!disableFacts) {
        hasAnswer = AddFactoid(searchResult);

        if (!hasAnswer && !porno && r.Ctx().HasExpFlag("search_enable_bno")) {
            hasAnswer = AddNav(docs[0]);
        }
        if (!hasAnswer) {
            hasAnswer = AddObjectAsFact(docs[0]);
        }
        if (!hasAnswer && !docsRight.empty()) {
            hasAnswer = AddObjectAsFact(docsRight[0]);
        }
        if (!hasAnswer) {
            if (ShouldAnswerWithSummarization(searchResult) || ctx.HasExpFlag(NAlice::NExperiments::SERP_SUMMARIZATION_BE_BRAVE)) {
                hasAnswer = AddSummarizationAnswer(searchResult[NSerp::SUMMARIZATION]);
            }
        }
    }

    if (hasAnswer) {
        ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::ONTOFACTS);
    }

    if (!hasAnswer) {
        // iterate over docs
        int tvProgramPos = FindTvProgramPosition(docs);
        int musicPlayerPos = FindMusicPlayerPosition(docs);
        int videoWizardPos = FindVideoWizardPosition(docs);
        int marketPos = FindMarketPosition(docs);
        int objectAnswerPos = FindEntitySearchPosition(IsTouchSearch ? docs : docsRight);
        int translateWizardPos = FindDocPosition(docs, "TRANSLATE_PROXY", "translate", ESS_SNIPPETS_FULL);

        if (tvProgramPos >= 0 && tvProgramPos < MAX_TV_PROGRAM_POS && tvProgramPos < musicPlayerPos && tvProgramPos < objectAnswerPos) {
            // есть ТВ-программа и она выше музыкального колдунщика и объектного ответа
            hasAnswer = AddTvProgram(docs[tvProgramPos]);
        }

        if(!hasAnswer && !DisableChangeIntent
                && marketPos != -1 && marketPos < MAX_MARKET_POS
                && (objectAnswerPos == -1 || (marketPos < objectAnswerPos))
                && (musicPlayerPos == -1 || (marketPos < musicPlayerPos))
                && (videoWizardPos == -1 || (marketPos < videoWizardPos))
                && (translateWizardPos == -1 || (marketPos < translateWizardPos))) {

            if (!Context->HasExpFlag("how_much_disable") && TMarketHowMuchFormHandler::MustHandle(Query)) {
                TMarketHowMuchFormHandler::SetAsResponse(*Context);
                Y_STATS_INC_COUNTER("search_switch_to_market");
                return Context->RunResponseFormHandler();
            }
        }

        if (!hasAnswer) {
            bool hasMusicSuggest = false;

            int firstNonWebRes = FindFirstNonWebPos(docs, MAX_MEDIA_WIZARD_POS);
            if (IsTouchSearch) {
                // SERP has video wizard
                if (!DisableChangeIntent &&
                    !Context->IsIntentForbiddenByExperiments(NAlice::NVideoCommon::SEARCH_VIDEO) &&
                    IsTvPluggedIn && videoWizardPos >= 0 && (musicPlayerPos < 0 || videoWizardPos < musicPlayerPos))
                {
                    NSc::TValue videoSnippet = GetVideoSnippetData(docs);
                    TString searchText(videoSnippet.Get("user_request").GetString());
                    if (!searchText.empty() && TVideoSearchFormHandler::SetAsResponse(*Context, false, searchText)) {
                        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_video");
                        return Context->RunResponseFormHandler();
                    }
                }

                if (musicPlayerPos >= 0) {
                    // муз колдун должен быть выше любого другого не-веб результата ASSISTANT-1694
                    if (musicPlayerPos < MAX_MEDIA_WIZARD_POS &&
                            (
                             (firstNonWebRes < 0) ||
                             (musicPlayerPos == firstNonWebRes) ||
                             IsSmartSpeaker
                            ) // check all possible wizards
                       ) {
                        if (!Context->IsIntentForbiddenByExperiments(NAlice::NMusic::MUSIC_PLAY) &&
                            !DisableChangeIntent &&
                            !Context->HasExpFlag(EXPERIMENTAL_FLAG_SEARCH_NO_MUSIC_FALLBACK) &&
                            NMusic::TSearchMusicHandler::SetAsResponse(*Context, false, GetMusicSnippetData(docs)))
                        {
                            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_music");
                            return Context->RunResponseFormHandler();
                        }
                    }

                    if (objectAnswerPos >= 0 && !disableFacts) {
                        hasAnswer = AddObjectAsFact(docs[objectAnswerPos]);
                    }

                    hasMusicSuggest = true;
                    AddMusicSuggest(docs);
                }

                if (!hasAnswer && !hasMusicSuggest) {
                    if (TryToSwitchToTranslate(translateWizardPos, firstNonWebRes)) {
                        return Context->RunResponseFormHandler();
                    }
                    if (!disableFacts) {
                        hasAnswer = AddSearchStubs(docs);
                    }
                }

            } else {
                // desktop (telegram bot and other unknown stuff)

                if (musicPlayerPos >= 0) {
                    // муз колдун должен быть выше любого другого не-веб результата ASSISTANT-1694
                    if (musicPlayerPos < MAX_MEDIA_WIZARD_POS && (firstNonWebRes < 0 || musicPlayerPos < firstNonWebRes)) {
                        // todo: музыкальный колдун может быть в правой колонке, тогда его позация заведомо > 5
                        if (!Context->IsIntentForbiddenByExperiments(NAlice::NMusic::MUSIC_PLAY) &&
                            !Context->HasExpFlag(EXPERIMENTAL_FLAG_SEARCH_NO_MUSIC_FALLBACK) &&
                            NMusic::TSearchMusicHandler::SetAsResponse(*Context, false, GetMusicSnippetData(docs)))
                        {
                            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_music");
                            return Context->RunResponseFormHandler();
                        }
                    } else {
                        hasMusicSuggest = true;
                    }
                }

                if (!hasMusicSuggest) {
                    if (TryToSwitchToTranslate(translateWizardPos, firstNonWebRes)) {
                        return Context->RunResponseFormHandler();
                    }
                    if (!disableFacts) {
                        hasAnswer = AddSearchStubs(docs);
                    }
                }

                if (!hasAnswer && !disableFacts) {
                    for (size_t i = 0, docsCount = docsRight.size(); i < docsCount && !hasAnswer; ++i) {
                        // OO has lowest priority over other stubs
                        hasAnswer = AddObjectAsFact(docsRight[i]);
                        if (hasAnswer && hasMusicSuggest) {
                            // add music suggest after "search__factoid_src"
                            AddMusicSuggest(docs);
                        }
                    }
                }

                if (!hasAnswer && hasMusicSuggest)
                    AddMusicSuggest(docs);
            }
        }
    }

    /**
     * Change to FIND_POI if poi snippet found on 1st position and the flag no_search_poi is not present.
     */
    if (!hasAnswer && !DisableChangeIntent && !Context->HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_SEARCH_POI)) {
        if (parallelFindPoi.Defined()) {
            if (FindPoiWizardPosition(docs) == 0 && parallelFindPoi->IsSuitable()) {
                Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_poi");
                return parallelFindPoi->Process();
            }
        } else {
            if (FindPoiWizardPosition(docs) == 0 && TPoiFormHandler::TryToHandleSimple(*Context, Query)) {
                Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_poi");
                return TResultValue();
            }

        }
    }

    if (parallelFindPoi.Defined()) {
        if (!hasAnswer && IsYaAuto && !DisableChangeIntent && parallelFindPoi->IsSuitable()) {
            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_poi");
            return parallelFindPoi->Process();
        }
    } else if (!hasAnswer && IsYaAuto && !DisableChangeIntent && TPoiFormHandler::TryToHandleSimple(*Context, Query)) {
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_poi");
        return TResultValue();
    }

    if (IsYaAuto) {
        Context->AddStopListeningBlock();
    }

    auto filmGalleryBuilder = NFilmGallery::TFilmGalleryBuilder::Create(*Context);
    if (!hasAnswer && filmGalleryBuilder && filmGalleryBuilder->TryBuild(searchResult)) {
        return TResultValue();
    }

    auto directGalleryBuilder = NDirectGallery::TDirectGalleryBuilder::Create(*Context);
    if (!hasAnswer && !DisableChangeIntent && directGalleryBuilder) {
        TContext::TPtr context = directGalleryBuilder->Build(Query, searchResult);
        if (context) {
            if (!context->HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_RELATED_ON_DIRECT_GALLERY)) {
                AddRelated(searchResult, context.Get());
            }
            return TResultValue();
        }
    }

    auto serpGalleryBuilder = NSerpGallery::TSerpGalleryBuilder::Create(*Context);
    if (!hasAnswer && serpGalleryBuilder) {
        serpGalleryBuilder->Build(Query, searchResult);
        AddRelated(searchResult, serpGalleryBuilder->Ctx);
        return TResultValue();
    }

    if (ctx.ClientFeatures().SupportsDivCards() && ctx.Meta().HasImageSearchGranet()) {
        size_t numTopToConsider = Min(GetNumTopsToConsiderInImageSearch(ctx), docs.size());
        for (size_t i = 0; i < numTopToConsider; ++i) {
            for (const auto& constructValue : docs[i].TrySelect("construct").GetArray()) {
                if (constructValue.TrySelect("alice_info/card").GetString() == IMAGES_CARD_NAME) {
                    TContext::TPtr newContext = Context->SetResponseForm(FORM_NAME_IMAGE_GALLERY, false /* setCurrentFormAsCallback */);
                    if (newContext) {
                        newContext->GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::IMAGES_GALLERY);
                        newContext->AddDivCardBlock(IMAGES_CARD_NAME, constructValue);
                        newContext->AddAttention("search_images");
                        newContext->AddStopListeningBlock();
                        return TResultValue();
                    }
                }
            }
        }
    }

    bool addSerpSuggest = true;
    if (!hasAnswer && (skillsRequestHandler)) {
        addSerpSuggest = !AddRelevantSkills(skillsRequestHandler.get(), sourceIntent);
        if (addSerpSuggest && sourceIntent == NExternalSkill::EDiscoverySourceIntent::ExternalSkill) {
            return NothingFound(true /* addAttention */, TResultValue(), disableFacts);
        }
    }

    if (!disableFacts || ctx.HasExpFlag(NAlice::NExperiments::ENABLE_SERP_REDIRECT)) {
        hasAnswer |= AddSerp(Query, docs, addSerpSuggest);
        hasAnswer |= AddRelated(searchResult, Context);
    }
    if (!hasAnswer) {
        if (IsSmartSpeaker && Context->HasExpFlag(EXP_WEB_SEARCH_PUSH) &&
            SendWebSearchPush(Context, Query, GenerateSearchUri(Context, Query))) {
            ctx.AddAttention(ATTENTION_WEB_SEARCH_PUSH);
            return TResultValue();
        }

        return NothingFound(false /* addAttention */, TResultValue(), disableFacts);
    }

    if (hasAnswer && IsSmartSpeaker) {
        Context->GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SEARCH);
        Context->AddAttention(TStringBuf("search__nothing_found"), NSc::Null());
        Answer->Value = NSc::Null();
    }

    TContext::TSlot* slotChangeForm = ctx.GetSlot(CHANGE_FORM_SLOT);
    if (hasAnswer && IsSlotEmpty(slotChangeForm) &&
        (IsSmartSpeaker || Context->HasExpFlag("bass_search_irrelevant_response")))
    {
        Context->AddIrrelevantAttention(/* relevantIntent= */ TStringBuf("Search"));
    }

    return TResultValue();
}

TResultValue TSearchFormHandler::DoSetup(TSetupContext &ctx)
{
    bool isExperiment = ctx.HasExpFlag(EXPERIMENTAL_FLAG_SEARCH_SETUP);
    bool isSearch = ctx.FormName() == FORM_NAME;
    bool isQuasar = ctx.MetaClientInfo().IsQuasar();

    if (!(isExperiment && isSearch && isQuasar)) {
        return TResultValue();
    }

    TContext::TSlot* slotQuery = ctx.GetSlot(QUERY_SLOT);
    if (IsSlotEmpty(slotQuery)) {
        return TError(TError::EType::INVALIDPARAM, "query");
    }

    Context = &ctx;
    Query = slotQuery->Value.GetString();
    TString newQuery = NormalizedQuery();
    if (newQuery) {
        Query = newQuery;
    }

    auto multiRequest = NHttpFetcher::WeakMultiRequest();
    auto handle = FetchSetupRequest<NSc::TValue>(THolder(new TSearchRequest(ctx, Query)), ctx, multiRequest);

    std::unique_ptr<IRequestHandle<NExternalSkill::TServiceResponse>> skillsRequestHandler =
        Context->HasExpFlag(EXPERIMENTAL_FLAG_ENABLE_SKILLS_DISCOVERY)
        ? FetchSetupRequest<NExternalSkill::TServiceResponse>(THolder(new TSkillsDiscoveryRequest(*Context, Query)), *Context, multiRequest)
        : nullptr;

    NSc::TValue searchResult;
    if (TResultValue err = handle->WaitAndParseResponse(&searchResult)) {
        return err;
    }

    if(skillsRequestHandler) {
        NExternalSkill::TServiceResponse answer;
        skillsRequestHandler->WaitAndParseResponse(&answer);
    }

    return TResultValue();
}

TResultValue TSearchFormHandler::NothingFound(bool addAttention, TResultValue defaultResult, bool disableFacts) {
    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_nothing_found");
    if (((IsSmartSpeaker && !Context->HasExpFlag(TStringBuf("disable_quasar_gc_instead_of_search"))) ||
        (disableFacts && Context->HasExpFlag(NAlice::NExperiments::GC_INSTEAD_FACTS_ON_ONEWORD))) &&
        !Context->Meta().IsBanned())
    {
        LOG(INFO) << "Change form to GC" << Endl;
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_gc");
        TGeneralConversationFormHandler::SetAsResponse(*Context);
        return Context->RunResponseFormHandler();
    }
    if (addAttention || disableFacts)
        Context->AddAttention(TStringBuf("search__nothing_found"), NSc::Null());

    if (disableFacts)
        return TResultValue();

    return defaultResult;
}

bool TSearchFormHandler::AddFactoid(NSc::TValue& searchResult) {
    NSc::TArray& wizplaces = searchResult["wizplaces"]["important"].GetArrayMutable();
    NSc::TArray& docs = searchResult["searchdata"]["docs"].GetArrayMutable();
    bool summarizationTrigger = ShouldAnswerWithSummarization(searchResult);

    bool found =    AddCalculator(wizplaces)        || AddCalculator(docs)
                 || AddSuggestFact(wizplaces, true, summarizationTrigger)
                 || AddSuggestFact(docs, false, summarizationTrigger)
                 || AddRichFact(wizplaces, true)
                 || AddRichFact(docs, false)
                 || AddEntityFact(wizplaces)        || AddEntityFact(docs)
                 || AddWikipediaFact(wizplaces)     || AddWikipediaFact(docs) // no pictures
                 || AddCaloriesFact(wizplaces)      || AddCaloriesFact(docs)
                 || AddUnitsConverter(docs)
                 || AddTimeDifference(docs)
                 || AddDistanceFact(wizplaces)      || AddDistanceFact(docs)
                 || AddTableFact(wizplaces)         || AddTableFact(docs)
                 || AddAutoRegion(docs)
                 || AddZipCode(docs)
                 || AddSportLivescore(wizplaces, true) || AddSportLivescore(docs, false);

    LOG_IF(RESOURCES, found) << "Found factoid" << Endl;
    return found;
}

bool TSearchFormHandler::AddFactoidAppHost(NSc::TValue& appHostResult) {
    bool found =    AddCalculatorAppHost(appHostResult)
                 || AddSuggestFactAppHost(appHostResult)
                 || AddEntityFactAppHost(appHostResult)
                 || AddCaloriesFactAppHost(appHostResult)
                 || AddUnitsConverterAppHost(appHostResult)
                 || AddTimeDifferenceAppHost(appHostResult)
                 || AddAutoRegionAppHost(appHostResult);

    LOG_IF(RESOURCES, found) << "Found app_host factoid" << Endl;
    return found;
}

void TSearchFormHandler::AddFactoidPhone(const NSc::TValue& snippet, NSc::TValue* factoid, bool addSuggest) const {
    TStringBuf phone = snippet.TrySelect("/data/phone").GetString();
    if (!phone.empty() && Context->MetaClientInfo().IsTouch()) {
        (*factoid)["phone"].SetString(phone);
        (*factoid)["phone_uri"] = GeneratePhoneUri(Context->MetaClientInfo(), phone);
        if (addSuggest)
            Context->AddSuggest(TStringBuf("search__phone_call"));
    }
}

bool IsBadRelatedFactQuery(const TStringBuf& query) {
    // TODO: do this in more general way when preparing related facts dictionary.
    if (query.Contains("имя")
        || query.Contains("имени")
        || query.Contains("рецепт"))
    {
        return true;
    }

    if (!query.Contains(' ')) {
        return true;
    }

    return false;
}

void TSearchFormHandler::AddRelatedFactPromo(const NSc::TValue& snippet, NSc::TValue& factoid) const {
    Y_ASSERT(Context != nullptr);

    // Check if enabled.
    if (!IsSmartSpeaker
        || Context->HasExpFlag(NAlice::NExperiments::DISABLE_RELATED_FACTS_PROMO))
    {
        return;
    }

    // Get current query.
    TStringBuf currentQuery;
    {
        const TContext::TSlot* const slot = Context->GetSlot(QUERY_SLOT);
        if (slot != nullptr) {
            currentQuery = slot->Value.GetString();
        }
    }

    // Load history.
    THashSet<TStringBuf> bayanQueries;
    {
        const TContext::TSlot* const slot = Context->GetSlot("related_queries_history");
        if (slot != nullptr) {
            const NSc::TArray& values = slot->Value.GetArray();
            bayanQueries.reserve(values.size() + 2);

            for (const NSc::TValue& query: values) {
                bayanQueries.insert(query.GetString());
            }
        }
    }

    // Check probability.
    bool isEnabledByProba = false;
    {
        double proba = 1.0;

        const TMaybe<TStringBuf> probaStr =
            Context->GetValueFromExpPrefix(NAlice::NExperiments::RELATED_FACTS_PROMO_PROBA_PREFIX);
        if (!probaStr.Empty()) {
            proba = FromStringWithDefault(probaStr.GetRef(), 1.0);
        }

        const double rnd = Context->GetRng().RandomDouble();
        isEnabledByProba = (rnd <= proba);
    }

    // Find related query.
    TStringBuf relatedQuery;
    if (isEnabledByProba
        && !IsBadRelatedFactQuery(currentQuery))
    {
        for (const NSc::TValue& relatedFact : snippet["related_facts"].GetArray()) {
            const TStringBuf query = relatedFact["query"].GetString();
            if (!query.empty()
                && !bayanQueries.contains(query)
                && !IsBadRelatedFactQuery(query))
            {
                    relatedQuery = query;
                    break;
            }
        }
    }

    // Add related query.
    if (!relatedQuery.empty()) {
        factoid["related_query"] = relatedQuery;
        Context->CreateSlot("related_query", "string", false, NSc::TValue(relatedQuery));
    }

    // Update history.
    {
        if (!currentQuery.empty()) {
            bayanQueries.insert(currentQuery);
        }

        if (!relatedQuery.empty()) {
            bayanQueries.insert(relatedQuery);
        }

        TVector<TStringBuf> newHistoryOrdered;
        newHistoryOrdered.reserve(bayanQueries.size());
        for (const TStringBuf& query: bayanQueries) {
            newHistoryOrdered.push_back(query);
        }
        Sort(newHistoryOrdered);

        NSc::TValue newHistoryJson;
        newHistoryJson.SetArray();
        for (const TStringBuf& query: newHistoryOrdered) {
            newHistoryJson.Push(query);
        }
        Context->CreateSlot("related_queries_history", "related_queries_history", false, newHistoryJson);
    }
}

bool TSearchFormHandler::AddRichFact(NSc::TArray& docs, bool important) {
    Y_ASSERT(Context);

    ui8 maxPos = static_cast<ui8>(important ? 5 : 1);
    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("rich_fact"), maxPos, ESS_CONSTRUCT);

    if (!r.Snippet) {
        return false;
    }

    return AddSuggestFactImpl(r.Snippet);
}

bool TSearchFormHandler::AddSuggestFact(NSc::TArray& docs, bool important, bool summarizationTrigger) {
    Y_ASSERT(Context);

    ui8 maxPos = static_cast<ui8>(important ? 5 : 1);
    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("suggest_fact"), maxPos, ESS_CONSTRUCT);

    if (!r.Snippet) {
        return false;
    }

    if (Context->HasExpFlag(NAlice::NExperiments::SUMMARIZATION_TRIGGER_FACTSNIP)
        && (*r.Snippet)["source"].GetString() == "fact_snippet"
        && !summarizationTrigger)
    {
        return false;
    }

    return AddSuggestFactImpl(r.Snippet);
}

bool TSearchFormHandler::AddSuggestFactAppHost(const NSc::TValue& appHostResult) {
    if (appHostResult.IsNull()) {
        return false;
    }

    return AddSuggestFactImpl(&appHostResult);
}

bool TSearchFormHandler::AddSuggestFactImpl(const NSc::TValue* snippet) {
    if (snippet == nullptr) {
        return false;
    }

    TStringBuf subtype = (*snippet)["subtype"].GetString();
    TString text, tts;
    if (subtype.empty() || TStringBuf("suggest_fact") == subtype || TStringBuf("wikipedia_fact") == subtype) {
        NSc::TValue node = (*snippet)["text"];
        if (node.IsString()) {
            text = RemoveHiLight(node.GetString());
        } else if (node.IsArray()) {
            TStringBuilder sb;
            for (NSc::TValue n : node.GetArray()) {
                if (n.IsString()) {
                    if (!sb.empty())
                        sb << "\n";
                    sb << RemoveHiLight(n.GetString());
                }
            }
            text = sb;
        }
    } else if (TStringBuf("quotes") == subtype) {
        text = (*snippet)["text"].GetString();
        if (text.empty()) {
            NSc::TValue node = snippet->TrySelect("/text[0][0]");
            if (!node.IsNull()) {
                TStringBuf currency = node["currency"].GetString();
                TString amount = node["text"].ForceString();
                if (currency && amount) {
                    text = TStringBuilder() << amount << ' ' << currency;
                }
            }
        }
    } if (TStringBuf("list_featured_snippet") == subtype && !Context->HasExpFlag(NAlice::NExperiments::DISABLE_FACT_LISTS)) {
        const TString& prefix = RemoveHiLight((*snippet)["text"].GetString());
        bool isOrdered = (*snippet)["list_type"].GetString() == TStringBuf("ol");

        const auto& jsonItems = (*snippet)["list_items"].GetArray();
        TVector<TString> items;
        items.reserve(jsonItems.size());
        for (const auto& json: jsonItems) {
            items.push_back(RemoveHiLight(json.GetString()));
        }
        text = JoinListFact(prefix, items, isOrdered);
        tts = JoinListFact(prefix, items, isOrdered, /*isTts = */ true);
    }

    if (text.empty()) {
        LOG(WARNING) << "Unsupported suggest_fact: " << snippet->ToJson() << Endl;
        return false;
    }

    const TStringBuf source = (*snippet)["source"].GetString();

    const TStringBuf url = (*snippet)["url"].GetString();
    const TStringBuf hostName = NSerp::GetHostName(*snippet);

    auto voiceInfo = NSerp::GetFilteredVoiceInfo(*snippet, Tld, *Context);
    voiceInfo["read_source_before_text"] = ShouldReadSourceBeforeText(source, hostName)
        && !Context->HasExpFlag(NAlice::NExperiments::VOICE_FACTS_SOURCE_AFTER_TEXT);

    tts = tts.empty() ? voiceInfo.TrySelect("text").GetString() : tts;

    if (!url.empty())
        Context->AddSuggest(TStringBuf("search__factoid_src"));

    if (Context->ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        if (TStringBuf article = (*snippet)["article_name"].GetString()) {
            card["title"] = article;
        } else if (TString title = RemoveHiLight((*snippet)["headline"].GetString())) {
            card["title"] = title;
        } else if (TStringBuf question = (*snippet)["question"].GetString()) {
            card["title"] = question;
        }
        card["text"] = text;
        card["tts"] = tts;
        card["url"] = url;
        AddFactoidPhone(*snippet, &card, false); // phone_call sugges is already added
        NSc::TValue gallery0 = snippet->TrySelect("gallery[0]");
        if (gallery0.IsNull() || !AddDivCardImage(gallery0["thmb_href"].ForceString(),
                                                  static_cast<ui16>(gallery0["thmb_w_orig"].ForceIntNumber(0)),
                                                  static_cast<ui16>(gallery0["thmb_h_orig"].ForceIntNumber(0)),
                                                  &card)) {
            AddDivCardImage(CreateAvtarIdImageSrc(*snippet, 122, 122), 122, 122, &card);
        }
        card["snippet_type"] = TStringBuf("suggest_fact");
        card["serp_data"] = *snippet;
        card["source"] = source;
        card["hostname"] = hostName;
        Context->AddDivCardBlock("search_fact", card);
    }

    NSc::TValue factoid;
    if (Context->HasExpFlag(EXPERIMENTAL_FLAG_ENABLE_FACTOID_CHILD_ANSWER)) {
        const bool isChildUtterance = (*snippet)["is_child_utterance"].GetBool(false);
        factoid["child_search"]["is_child_utterance"].SetBool(isChildUtterance);
        const bool isChildAnswer = (*snippet)["is_child_answer"].GetBool(false);
        factoid["child_search"]["is_child_answer"].SetBool(isChildAnswer);
    }
    factoid["text"] = text;
    factoid["url"] = url;
    factoid["voice_info"] = voiceInfo;
    factoid["tts"] = tts; // TODO(@micyril) Remove it once VINS uses new format
    factoid["source"] = source;

    if (!hostName.empty()) {
        factoid["hostname"] = hostName;
    }

    AddFactoidPhone(*snippet, &factoid, true);
    AddRelatedFactPromo(*snippet, factoid);

    Answer->Value["factoid"] = factoid;

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_suggest_fact");
    Context->GetAnalyticsInfoBuilder().AddObject("factoid_source", "factoid source", TString(source));
    return true;
}

bool TSearchFormHandler::AddSummarization(const NSc::TValue& summarization, ::NSearch::NAlice::NSerpSummarizer::TServerResponse& response){
    if (!summarization.Has("Facts.Snippets") || !summarization.Has("Facts.AnswersUrls")) {
        LOG(INFO) << "Summarization sync request failed: required fields are absent" << Endl;
        return false;
    }

    NSearch::NAlice::NSerpSummarizer::TSummarizerInput requestProto;

    requestProto.SetQuery(Query);

    const NSc::TValue snippets = NSc::TValue::FromJson(Base64Decode(summarization["Facts.Snippets"]));
    const NSc::TValue urls = NSc::TValue::FromJson(Base64Decode(summarization["Facts.AnswersUrls"]));
    const NSc::TValue headlines = NSc::TValue::FromJson(Base64Decode(summarization["Facts.AnswersHeadlines"]));
    const auto& factSnippets = snippets.GetArray();
    const auto& factUrls = urls.GetArray();
    const auto& factHeadlines = headlines.GetArray();

    for (size_t i = 0; i < factSnippets.size(); i++) {
        const auto& snip = factSnippets[i];
        const auto& url = factUrls[i];
        auto* doc = requestProto.AddSerpResults();
        for (const NSc::TValue& s : snip.GetArray()) {
            doc->SetSnippet(TString(s.GetString()));
            break;
        }
        doc->SetUrl(TString(url.GetString()));
        if (i < factHeadlines.size()){
            doc->SetHeadline(TString(factHeadlines[i].GetString()));
        }
    }

    auto request = Context->GetSources().SerpSummarization().Request();
    request->SetBody(requestProto.SerializeAsString(), NAlice::NHttpMethods::POST);
    request->SetContentType(NAlice::NContentTypes::APPLICATION_PROTOBUF);
    request->AddHeader(NAlice::NNetwork::HEADER_X_ALICE_CLIENT_REQID, TLogging::ReqInfo.Get().ReqId());
    request->AddHeader(NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_REQUEST, TStringBuf("1"));
    if (const auto srcrwr = Context->GetValueFromExpPrefix(NAlice::NExperiments::SERP_SUMMARIZATION_SRCRWR)) {
        request->AddCgiParam("srcrwr", srcrwr.GetRef());
    }

    NHttpFetcher::TResponse::TRef resp = request->Fetch()->Wait();
    if (resp->IsError()) {
        LOG(DEBUG) << "Error fetching summarization service: " << resp->GetErrorText() << Endl;
        return false;
    };

    // Parse response.
    if (!response.ParseFromString(resp->Data)) {
        LOG(ERR) << "Error parsing summarization response." << Endl;
        return false;
    }

    if (response.GetFailed()) {
        LOG(DEBUG) << "Summarization failed to find answer." << Endl;
        return false;
    }
    return true;
}


bool TSearchFormHandler::AddSummarizationAsync(const NSc::TValue& summarization, ::NSearch::NAlice::NSerpSummarizer::TServerResponse& response){
    const TStringBuf reqId = summarization["request_id"];
    const TStringBuf uri = summarization["uri"];
    if (reqId.empty() || uri.empty()) {
        return false;
    }

    // Prepare request.
    NSearch::NAlice::NSerpSummarizer::TAsyncResult requestProto;
    requestProto.SetRequestId(reqId.data());

    auto request = Context->GetSources().SerpSummarizationAsync(uri).Request();
    request->SetBody(requestProto.SerializeAsString(), NAlice::NHttpMethods::POST);
    request->SetContentType(NAlice::NContentTypes::APPLICATION_PROTOBUF);
    request->AddHeader(NAlice::NNetwork::HEADER_X_ALICE_CLIENT_REQID, TLogging::ReqInfo.Get().ReqId());

    // Fetch request sequentially because we are sure summarization will be the final answer.
    NHttpFetcher::TResponse::TRef resp = request->Fetch()->Wait();
    if (resp->IsError()) {
        LOG(DEBUG) << "Error fetching summarization service: " << resp->GetErrorText() << Endl;
        return false;
    };

    // Parse response.
    if (!response.ParseFromString(resp->Data)) {
        LOG(ERR) << "Error parsing summarization response." << Endl;
        return false;
    }
    if (response.GetFailed()) {
        LOG(DEBUG) << "Summarization failed to find answer." << Endl;
        return false;
    }
    return true;
}


bool TSearchFormHandler::AddSummarizationAnswer(const NSc::TValue& summarization)
{
    Y_ASSERT(Context);

    ::NSearch::NAlice::NSerpSummarizer::TServerResponse response;

    if (!AddSummarizationAsync(summarization, response) && !AddSummarization(summarization, response)) {
        return false;
    }
    // Construct fact data.
    NSc::TValue serpData;
    serpData["text"] = response.GetOutput().GetSummary();
    serpData["type"] = "suggest_fact";
    serpData["source"] = NSerp::SUMMARIZATION;
    serpData["voiceInfo"]["ru"][0]["text"] = response.GetOutput().GetSummary();
    if (!response.GetOutput().GetVoicedSource().empty()) {
        serpData["voiceInfo"]["ru"][0]["source"] = response.GetOutput().GetVoicedSource();
    } else {
        serpData["voiceInfo"]["ru"][0]["trusted_source"] = false;
    }
    if (!response.GetOutput().GetUrl().empty()) {
        serpData["url"] = response.GetOutput().GetUrl();
    }

    // Stub for OTBET-89
    {
        NUri::TUri uri;
        NUri::TState::EParsed uriResult = uri.Parse(
            serpData["url"],
            NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible);

        if (uriResult == NUri::TState::EParsed::ParsedOK) {
            serpData["path"]["items"][0]["text"] = uri.GetField(NUri::TUri::TField::FieldHost);
        }
    }

    // Interpret as suggest fact wizard.
    return AddSuggestFactImpl(&serpData);
}

bool TSearchFormHandler::ShouldAnswerWithSummarization(const NSc::TValue& searchResult) const
{
    Y_ASSERT(Context);
    // Summarization trigger (besides experiments and other answers priority).
    if (!(IsSmartSpeaker
        || Context->MetaClientInfo().IsTvDevice()
        || Context->HasExpFlag(NAlice::NExperiments::ENABLE_ALL_PLATFORMS_HOLLYWOOD_SUMMARIZATION))
        || Context->HasExpFlag(NAlice::NExperiments::DISABLE_SERP_SUMMARIZATION))
    {
        LOG(DEBUG) << "Disabling summarization because of app." << Endl;
        return false;
    }

    const NSc::TValue& summarization = searchResult[NSerp::SUMMARIZATION];
    bool queryIsGood = summarization["Facts.SummarizationQueryIsGood"].GetString() == "1";

    // Condition 1: query score should be good enough if it is present in experiment.
    TMaybe<TStringBuf> queryThresholdStr = Context->GetValueFromExpPrefix(NAlice::NExperiments::SERP_SUMMARIZATION_QUERY_MX_PREFIX);
    if (queryThresholdStr.Defined()) {
        LOG(DEBUG) << "Got summarization query threshold exp: " << queryThresholdStr << Endl;
        const double queryThreshold = FromStringWithDefault(queryThresholdStr.GetRef(), 0.0);
        const double queryScore = FromStringWithDefault(summarization["Facts.SummarizationQueryMx"].GetString(), 0.0);
        queryIsGood = queryScore > queryThreshold;
    }

    // Condition 2: we should have SummarizationQueryIsGood flag.
    if (!queryIsGood) {
        LOG(DEBUG) << "Disabling summarization because query is bad." << Endl;
        return false;
    }

    // Condition 3: query is not commercial.
    if (IsTouch) {
        // We simply know that there are banners.
        if (NDirectGallery::DirectItemsCount(searchResult) > 0) {
            LOG(DEBUG) << "Disabling summarization because of ads." << Endl;
            return false;
        }

        // It is high CM2 value
        const double cmThreshold = Context->GetConfig()->Vins()->SerpSummarizationAsync()->CM2TouchThreshold();
        const TStringBuf& relev = searchResult["wizard"]["relev"].GetString();
        // Extract cm2 from wizard/relev string.
        static constexpr TStringBuf CM2 = "cm2=";
        TStringBuf left;
        TStringBuf right;
        if (relev.TrySplit(CM2, left, right)){
            double score = FromStringWithDefault(right.NextTok(";"), 0.0);
            if (score > cmThreshold) {
                LOG(DEBUG) << "Disabling summarization because query is commercial. cm2=" << score << Endl;
                return false;
            }
        } else {
            LOG(DEBUG) << "cm2= property is not found, enabling summarization."<< Endl;
            return true;
        }
    }

    // Condition 4: query is not politota.
    if (searchResult["wizard"]["rearr"].GetString().Contains("wizdetection_politota=1")) {
        LOG(DEBUG) << "Disabling summarization because query is politota." << Endl;
        return false;
    }

    return true;
}

bool TSearchFormHandler::AddWikipediaFact(NSc::TArray& docs) {
    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("wikipedia_fact"), 1, ESS_CONSTRUCT);
    if (!r)
        return false;

    const TString text = RemoveHiLight((*r.Snippet)["text"].GetString());
    const TStringBuf url = (*r.Snippet)["url"].GetString();
    auto voiceInfo = NSerp::GetFilteredVoiceInfo(*r.Snippet, Tld, *Context);
    TStringBuf tts = voiceInfo.TrySelect("text").GetString();
    TStringBuf source = (*r.Snippet)["source"].GetString();

    if (!url.empty())
        Context->AddSuggest(TStringBuf("search__factoid_src"));

    if (Context->ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        card["text"] = text;
        card["url"] = url;
        card["tts"] = tts;
        AddFactoidPhone(*r.Snippet, &card, false);
        // no image
        card["snippet_type"] = TStringBuf("wikipedia_fact");
        card["serp_data"] = *r.Snippet;
        card["source"] = source;
        Context->AddDivCardBlock("search_fact", card);
    }

    NSc::TValue factoid;
    factoid["text"] = text;
    factoid["url"] = url;
    factoid["tts"] = tts; // TODO(@micyril) Remove it once VINS uses new format
    factoid["voice_info"] = voiceInfo;
    factoid["source"] = source;
    AddFactoidPhone(*r.Snippet, &factoid, true);
    Answer->Value["factoid"] = factoid;

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_wiki_fact");
    return true;
}

bool TSearchFormHandler::AddEntityFact(NSc::TArray& docs) {
    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("entity-fact"), 1, ESS_CONSTRUCT);
    if (!r)
        return false;

    return AddEntityFactImpl(r.Snippet);
}

bool TSearchFormHandler::AddEntityFactAppHost(const NSc::TValue& appHostResult) {
    if (appHostResult.IsNull()) {
        return false;
    }

    return AddEntityFactImpl(&appHostResult);
}

bool TSearchFormHandler::AddEntityFactImpl(const NSc::TValue* snippet) {
    if (snippet == nullptr) {
        return false;
    }
    TStringBuf source = (*snippet)["source"].GetString();

    TStringBuilder text;
    for (const NSc::TValue& item : snippet->TrySelect("/requested_facts/item[0]/value").GetArray()) {
        if (!text.empty())
            text << ", ";
        text << item["text"].GetString();
    }
    if (text.empty()) {
        LOG(ERR) << "Cant create factoid for " << snippet->ToJsonSafe() << Endl;
        return false;
    }

    const TStringBuf url = snippet->TrySelect("/base_info/source/url").GetString();
    auto voiceInfo = NSerp::GetFilteredVoiceInfo(*snippet, Tld, *Context);
    TStringBuf tts = voiceInfo.TrySelect("text").GetString();

    if (!url.empty())
        Context->AddSuggest(TStringBuf("search__factoid_src"));

    if (Context->ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        if (TStringBuf title = snippet->TrySelect("/requested_facts/item[0]/name").GetString()) {
            card["title"] = title;
        }
        card["text"] = text;
        card["url"] = url;
        card["tts"] = tts;
        AddFactoidPhone(*snippet, &card, false);
        if (TStringBuf avatar = snippet->TrySelect("/base_info/image/avatar").GetString()) {
            AddDivCardImage(ToString(avatar), 120, 120, &card);
        }
        card["snippet_type"] = TStringBuf("entity-fact");
        card["serp_data"] = *snippet;
        card["source"] = source;
        card["hostname"] = NSerp::GetHostName(*snippet);
        Context->AddDivCardBlock("search_fact", card);
    }

    NSc::TValue factoid;
    factoid["text"] = text;
    factoid["url"] = url;
    factoid["tts"] = tts; // TODO(@micyril) Remove it once VINS uses new format
    factoid["voice_info"] = voiceInfo;
    factoid["source"] = source;
    AddFactoidPhone(*snippet, &factoid, true);
    Answer->Value["factoid"] = factoid;

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_entity_fact");
    return true;
}

bool TSearchFormHandler::AddCaloriesFact(NSc::TArray& docs) {
    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("calories_fact"), 1, ESS_CONSTRUCT);
    if (!r)
        return false;

    return AddCaloriesFactImpl(r.Snippet);
}

bool TSearchFormHandler::AddCaloriesFactAppHost(const NSc::TValue& appHostResult) {
    if (appHostResult.IsNull()) {
        return false;
    }

    return AddCaloriesFactImpl(&appHostResult);
}

bool TSearchFormHandler::AddCaloriesFactImpl(const NSc::TValue* snippet) {
    if (snippet == nullptr) {
        return false;
    }

    TStringBuf source = (*snippet)["source"].GetString();

    const TString text = ToString(snippet->TrySelect("/data/calories1").GetString());
    auto voiceInfo = NSerp::GetFilteredVoiceInfo(*snippet, Tld, *Context);
    TStringBuf tts = voiceInfo.TrySelect("text").GetString();
    const TStringBuf url = (*snippet)["url"].GetString();

    if (!url.empty())
        Context->AddSuggest(TStringBuf("search__factoid_src"));

    if (Context->ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        if (TString title = RemoveHiLight((*snippet)["headline"].GetString())) {
            card["title"] = title;
        }
        card["text"] = text;
        card["tts"] = tts;
        card["url"] = url;
        AddDivCardImage(CreateAvtarIdImageSrc(*snippet, 91, 91), 91, 91, &card);
        card["snippet_type"] = TStringBuf("calories_fact");
        card["serp_data"] = *snippet;
        card["source"] = source;
        card["hostname"] = NSerp::GetHostName(*snippet);
        Context->AddDivCardBlock("search_fact", card);
    }

    NSc::TValue factoid;
    factoid["text"] = text;
    factoid["tts"] = tts; // TODO(@micyril) Remove it once VINS uses new format
    factoid["voice_info"] = voiceInfo;
    factoid["url"] = url;
    factoid["source"] = source;
    Answer->Value["factoid"] = factoid;

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_calories_fact");
    return true;
}

bool TSearchFormHandler::AddDistanceFact(NSc::TArray& docs) {
    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("distance_fact"), 1, ESS_CONSTRUCT);
    if (!r)
        return false;

    const NSc::TValue& value = r.Snippet->TrySelect("/data/distance");
    if (!value.IsNumber()) {
        LOG(WARNING) << "Unexpected distance type: " << value.ToJsonSafe() << Endl;
        return false;
    }

    double distance = value.GetNumber();
    bool ru = Tld == TStringBuf("ru");
    TStringBuf prefix = ru ? TStringBuf("Примерно ") : TStringBuf("~ ");
    TStringBuf suffix;
    if (distance > 1000) {
        distance = std::round(distance / 1000.0);
        suffix = ru ? TStringBuf(" км") : TStringBuf(" km");
    } else if (distance > 1) {
        distance = std::round(distance);
        suffix = ru ? TStringBuf(" м") : TStringBuf(" m");
    } else if (distance > 0.01){
        distance = std::round(distance * 100.0);
        suffix = ru ? TStringBuf(" см") : TStringBuf(" cm");
    } else if (distance > 0.0001) {
        distance = std::round(distance * 1000.0);
        suffix = ru ? TStringBuf(" мм") : TStringBuf(" mm");
    } else {
        distance = distance * 1000000000;
        suffix = ru ? TStringBuf(" нм") : TStringBuf(" nm");
    }

    TString text = TStringBuilder() << prefix << ToString(distance) << suffix;
    TStringBuf source = (*r.Snippet)["source"].GetString();

    if (Context->ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        card["text"] = text;
        card["snippet_type"] = TStringBuf("distance_fact");
        card["serp_data"] = *r.Snippet;
        card["source"] = source;
        Context->AddDivCardBlock("distance_fact", card);
    }

    NSc::TValue factoid;
    factoid["text"] = text;
    factoid["source"] = source;
    Answer->Value["factoid"] = factoid;

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_distance_fact");
    return true;
}

bool TSearchFormHandler::AddUnitsConverter(NSc::TArray& docs) {
    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("units_converter"), 1, ESS_SNIPPETS_FULL);
    if (!r)
        return false;

    return AddUnitsConverterImpl(r.Snippet);
}

bool TSearchFormHandler::AddUnitsConverterAppHost(const NSc::TValue& appHostResult) {
    if (appHostResult.IsNull()) {
        return false;
    }

    return AddUnitsConverterImpl(&appHostResult);
}

bool TSearchFormHandler::AddUnitsConverterImpl(const NSc::TValue* snippet) {
    if (snippet == nullptr) {
        return false;
    }

    TStringBuf source = (*snippet)["source"].GetString();

    auto voiceInfo = NSerp::GetFilteredVoiceInfo((*snippet)["data"], Tld, *Context);
    TStringBuf tts = voiceInfo.TrySelect("text").GetString();
    if (tts.empty()) {
        LOG(WARNING) << "skip units_converter due to the lack of tts: " << snippet->ToJsonSafe() << Endl;
        return false;
    }

    if (Context->ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        card["text"] = tts;
        card["tts"] = tts;
        card["snippet_type"] = TStringBuf("units_converter");
        //card["serp_data"] = *snippet; // is too big
        card["source"] = source;
        Context->AddDivCardBlock("search_fact", card);
    }

    NSc::TValue factoid;
    factoid["text"] = tts;
    factoid["tts"] = tts; // TODO(@micyril) Remove it once VINS uses new format
    factoid["voice_info"] = voiceInfo;
    factoid["source"] = source;
    Answer->Value["factoid"] = factoid;

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_units_converter");
    return true;
}

bool TSearchFormHandler::AddTimeDifference(NSc::TArray& docs) {
    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("time"), 1, ESS_SNIPPETS_FULL);
    if (!r)
        return false;

    return AddTimeDifferenceImpl(r.Snippet);
}

bool TSearchFormHandler::AddTimeDifferenceAppHost(const NSc::TValue& appHostResult) {
    if (appHostResult.IsNull()) {
        return false;
    }

    return AddTimeDifferenceImpl(&appHostResult);
}

bool TSearchFormHandler::AddTimeDifferenceImpl(const NSc::TValue* snippet) {
    if (snippet == nullptr) {
        return false;
    }

    if ((*snippet)["format"].GetString() != TStringBuf("difference"))
        return false;

    TStringBuf date1 = (*snippet)["date1"].GetString();
    TStringBuf date2 = (*snippet)["date2"].GetString();
    if (!date1 || !date2)
        return false;

    TIso8601DateTimeParser parser1;
    TIso8601DateTimeParser parser2;
    if (!parser1.ParsePart(date1.data(), date1.size()) || !parser2.ParsePart(date2.data(), date2.size())) {
        LOG(WARNING) << "Cant parse ISO8601 datetime: " << date1 << ", " << date2 << Endl;
        return false;
    }

    i32 diff = parser2.GetDateTimeFields().ZoneOffsetMinutes - parser1.GetDateTimeFields().ZoneOffsetMinutes;
    TString text = FormatTimeDifference(diff, Tld);

    auto voiceInfo = NSerp::GetFilteredVoiceInfo(*snippet, Tld, *Context);
    TStringBuf tts = voiceInfo.TrySelect("text").GetString();
    TStringBuf source = (*snippet)["source"].GetString();

    if (Context->ClientFeatures().SupportsDivCards()) {
        // todo: st/DIALOG-897
        NSc::TValue card;
        card["text"] = text;
        card["tts"] = tts;
        card["snippet_type"] = TStringBuf("time");
        card["serp_data"] = *snippet;
        card["source"] = source;
        Context->AddDivCardBlock("search_fact", card);
    }

    NSc::TValue factoid;
    factoid["text"] = text;
    factoid["tts"] = tts; // TODO(@micyril) Remove it once VINS uses new format
    factoid["voice_info"] = voiceInfo;
    factoid["source"] = source;
    Answer->Value["factoid"] = factoid;

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_time_diff");
    return true;
}

bool TSearchFormHandler::AddTableFact(NSc::TArray& docs) {
    if (!Context->HasExpFlag("table_fact"))
        return false;

    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("table_fact"), 1, ESS_CONSTRUCT);
    if (!r)
        return false;

    if (Context->ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        card["snippet_type"] = TStringBuf("table_fact");
        card["serp_data"] = *r.Snippet;
        card["source"] = (*r.Snippet)["source"].GetString();
        Context->AddDivCardBlock("search_fact", card);

        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_table_fact");
        return true;
    }

    return false;
}

bool TSearchFormHandler::AddAutoRegion(NSc::TArray& docs) {
    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("auto_regions"), 1, ESS_SNIPPETS_FULL);
    if (!r)
        return false;

    return AddAutoRegionImpl(r.Snippet);
}

bool TSearchFormHandler::AddAutoRegionAppHost(const NSc::TValue& appHostResult) {
    if (appHostResult.IsNull()) {
        return false;
    }

    return AddAutoRegionImpl(&appHostResult);
}

bool TSearchFormHandler::AddAutoRegionImpl(const NSc::TValue* snippet) {
    if (snippet == nullptr) {
        return false;
    }

    TStringBuf name = (*snippet)["region"]["name"][Tld]["nominative"].GetString();

    if (Context->ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        card["text"] = name;
        card["snippet_type"] = TStringBuf("auto_regions");
        card["serp_data"] = *snippet;
        Context->AddDivCardBlock("search_fact", card);
    }

    NSc::TValue factoid;
    factoid["text"] = name;
    Answer->Value["factoid"] = factoid;

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_auto_regions");
    return true;
}

bool TSearchFormHandler::AddZipCode(NSc::TArray& docs) {
    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("post_indexes"), 1, ESS_SNIPPETS_FULL);
    if (!r)
        return false;

    TStringBuf name = (*r.Snippet)["obj"]["index"].GetString();

    if (Context->ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        card["text"] = name;
        card["snippet_type"] = TStringBuf("post_indexes");
        card["serp_data"] = *r.Snippet;
        Context->AddDivCardBlock("search_fact", card);
    }

    NSc::TValue factoid;
    factoid["text"] = name;
    Answer->Value["factoid"] = factoid;

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_post_indexes");
    return true;
}

bool TSearchFormHandler::AddSportLivescore(NSc::TArray& docs, bool wizplace) {
    auto fn = [this] (const NSc::TValue& snippet) {
        auto voiceInfo = NSerp::GetFilteredVoiceInfo(snippet["data"], Tld, *Context);
        if (TStringBuf text = voiceInfo.TrySelect("text").GetString()) {
            NSc::TValue factoid;
            factoid["text"] = text;
            factoid["tts"] = text; // TODO(@micyril) Remove it once VINS uses new format
            factoid["voice_info"] = voiceInfo;
            factoid["source"] = "sport/livescore";
            Answer->Value["factoid"] = factoid;
            Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_sport_livescore");
            return true;
        }
        return false;
    };

    if (wizplace) {
        for (NSc::TValue& snippet : docs) {
            TStringBuf type = snippet["type"].GetString();
            TStringBuf subtype = snippet["subtype"].GetString();
            if ((type == TStringBuf("sport/livescore") ||
                 type == TStringBuf("sport/tournament") ||
                 type == TStringBuf("special/event") &&
                    (subtype.StartsWith("chmf2018") ||
                     subtype == TStringBuf("football") ||
                     subtype == TStringBuf("hockey") ||
                     subtype.StartsWith("sport/")) // All new sports will have subtype with prefix "sport/"
                ) // SPORTSERP-179
                && fn(snippet))
            {
                return true;
            }
        }
        return false;
    }

    const NSc::TValue& snippet = FindSnippet(docs[0], TStringBuf("sport/livescore"), ESS_SNIPPETS_FULL);
    return !snippet.IsNull() && fn(snippet);
}

bool TSearchFormHandler::AddObjectAsFact(NSc::TValue& doc) {
    const NSc::TValue& r = FindSnippet(doc, TStringBuf("entity_search"), ESS_SNIPPETS_FULL);
    if (r.IsNull())
        return false;

    const NSc::TValue& snippetData = r["data"];
    if (snippetData.IsNull())
        return false;

    if (snippetData.TrySelect("/display_options/show_as_fact").GetIntNumber(0) != 1)
        return false;

    const NSc::TValue& baseInfo = snippetData["base_info"];

    TStringBuf name = baseInfo["name"].GetString();
    TStringBuf titleOrName = baseInfo.Has("title") ? baseInfo["title"].GetString() : name;
    TStringBuf description = baseInfo["description"].GetString();
    auto voiceInfo = NSerp::GetFilteredVoiceInfo(snippetData, Tld, *Context);
    TStringBuf tts = voiceInfo.TrySelect("text").GetString();
    const TStringBuf url = baseInfo.TrySelect("/description_source/url").GetString();

    if (titleOrName.empty()) {
        LOG(ERR) << "OO without title: " << snippetData << Endl;
        return false;
    }

    if (name.empty()) {
        LOG(ERR) << "OO without name: " << snippetData << Endl;
        return false;
    }

    if (Context->ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        card["title"] = titleOrName;
        card["text"] = description;
        card["url"] = url;
        card["serp_url"] = GenerateSearchUri(Context, titleOrName, TCgiParameters(
                {std::make_pair(TString{"entref"}, TString{baseInfo["entref"].GetString()})})
        );
        card["tts"] = tts;
        if (TStringBuf avatar = baseInfo.TrySelect("/image/avatar").GetString()) {
            AddDivCardImage(ToString(avatar), 120, 120, &card);
        }
        Context->AddDivCardBlock("search_object", card);
    }

    NSc::TValue object;
    object["text"] = TStringBuilder() << titleOrName << TStringBuf(" — ") << description;
    object["tts"] = tts; // TODO(@micyril) Remove it once VINS uses new format
    object["voice_info"] = voiceInfo;
    if (url.size()) {
        object["url"] = url;
        Context->AddSuggest(TStringBuf("search__factoid_src"));
    }

    if (Context->HasExpFlag(NAlice::NExperiments::OBJECT_AS_FACT_LONG_TTS)
        && url.Contains("ru.wikipedia.org"))
    {
        // Replace original voiceInfo with full text.
        const TString longTts = TStringBuilder() << "По данным русской википедии: " << object["text"].GetString();
        object["tts"] = longTts;
        object["voice_info"]["text"] = longTts;
    }

    Answer->Value["object"] = std::move(object);
    Answer->Value["entity_name"].SetString(name);

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_object");
    return true;
}

NSc::TValue TSearchFormHandler::GetMusicSnippetData(NSc::TArray& docs) {
    TDocWithSnippet r = FindDocBySnippetType(docs, "YMUSIC", "musicplayer", MAX_MEDIA_WIZARD_POS, ESS_SNIPPETS_FULL);
    if (!r)
        return NSc::TValue();

    NSc::TValue snippet = *r.Snippet;

    // delete too big unused field
    snippet.Delete("track_lyrics");

    return snippet;
}

NSc::TValue TSearchFormHandler::GetVideoSnippetData(NSc::TArray& docs) {
    TDocWithSnippet r = FindDocBySnippetType(docs, "videowiz", MAX_MEDIA_WIZARD_POS, ESS_CONSTRUCT);
    if (!r) {
        return NSc::TValue();
    }

    NSc::TValue snippet = r.Snippet->Get("request_caps");
    return snippet;
}

bool TSearchFormHandler::AddMusicSuggest(NSc::TArray& docs) {
    const NSc::TValue snippet = GetMusicSnippetData(docs);
    if (snippet.IsNull())
        return false;

    NSc::TValue musicdata = NMusic::TSearchMusicHandler::CreateMusicDataFromSnippet(*Context, snippet);
    if (musicdata.IsDict() && musicdata.Has("url")) {
        Context->AddSuggest("search__musicplayer", musicdata);
        return true;
    }

    return false;
}

bool TSearchFormHandler::AddCalculator(NSc::TArray& docs) {
    TDocWithSnippet r = FindDocBySnippetType(docs, TStringBuf("calculator"), 10, ESS_SNIPPETS_FULL | ESS_FLAT);
    if (!r)
        return false;

    return AddCalculatorImpl(r.Snippet);
}

bool TSearchFormHandler::AddCalculatorAppHost(const NSc::TValue& appHostResult) {
    if (appHostResult.IsNull()) {
        return false;
    }

    return AddCalculatorImpl(&appHostResult);
}

bool TSearchFormHandler::AddCalculatorImpl(const NSc::TValue* snippet) {
    if (snippet == nullptr) {
        return false;
    }

    TStringBuf result = (*snippet)["result"].GetString();
    if (result.empty())
        return false;

    Answer->Value["calculator"] = result;

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_calculator");
    return true;
}

bool TSearchFormHandler::AddNav(NSc::TValue& doc) {
    if (IsSmartSpeaker || IsElariWatch || IsYaAuto)
        return false;

    const NSc::TValue& bno = FindSnippet(doc, TStringBuf("bno"), ESS_CONSTRUCT);
    if (bno.IsNull())
        return false;

    TString text;
    const NSc::TValue& generic = FindSnippet(doc, TStringBuf("generic"), ESS_SNIPPETS_MAIN);
    if (!generic.IsNull())
        text = RemoveHiLight(generic.TrySelect("passages[0]").GetString());
    if (text.empty())
        text = RemoveHiLight(generic["headline"].GetString());
    if (text.empty())
        text = RemoveHiLight(doc["doctitle"].GetString());

    if (text.empty()) {
        LOG(ERR) << "Cannot find text for bno: " << doc << Endl;
        return false;
    }

    TStringBuf app;
    if (IsAndroid)
        app = bno.TrySelect("/mobile_apps/gplay/id").GetString();
    else if (IsIos)
        app = bno.TrySelect("/mobile_apps/itunes/id").GetString();
    if (app) {
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "nav_bnoapp");
    }

    NSc::TValue result = NAlice::CreateNavigationBlock(text, TStringBuf(""), app, doc["url"].GetString(), Context->ClientFeatures());
    Answer->Value["nav"].Swap(result);

    Context->AddSuggest(TStringBuf("search__nav"));

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_bno");
    return true;
}

int TSearchFormHandler::FindTvProgramPosition(const NSc::TArray& docs) const {
    for (ui32 i = 0; i < docs.size(); ++i) {
        if (docs[i]["host"].GetString() == TStringBuf("tv.yandex.ru"))
            return i;
    }
    return -1;
}

int TSearchFormHandler::FindMarketPosition(const NSc::TArray& docs) const {
    for (int i = 0; i < MAX_MARKET_POS; ++i) {
        if (docs[i]["server_descr"].GetString() == TStringBuf("MARKET") // wizard
            || docs[i]["host"] == TStringBuf("market.yandex.ru") /* organic */) {
            return i;
        }
    }
    return -1;
}

bool TSearchFormHandler::AddTvProgram(NSc::TValue& doc) {
    if (IsSmartSpeaker || IsElariWatch)
        return false;

    if (doc["host"].GetString() == TStringBuf("tv.yandex.ru")) {
        TString text = RemoveHiLight(doc["doctitle"].GetString());
        TStringBuf url = doc["url"].GetString();
        NSc::TValue result = NAlice::CreateNavigationBlock(text, TStringBuf(""), TStringBuf(""), url, Context->ClientFeatures());
        Answer->Value["nav"].Swap(result);

        Context->AddSuggest(TStringBuf("search__nav"));

        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_tv");
        return true;
    }
    return false;
}

bool TSearchFormHandler::AddPreRenderedCard(const NSc::TValue& searchResult) {
    if (IsSmartSpeaker || IsElariWatch)
        return false;

    for (const NSc::TValue& item : searchResult.TrySelect("wizplaces/important").GetArray()) {
        const NSc::TValue& ad = item.TrySelect("data/assistant_data");
        if (ad.IsNull())
            continue;
        const NSc::TValue& divCard = ad["div_card"];
        TStringBuf text = ad["text"].GetString();
        if (text.empty()) {
            LOG(ERR) << "div card text is null" << Endl;
            continue;
        }
        TStringBuf tts = NSerp::GetVoiceTTS(item["data"], Tld);

        if (Context->ClientFeatures().SupportsDivCards()) {
            if (!divCard.IsNull())
                Context->AddPreRenderedDivCardBlock(divCard.Clone());
            NSc::TValue card;
            card["text"] = text;
            card["tts"] = !tts.empty() ? tts : text;
            Answer->Value["pre_rendered_card"].Swap(card);
        } else {
            // add as fact
            NSc::TValue factoid;
            factoid["text"] = text;
            factoid["tts"] = !tts.empty() ? tts : text;
            factoid["source"] = "pre_rendered_card";
            Answer->Value["factoid"] = factoid;
        }
        return true;
    }

    return false;
}

bool TSearchFormHandler::AddSerp(TStringBuf query, NSc::TArray&, bool addSerpSuggest) {
    if (IsSmartSpeaker || IsElariWatch || IsYaAuto)
        return false;

    NSc::TValue result;
    result["url"] = GenerateSearchUri(Context, query);
    Answer->Value["serp"] = std::move(result);

    if (addSerpSuggest)
        Context->AddSuggest(TStringBuf("search__serp"));

    return true;
}

bool AppendRelatedFacts(NSc::TValue& searchResult, NSc::TValue& output) {
    NSc::TArray& wizplaces = searchResult["wizplaces"]["important"].GetArrayMutable();
    NSc::TArray& docs = searchResult["searchdata"]["docs"].GetArrayMutable();

    TDocWithSnippet doc =
           FindDocBySnippetType(wizplaces, TStringBuf("suggest_fact"), 5, ESS_CONSTRUCT);
    if (!doc.Snippet) {
        doc = FindDocBySnippetType(docs, TStringBuf("suggest_fact"), 1, ESS_CONSTRUCT);
    }
    if (!doc.Snippet) {
        return false;
    }
    const NSc::TValue& snippet = *doc.Snippet;

    const NSc::TArray& relatedFactsArray = snippet.TrySelect("/related_facts").GetArray();
    if (relatedFactsArray.size() == 0) {
        return false;
    }
    for (const NSc::TValue& item : relatedFactsArray) {
        output.GetArrayMutable().push_back(item["query"].GetString());
    }

    LOG(DEBUG) << "Added related facts" << Endl;
    return true;
}

bool AppendRelatedSuggestQueries(NSc::TValue& searchResult, NSc::TValue& output) {
    const NSc::TArray& relatedSuggestArray = searchResult.TrySelect("/wizplaces/related").GetArray();
    for (const NSc::TValue& entry : relatedSuggestArray) {
        if (entry["type"] == TStringBuf("request_extensions")) {
            const NSc::TArray& relatedSuggestArray = entry.TrySelect("/items[0]/text").GetArray();
            for (const NSc::TValue& item : relatedSuggestArray) {
                output.GetArrayMutable().push_back(item.GetString());
            }

            LOG(DEBUG) << "Added related suggest queries" << Endl;
            return true;
        }
    }
    return false;
}

bool TSearchFormHandler::AddRelated(NSc::TValue& data, TContext* const ctx) {
    if (IsSmartSpeaker || IsElariWatch || IsYaAuto)
        return false;

    bool success = false;
    NSc::TValue related;
    if (!Context->HasExpFlag(NAlice::NExperiments::DISABLE_RELATED_FACTS_SUGGESTS)) {
        success |= AppendRelatedFacts(data, related);
    }
    success |= AppendRelatedSuggestQueries(data, related);

    for (const NSc::TValue& item : related.GetArray()) {
        NSc::TValue rel;
        rel["query"].SetString(item.GetString());
        ctx->AddSuggest(TStringBuf("search__see_also"), rel);
    }

    return success;
}

bool TSearchFormHandler::AddSearchStubs(NSc::TArray& docs) {
    bool hasAnswer = false;
    if (!Context->HasExpFlag("disable_search_stubs") && !IsSmartSpeaker && !IsElariWatch) {
        for (size_t i = 0, docsCount = MIN(docs.size(), 5); i < docsCount && !hasAnswer; ++i) {
            TStringBuf stub = ApplyStubs(docs[i], Stubs);
            hasAnswer = !stub.empty();
            if (!hasAnswer) {
                hasAnswer = AddObjectAsFact(docs[i]);
            }
        }
    } else {
        for (size_t i = 0, docsCount = MIN(docs.size(), 5); i < docsCount && !hasAnswer; ++i) {
            hasAnswer = AddObjectAsFact(docs[i]);
        }
    }
    return hasAnswer;
}

bool TSearchFormHandler::AddDivCardImage(const TString& src, ui16 w, ui16 h, NSc::TValue* card) const {
    if (src.size()) {
        NSc::TValue image;
        image["src"].SetString(src.StartsWith('/') ? TStringBuilder() << "https:" << src : src);
        image["w"] = w;
        image["h"] = h;
        (*card)["image"] = image;
        return true;
    }
    return false;
}

TString TSearchFormHandler::CreateAvtarIdImageSrc(const NSc::TValue& snippet, ui16 w, ui16 h) {
    if (TStringBuf id = snippet["avatar_id"].GetString()) {
        return TStringBuilder() << "https://avatars.mds.yandex.net/get-entity_search/" << id << "/S" << w << "x" << h;
    }
    return TString();
}

bool TSearchFormHandler::AddRelevantSkills(IRequestHandle<NExternalSkill::TServiceResponse>* skillsRequestHandler,
                                           NExternalSkill::EDiscoverySourceIntent sourceIntent) {
    if (!Context->ClientFeatures().SupportsDivCards())
        return false;

    TVector<const NExternalSkill::TSkillBadge> skills;
    skills = NExternalSkill::WaitDiscoveryResponses(skillsRequestHandler, *Context);

    if (skills.empty())
        return false;

    NSc::TValue data;
    for (const auto& badge : skills) {
        data.Push(badge.ToJson());
    }

    NSc::TValue card;
    card["text"] = "Возможно, Вас заинтересует";
    card["store_url"] = Context->GetConfig().DialogsStoreUrl().Get();
    card["items"] = data;
    card["source_intent"] = ToString(sourceIntent);

    if (!Context->HasExpFlag("not_change_form_skills_in_bass_search")) {
        // We need this change_form to correctly activate protocol Search
        TContext::TPtr newContext = Context->SetResponseForm(FORM_NAME_RELEVANT_SKILLS, false /* setCurrentFormAsCallback */);
        if (newContext) {
            auto* newAnswer = newContext->CreateSlot(SEARCH_RESULTS, SEARCH_RESULTS);
            *newAnswer = *Answer;
            Answer = newAnswer;
            auto* querySlot = newContext->CreateSlot(QUERY_SLOT, TStringBuf("string"));
            querySlot->Value = Query;
            Context = newContext.Get();
        }
    }

    Context->GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SERP);
    Context->AddTextCardBlock("search__skills_discovery");
    Context->AddAttention("skills_discovery");
    Context->AddDivCardBlock("relevant_skills", card);

    return true;
}

// static
void TSearchFormHandler::Register(THandlersMap* handlers, IGlobalContext& globalCtx) {
    static auto threadPool = CreateThreadPool(globalCtx.Config().SearchThreads());

    handlers->emplace(FORM_NAME, []() { return MakeHolder<TSearchFormHandler>(*threadPool); });
    handlers->emplace(FORM_NAME_ANAPHORIC, []() { return MakeHolder<TSearchFormHandler>(*threadPool); });
    handlers->emplace(FORM_NAME_ANAPHORIC_NEW, []() { return MakeHolder<TSearchFormHandler>(*threadPool); });
    handlers->emplace(FORM_NAME_SHOW_ON_MAP, []() {return MakeHolder<TSearchFormHandler>(*threadPool); });
    handlers->emplace(FORM_NAME_RELATED, []() {return MakeHolder<TSearchFormHandler>(*threadPool); });
}

void TSearchFormHandler::Init() {
    LoadBundledSearchStubs();
}

bool TSearchFormHandler::ApplyFixedAnswers() {
    if (!DisableChangeIntent && Context->MetaClientInfo().IsDesktop()) {
        // For Stroka we would like to preserve old semantic when if the query matches something
        // from Windows soft fixlist it should be navigation scenario.
        if (auto winApp = TNavigationFixList::Instance()->FindWindowsApp(Query)) {
            if (TNavigationFormHandler::SetAsResponse(*Context, false, Query)) {
                Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_nav");
                return !Context->RunResponseFormHandler();
            }
        }
    }

    NSc::TValue fixlistAnswer = TNavigationFixList::Instance()->Find(Query, *Context);
    if (fixlistAnswer.IsNull())
        return false;

    NSc::TValue block = NAlice::CreateNavBlock(fixlistAnswer, Context->ClientFeatures(), true);
    if (Y_UNLIKELY(block.IsNull()))
        return false;

    Answer->Value["nav"].Swap(block);

    Context->AddSuggest(TStringBuf("search__nav"));

    if (TStringBuf serpSuggestText = fixlistAnswer.TrySelect("suggests/serp/text").GetString())
        AddSerp(serpSuggestText, NSc::TValue().GetArrayMutable());

    return true;
}

bool TSearchFormHandler::TryToSwitchToTranslate(int translateWizardPos, int firstNonWebRes) const {
    if (!DisableChangeIntent && Context->HasExpFlag("translate") &&
        Context->HasExpFlag(EXPERIMENTAL_FLAG_ENABLE_SEARCH_SWITCH_TO_TRANSLATE) &&
        translateWizardPos >= 0 && translateWizardPos < MAX_TRANSLATE_WIZARD_POS &&
        (firstNonWebRes < 0 || translateWizardPos == firstNonWebRes)
    ) {
        TTranslateFormHandler::SetAsResponse(*Context, Query);
        Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_switch_to_translate");
        return true;
    }
    return false;
}

TStringBuf TSearchFormHandler::ApplyStubs(NSc::TValue& doc, const NSc::TValue& stubs) {
    if (stubs.IsNull())
        return TStringBuf("");

    TStringBuf host = doc["host"].GetString();
    const NSc::TValue& stub = stubs[host];
    if (stub.IsNull())
        return TStringBuf("");

    ESnippetSection snippetSection = ESS_NONE;
    for (const NSc::TValue& sectionNameNode : stub["sections"].GetArray()) {
        TStringBuf sectionName = sectionNameNode.GetString();
        if (TStringBuf("construct") == sectionName)
            snippetSection = snippetSection | ESS_CONSTRUCT;
        else if (TStringBuf("full") == sectionName)
            snippetSection = snippetSection | ESS_SNIPPETS_FULL;
        else if (TStringBuf("main") == sectionName)
            snippetSection = snippetSection | ESS_SNIPPETS_MAIN;
        else if (TStringBuf("pre") == sectionName)
            snippetSection = snippetSection | ESS_SNIPPETS_PRE;
        else if (TStringBuf("post") == sectionName)
            snippetSection = snippetSection | ESS_SNIPPETS_POST;
        else
            LOG(ERR) << "Unknown snippet section " << sectionName << Endl;
    }

    const NSc::TArray& snippetTypes = stub["types"].GetArray();
    const TStringBuf url = doc["url"].GetString();

    if (snippetTypes.empty()) {
        AddStub(stub, host, url);
        return stub["result_type"].GetString();
    } else {
        for (const NSc::TValue& snippetType : snippetTypes) {
            const NSc::TValue& snippet = FindSnippet(doc, snippetType.GetString(), snippetSection);
            if (!snippet.IsNull()) {
                AddStub(stub, host, url);
                return stub["result_type"].GetString();
            }
        }
    }

    return TStringBuf("");
}

void TSearchFormHandler::AddStub(const NSc::TValue& stub, TStringBuf host, TStringBuf url) {
    NSc::TValue result;
    result["service"] = host;
    if (url.StartsWith('/'))
        result["url"] = TStringBuilder() << "https:" << url;
    else
        result["url"] = url;
    Answer->Value[stub["result_type"].GetString()] = std::move(result);

    TStringBuf suggest = stub["suggest"].GetString();
    if (!suggest.empty())
        Context->AddSuggest(suggest);

    Y_STATS_INC_COUNTER_IF(!Context->IsTestUser(), "search_stub");
}

void TSearchFormHandler::LoadBundledSearchStubs() {
    LOG(INFO) << "Loading search_stubs.json" << Endl;

    TString content;
    if (!NResource::FindExact("search_stubs.json", &content))
        ythrow yexception() << "Unable to load built-in resource 'search_stubs.json'";

    NSc::TValue result;
    NSc::TValue json = NSc::TValue::FromJson(TStringBuf(content));
    for (const NSc::TValue& entry : json.GetArray()) {
        for (const NSc::TValue& host : entry["hosts"].GetArray()) {
            LOG(DEBUG) << "Search.Stubs: + '" << host.GetString() << "'" << Endl;
            result[host.GetString()] = entry;
        }
    }

    Stubs.Swap(result);

    LOG(INFO) << "search_stubs.json loaded" << Endl;
}

// static
TContext::TPtr TSearchFormHandler::SetAsResponse(TContext& ctx, bool callbackSlot, TStringBuf text) {
    TContext::TPtr newCtx = ctx.SetResponseForm(FORM_NAME, callbackSlot);
    if (!newCtx) {
        return nullptr;
    }

    if (!text) {
        text = newCtx->Meta().Utterance();
    }
    newCtx->CreateSlot(QUERY_SLOT, "string", true, NSc::TValue(text));
    newCtx->CreateSlot(CHANGE_FORM_SLOT, "bool", true, NSc::TValue(true));
    return newCtx;
}

// static
TContext::TPtr TSearchFormHandler::SetAsExternalSkillActivationResponse(TContext& ctx, bool callbackSlot, TStringBuf text) {
    TContext::TPtr newCtx = SetAsResponse(ctx, callbackSlot, text);
    newCtx->CreateSlot(NExternalSkill::ACTIVATION_SLOT_NAME, "string" /* type */, true /* optional */, NSc::TValue("") /* value */);
    return newCtx;
}

void FillSearchFactorsData(const NAlice::TClientInfo& clientInfo, const NSc::TValue& searchResult, NSc::TValue* factorsData) {
    Y_ASSERT(factorsData);

    bool isTouchSearch = NSerp::IsTouchSearch(clientInfo);

    NSc::TValue result = searchResult;
    NSc::TArray& docs = result["searchdata"]["docs"].GetArrayMutable();
    NSc::TArray& docsRight = result["searchdata"]["docs_right"].GetArrayMutable();

    { // Entity search
        i32 pos = FindEntitySearchPosition(isTouchSearch ? docs : docsRight);

        if (pos >= 0) {
            Y_ASSERT(pos < static_cast<i32>(docs.size()));

            const NSc::TValue& src = NSerpSnippets::FindSnippet(docs[pos], TStringBuf("entity_search"), NSerpSnippets::ESS_SNIPPETS_FULL);
            if (src["data"]["base_info"]["title"].GetString().length() > 0) {
                NSc::TValue& dst = (*factorsData)["snippets"]["entity_search"];
                dst["found"].SetBool(true);

                NScUtils::CopyField(src, dst, TStringBuf("data"), TStringBuf("base_info"), TStringBuf("ids"), TStringBuf("kinopoisk"));
                NScUtils::CopyField(src, dst, TStringBuf("data"), TStringBuf("base_info"), TStringBuf("title"));
                NScUtils::CopyField(src, dst, TStringBuf("data"), TStringBuf("base_info"), TStringBuf("type"));
            }
        }
    }

    // Wizards
    FillWizardsFactorsData(docs, WIZARD_MUSIC, factorsData);
    FillWizardsFactorsData(docs, WIZARD_VIDEO, factorsData);

    // Documents
    FillDocumentsFactorsData(docs, factorsData, /*maxDocuments*/ 10);
}

bool IsCommercialQuery(TContext& ctx, TStringBuf query) {
    const auto& wizardResponse = ctx.ReqWizard(query, ctx.UserRegion(), {});
    const auto& commercialMx = wizardResponse["rules"]["CommercialMx"]["commercialMx"];
    double cm2 = 0.0;
    bool parsed = TryFromString(commercialMx.GetString(), cm2);
    if (!parsed && !commercialMx.IsNull()) {
        LOG(ERR) << "Failed to extract CommercialMx value from wizard response " << wizardResponse["rules"]["CommercialMx"] << Endl;
    }
    return NAlice::NSkillDiscovery::IsCommercialQuery(cm2);
}

IParallelHandler::TTryResult TSearchFormHandler::TryToHandle(TContext& ctx) {
    SetAsResponse(ctx, false);
    if (const auto error = ctx.RunResponseFormHandler()) {
        return *error;
    }
    return ETryResult::Success;
}

bool ShouldReadSourceBeforeText(const TStringBuf source, const TStringBuf hostName) {
    if (FACTOID_SOURCES_WHITELIST.contains(source)) {
        return false;
    }
    if (FACTOID_HOSTNAMES_WHITELIST.contains(hostName)) {
        return false;
    }
    if (source == "fact_instruction"
        && (hostName == "povar.ru" || hostName == "m.povar.ru"))
    {
        return false;
    }
    return true;
}

} // namespace NBASS
