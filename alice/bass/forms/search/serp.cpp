#include "serp.h"

#include "direct_gallery.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/library/analytics/common/utils.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>
#include <alice/library/network/headers.h>
#include <alice/library/proto/proto.h>
#include <alice/library/util/search_convert.h>
#include <alice/library/websearch/direct_gallery.h>
#include <alice/library/websearch/prepare_search_request.h>
#include <alice/library/websearch/strip_alice_meta_info.h>
#include <alice/megamind/library/search/protos/alice_meta_info.pb.h>
#include <alice/rtlog/client/client.h>
#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <kernel/geodb/countries.h>

#include <library/cpp/neh/http_common.h>
#include <library/cpp/scheme/util/utils.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/scan/scan.h>

#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <util/stream/file.h>
#include <util/string/subst.h>

using namespace NAlice;
using namespace NAlice::NNetwork;

using EService = NAlice::TWebSearchBuilder::EService;

namespace NBASS {
namespace NSerp {

namespace {

constexpr TStringBuf TEXT_FIELD = "text";

void LogSearchReqId(TStringBuf reqid, const TString& requestActivationId) {
    if (reqid.empty() || !TLogging::RequestLogger) {
        return;
    }
    auto ev = NRTLogEvents::SearchRequest();
    ev.SetReqId(TString{reqid});
    ev.SetActivationId(requestActivationId);
    TLogging::RequestLogger->LogEvent(ev);
    LOG(INFO) << "WebSearch reqid: " << reqid << Endl;
}

} // namespace

namespace NImpl {

ui64 IncSearchCounter(TContext& ctx, EService service) {
    NMonitoring::TLabels labels;
    labels.Add("sensor", "search_request");
    labels.Add("client", ctx.MetaClientInfo().Name);
    labels.Add("bass_service", ToString(service));
    labels.Add("parent_form", ctx.ParentFormName());
    return ctx.GlobalCtx().Counters().Sensors().Rate(labels)->Inc();
}

}  // namespace NImpl

TResultValue ParseSearchResponse(NHttpFetcher::TResponse::TConstRef response, NSc::TValue* result) {
    if (response) {
        LOG(INFO) << "WebSearch response headers: " << response->Headers << Endl;
    }
    if (!response || response->IsError()) {
        return TError(TError::EType::SYSTEM, response ? response->GetErrorText() : TStringBuf("no response"));
    }

    NSc::TValue searchResult;
    if (!NSc::TValue::FromJson(searchResult, response->Data) || !searchResult.IsDict()) {
        return TError(TError::EType::SYSTEM, TStringBuilder() << TStringBuf("Cannot parse search result: ") << response->Data);
    }

    const auto& tmplData = searchResult["tmpl_data"];
    if (TStringBuf reqid = tmplData["reqdata"]["reqid"].GetString(); !reqid.empty()) {
        TVector<TStringBuf> tmp = StringSplitter(response->RTLogToken).Split('$');
        TString requestActivationId{};
        if (tmp.size() == 3) {
            requestActivationId = tmp[2];
        }
        LogSearchReqId(reqid, requestActivationId);
    } else {
        LOG(INFO) << "WebSearch reqid not found in response: " << response->Data << Endl;
    }

#if 0
    {
        TUnbufferedFileOutput f(TStringBuilder() << "resp/resp_" << TLogging::ReqInfo.Get().ReqId() << ".json");
        searchResult.ToJsonPretty(f);
    }
#endif

    result->Clear();

    NScUtils::CopyField(tmplData, *result, "wizplaces");
    NScUtils::CopyField(tmplData, *result, "searchdata");
    NScUtils::CopyField(tmplData, *result, "reqdata");
    NScUtils::CopyField(tmplData, *result, "search");
    NScUtils::CopyField(tmplData, *result, "banner");
    NScUtils::CopyField(searchResult, *result, "wizard");
    NScUtils::CopyField(searchResult, *result, "renderrer");

    constexpr TStringBuf tunnellerRawResponseKey = "tunneller_raw_response";
    if (searchResult.Has(tunnellerRawResponseKey)) {
        NScUtils::CopyField(searchResult, *result, tunnellerRawResponseKey);
    }

    if (searchResult.Has(SUMMARIZATION)) {
        NScUtils::CopyField(searchResult, *result, SUMMARIZATION);
    }

    return {};
}

bool IsTouchSearch(const NAlice::TClientInfo& clientInfo, const TSearchFeatures& searchFeatures) {
    return searchFeatures.Platform == ESearchPlatform::TOUCH ||
        (searchFeatures.Platform == ESearchPlatform::DEFAULT && (clientInfo.IsTouch() ||
                                                                 clientInfo.IsSmartSpeaker() ||
                                                                 clientInfo.IsElariWatch()));
}

void AddBassSearchSrcrwr(TWebSearchBuilder::TInternalFlagsBuilder& internalFlags, TStringBuf flagValue) {
    TStringBuilder sourcesRewritten;
    ScanKeyValue<true, ',', ':'>(flagValue, [&sourcesRewritten](TStringBuf name, TStringBuf value) {
        if (!name.empty()) {
            if (!sourcesRewritten.empty()) {
                sourcesRewritten << ',';
            }
            sourcesRewritten << TString{name}.Quote() << ':' << TString{value}.Quote();
        }
    });
    if (!sourcesRewritten.empty()) {
        internalFlags << TStringBuf(",\"srcrwr\":{") << sourcesRewritten << '}';
    }
}

TResultValue MakeRequest(TStringBuf query, TContext& context, const TCgiParameters& cgi, NSc::TValue* result,
                         EService service) {
    NHttpFetcher::TRequestPtr request = PrepareSearchRequest(query, context, cgi, service);
    LOG(DEBUG) << "search request: " << request->Url() << Endl;
    LOG(DEBUG) << "search request headers: "  << Endl;
    auto headers = request->GetHeaders();
    for (const auto& header: headers) {
        if (!header.StartsWith(HEADER_COOKIE)) {
            LOG(DEBUG) << "    " << header << Endl;
        } else {
            LOG(DEBUG) << "    " << "Obfuscated Cookie header" << Endl;
        }
    }
    NHttpFetcher::TResponse::TRef response = request->Fetch()->Wait();
    return ParseSearchResponse(response, result);
}

namespace {

NHttpFetcher::TRequestPtr PrepareRequest(
    TSourceRequestFactory source,
    TStringBuf tvm2clientId,
    TStringBuf query,
    TContext& context,
    const TCgiParameters& cgi,
    EService service,
    NHttpFetcher::IMultiRequest::TRef multiRequest,
    TString& encodedAliceMeta)
{
    const TClientInfo& clientInfo = context.MetaClientInfo();

    NBASS::TPersonalDataHelper personalDataHelper(context);
    TString userTicket;
    bool userTicketAvailable = personalDataHelper.GetTVM2UserTicket(userTicket);

    const auto ip = *context.Meta().ClientIP();

    auto processId = TString{*context.Meta().ProcessId()};

    TVector<TString> cookies;
    Copy(context.Meta().Cookies().begin(), context.Meta().Cookies().end(), cookies.begin());

    NAlice::TWebSearchBuilder webSearchBuilder = PrepareSearchRequest(
        query,
        context.ClientFeatures(),
        context.ClientFeatures().Experiments().GetRawExpFlags(),
        context.ClientFeatures().SupportsOpenLink(),
        context.GetSpeechKitEvent(),
        clientInfo.UserAgent,
        context.GetContentRestrictionLevel(),
        context.FormName(),
        context.Meta().HasLang() ? MakeMaybe(context.Meta().Lang()) : Nothing(),
        cgi,
        TLogging::ReqInfo.Get().ReqId(),
        context.Meta().HasUUID() ? MakeMaybe(*context.Meta().UUID()) : Nothing(),
        userTicketAvailable ? MakeMaybe(userTicket) : Nothing(),
        context.Meta().HasYandexUID() ? MakeMaybe(TString{ context.Meta().YandexUID() }) : Nothing(),
        !ip.Empty() ? MakeMaybe(ip) : Nothing(),
        cookies,
        service,
        context.Meta().MegamindCgiString(),
        !processId.empty() ? MakeMaybe(processId) : Nothing(),
        context.GetRngSeed(),
        context.UserRegion(),
        context.Meta().HasImageSearchGranet(),
        context.GetConfig().HamsterQuota(),
        context.GetConfig()->Vins().Search().WaitAll(),
        encodedAliceMeta,
        [](const TStringBuf msg) { LOG(WARNING) << msg << Endl; }
    );

    if (TString serviceTicket; !tvm2clientId.empty() && personalDataHelper.GetTVM2ServiceTicket(tvm2clientId, serviceTicket)) {
        webSearchBuilder.SetServiceTicket(serviceTicket);
    }

    NHttpFetcher::TRequestPtr request = source.MakeOrAttachRequest(multiRequest);
    NHttpFetcher::TRequestBuilder requestBuilder{*request};
    webSearchBuilder.Flush(requestBuilder);

    return request;
}

} // namespace

NHttpFetcher::TRequestPtr PrepareMusicSearchRequest(
    TStringBuf query,
    TContext& context,
    const TCgiParameters& cgi,
    NHttpFetcher::IMultiRequest::TRef multiRequest,
    TString& encodedAliceMeta)
{
    return PrepareRequest(
        context.GetSources().Search(),
        context.GetConfig().Vins().Search().Tvm2ClientId(),
        query,
        context,
        cgi,
        NAlice::TWebSearchBuilder::EService::BassMusic,
        multiRequest,
        encodedAliceMeta);
}

NHttpFetcher::TRequestPtr PrepareSearchRequest(
    TStringBuf query,
    TContext& context,
    const TCgiParameters& cgi,
    NAlice::TWebSearchBuilder::EService service,
    NHttpFetcher::IMultiRequest::TRef multiRequest)
{
    TString encodedAliceMeta; // dummy out param
    return PrepareRequest(
        context.GetSources().Search(),
        context.GetConfig().Vins().Search().Tvm2ClientId(),
        query,
        context,
        cgi,
        service,
        multiRequest,
        encodedAliceMeta);
}

bool TSnippetIterator::Next() {
    while (DocIt != Docs.cend()) {
        const NSc::TDict& snippets = (*DocIt)["snippets"].GetDict();
        ++DocIt;
        for (const auto& snipKv : snippets) {
            if (snipKv.second.IsDict()) {
                if (Predicate(snipKv.second)) {
                    CurSnippet = &snipKv.second;
                    return true;
                }
            } else if (snipKv.second.IsArray()) {
                for (const NSc::TValue& sn : snipKv.second.GetArray()) {
                    if (Predicate(sn)) {
                        CurSnippet = &sn;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

const NSc::TValue& GetVoiceInfo(const NSc::TValue& snippet, TStringBuf tld) {
    TString path = TStringBuilder() << TStringBuf("/voiceInfo/") << tld << TStringBuf("[0]");
    const NSc::TValue& voiceInfo = snippet.TrySelect(path);
    if (voiceInfo.IsNull()) {
        // Try fallback case with ru TTS. It fixes cases when tld=kz but tts exists only for ru.
        // TODO: sort this mess and find they how to get right tts in all cases.
        return snippet.TrySelect(TStringBuf("/voiceInfo/ru/[0]"));
    }

    return voiceInfo;
}

NSc::TValue GetFilteredVoiceInfo(const NSc::TValue& snippet, TStringBuf tld, const TContext& ctx) {
    const NSc::TValue& voiceInfo = GetVoiceInfo(snippet, tld);

    if (ctx.HasExpFlag(NAlice::NExperiments::DISABLE_READ_FACTOID_SOURCE)) {
        NSc::TValue result;
        result[TEXT_FIELD] = voiceInfo.TrySelect(TEXT_FIELD); // Returning only tts
        return result;
    }

    return voiceInfo;
}

TStringBuf GetVoiceTTS(const NSc::TValue& snippet, TStringBuf tld) {
    return GetVoiceInfo(snippet, tld).TrySelect(TEXT_FIELD).GetString();
}

TStringBuf GetHostName(const NSc::TValue& snippet) {
    return snippet.TrySelect(TStringBuf("/path/items/[0]/text")).GetString();
}

} // NSerp

namespace NSerpSnippets {

namespace {

void AddIfMatched(const NSc::TValue& node, TMaybe<TStringBuf> snippetType, TVector<const NSc::TValue*>* dest) {
    if (node.IsNull())
        return;
    if (node.IsArray()) {
        for (const NSc::TValue& item : node.GetArray()) {
            if (!snippetType || item["type"].GetString() == *snippetType) {
                dest->push_back(&item);
            }
        }
    } else if (!snippetType || node["type"].GetString() == *snippetType) {
        dest->push_back(&node);
    }
}

TVector<const NSc::TValue*> FindSnippets(const NSc::TValue& doc, TMaybe<TStringBuf> snippetType, ESnippetSection section) {
    TVector<const NSc::TValue*> found;

    if (section & ESS_SNIPPETS_ALL) {
        const NSc::TValue& snippets = doc["snippets"];
        if (!snippets.IsNull()) {
            if (section & ESS_SNIPPETS_FULL) {
                AddIfMatched(snippets["full"], snippetType, &found);
            }
            if (section & ESS_SNIPPETS_MAIN) {
                AddIfMatched(snippets["main"], snippetType, &found);
            }
            if (section & ESS_SNIPPETS_PRE) {
                AddIfMatched(snippets["pre"], snippetType, &found);
            }
            if (section & ESS_SNIPPETS_POST) {
                AddIfMatched(snippets["post"], snippetType, &found);
            }
        }
    }

    if (section & ESS_CONSTRUCT) {
        AddIfMatched(doc["construct"], snippetType, &found);
    }


    if (section & ESS_FLAT) {
        AddIfMatched(doc, snippetType, &found);
    }

    return found;
}

} // namespace anonymous

const NSc::TValue& FindSnippet(NSc::TValue& doc, TStringBuf snippetType, ESnippetSection section) {
    TString mergedKey = TStringBuilder() << snippetType << TStringBuf("_merged");
    const NSc::TValue& merged = doc[mergedKey];
    if (!merged.IsNull()) {
        return merged;
    }

    TVector<const NSc::TValue*> found = FindSnippets(doc, snippetType, section);

    if (found.size() == 0) {
        return NSc::Null();
    }

    if (found.size() == 1) {
        return *found[0];
    }

    NSc::TValue result = found[0]->Clone();
    for (size_t i = 1, count = found.size(); i < count; ++i) {
        result.MergeUpdate(*found[i]);
    }

    return doc[mergedKey] = result;
}

const NSc::TValue& FindAnySnippet(NSc::TValue& doc, const TVector<TStringBuf>& snippetTypes, ESnippetSection section) {
    for (TStringBuf snippetType : snippetTypes) {
        const NSc::TValue& snippet = FindSnippet(doc, snippetType, section);
        if (!snippet.IsNull())
            return snippet;
    }
    return NSc::Null();
}

const NSc::TValue& FindFirstSnippet(const NSc::TValue& doc, ESnippetSection section) {
#define RETURN_FIRST_SNIPPET(snippet)                           \
    {                                                           \
        if (snippet.IsDict())                                   \
            return snippet;                                     \
        else if (snippet.IsArray() && snippet.ArraySize() > 0)  \
            return snippet.GetArray()[0];                       \
    }

    if (section & ESS_CONSTRUCT)
        RETURN_FIRST_SNIPPET(doc["construct"]);

    if (section & ESS_SNIPPETS_ALL) {
        const NSc::TValue& snippets = doc["snippets"];
        if (!snippets.IsNull()) {
            if (section & ESS_SNIPPETS_FULL)
                RETURN_FIRST_SNIPPET(snippets["full"]);
            if (section & ESS_SNIPPETS_MAIN)
                RETURN_FIRST_SNIPPET(snippets["main"]);
            if (section & ESS_SNIPPETS_PRE)
                RETURN_FIRST_SNIPPET(snippets["pre"]);
            if (section & ESS_SNIPPETS_POST)
                RETURN_FIRST_SNIPPET(snippets["post"]);
        }
    }

    return NSc::Null();

#undef RETURN_FIRST_SNIPPET
}

TVector<const NSc::TValue*> FindSnippets(const NSc::TValue& doc, ESnippetSection section) {
    return FindSnippets(doc, Nothing(), section);
}

TVector<const NSc::TValue*> FindSnippets(const NSc::TValue& doc, TStringBuf snippetType, ESnippetSection section) {
    return FindSnippets(doc, TMaybe<TStringBuf>(snippetType), section);
}

static constexpr TStringBuf HL_OPEN_TAG = "\007[";
static constexpr TStringBuf HL_CLOSE_TAG = "\007]";

TString RemoveHiLight(TStringBuf str) {
    if (str.empty())
        return TString();
    TString s{TString{str}};
    SubstGlobal(s, HL_OPEN_TAG, TStringBuf());
    SubstGlobal(s, HL_CLOSE_TAG, TStringBuf());
    return s;
}

TString JoinListFact(const TString& text, const TVector<TString>& items, bool isOrdered, bool isTts) {
    TStringBuilder builder;
    builder << text;
    TString delim = isTts ? " sil<[0]> " : (isOrdered ? "" : " - ");
    for (size_t i = 0; i < items.size(); i++) {
        if (!isTts) {
            builder << "\n";
        }
        if (delim.Empty()) {  // ordered list
            builder << ToString(i + 1) << ". ";
        } else {
            builder << delim;
        }
        builder << items[i];
    }
    return builder;
}

} // namespace NSerpSnippets

} // namespace NBASS
