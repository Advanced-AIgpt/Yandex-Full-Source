#include "facts.h"
#include "ellipsis_intents.h"

#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/search/utils/utils.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>
#include <alice/library/network/common.h>
#include <alice/library/proto/proto_struct.h>
#include <alice/library/url_builder/url_builder.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/features/search.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>

#include <library/cpp/iterator/zip.h>
#include <library/cpp/string_utils/url/url.h>

#include <util/charset/utf8.h>
#include <util/datetime/parser.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NSearch {

namespace {

inline bool CheckSportsType(const TStringBuf type) {
    return type == TStringBuf("sport/livescore") ||
            type == TStringBuf("sport/tournament") ||
            type == TStringBuf("special/event");
}

inline bool CheckSportsSubtype(const TStringBuf subtype) {
    return subtype.StartsWith("sport/") ||
            subtype.StartsWith("football") ||
            subtype.StartsWith("hockey");
}


static constexpr TStringBuf ENTITY_FACT = "entity-fact";
static constexpr TStringBuf SUGGEST_FACT = "suggest_fact";
static constexpr TStringBuf RICH_FACT = "rich_fact";
static constexpr TStringBuf CALORIES_FACT = "calories_fact";
static constexpr TStringBuf ENTITY_SEARCH = "entity_search";
static constexpr TStringBuf RELATED_DISCOVERY = "related_discovery";

const TString ONTOFACTS_SCENARIO = "ontofacts";

// TODO(kolyakolya) decide on max position
const size_t MAX_ENTITY_SEARCH_POS = 5;

const size_t MAX_RELATED_DISCOVERY_POS = 15;

const double CM2_SUMMARIZATION_THRESHOLD = 0.05;

static const THashSet<TStringBuf> FACTOID_SOURCES_WHITELIST = {
    "object_facts",
    "big_mediawiki",
    "znatoki",
    "yandex_q_child",
    "fresh_console_fact",
    "wiki-facts-objects",
    "poetry_casual"
};

static const THashSet<TStringBuf> FACTOID_HOSTNAMES_WHITELIST = {
    "ru.wikipedia.org",
    "ru.m.wikipedia.org"
};

static const TVector<std::pair<TString, TMaybe<TString>>> BANNED_WIZARDS_TYPES ={
    {"alice-gifts", Nothing()},
    {"alice_postcard", Nothing()},
    {"single-row-carousel", "newyear"},
    {"special/event", "newyear"},
};

// OTBET-90
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

} // namespace

TMaybe<NJson::TJsonValue> FindFactoidInDocsRight(const TSearchResult& searchResult, const TStringBuf snippetType,
                                                 size_t maxPos, ESnippetSection section, size_t& pos)
{
    if (searchResult.DocsRightLight) {
        return FindFactoidInDocs(*searchResult.DocsRightLight, snippetType, maxPos, section, pos);
    } else if (searchResult.DocsRight) {
        return FindFactoidInDocs(*searchResult.DocsRight, snippetType, maxPos, section, pos);
    }
    return Nothing();
}

TMaybe<NJson::TJsonValue> FindFactoidInWizplaces(const TSearchResult& searchResult, const TStringBuf snippetType) {
    if (searchResult.WizplacesLight) {
        return FindFactoidInWizplacesImportant(*searchResult.WizplacesLight, snippetType);
    } else if (searchResult.Wizplaces) {
        return FindFactoidInWizplacesImportant(*searchResult.Wizplaces, snippetType);
    }
    return Nothing();
}

TMaybe<NJson::TJsonValue> FindFactoidInDocs(const TSearchResult& searchResult, const TStringBuf snippetType,
                                            size_t maxPos, ESnippetSection section, size_t& pos)
{
    if (searchResult.DocsLight) {
        return FindFactoidInDocs(*searchResult.DocsLight, snippetType, maxPos, section, pos);
    } else if (searchResult.Docs) {
        return FindFactoidInDocs(*searchResult.Docs, snippetType, maxPos, section, pos);
    }
    return Nothing();
}

// TODO(kolyakolya) findout if entity fact can be in Docs
bool TSearchFactsScenario::AddEntityFact(const TSearchResult& searchResult, size_t& pos) {
    LOG_INFO(Ctx.GetLogger()) << "Checking entity fact";
    TMaybe<NJson::TJsonValue> snippet = FindFactoidInWizplaces(searchResult, ENTITY_FACT);
    Ctx.GetFeatures().SetFactFromWizplaces(snippet.Defined());
    pos = 0;
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Checking docs";
        snippet = FindFactoidInDocs(searchResult, ENTITY_FACT, /* maxPos */ 1, ESS_CONSTRUCT, pos);
        Ctx.GetFeatures().SetFactFromDocs(snippet.Defined());
    }
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
        return ResetFeatures();
    }

    const auto& data = *snippet;
    NJson::TJsonValue factoid;
    TStringBuilder text;
    for (const NJson::TJsonValue& item : data["requested_facts"]["item"][0]["value"].GetArray()) {
        if (!text.empty()) {
            text << ", ";
        }
        text << item["text"].GetString();
    }
    if (text.empty()) {
        LOG_WARN(Ctx.GetLogger()) << "Cant create factoid for " << data;
        return ResetFeatures();
    }

    factoid["url"] = data["base_info"]["url"].GetString();
    if (!factoid["url"].GetString().empty()) {
       AddFactoidSrcSuggest(Ctx, factoid);
    }
    if (TStringBuf title = data["requested_facts"]["item"][0]["name"].GetString()) {
        factoid["title"] = title;
    }
    factoid["text"] = text;
    factoid["source"] = data["source"];
    factoid["tts"] = GetFilteredVoiceInfo(data)["text"].GetString();
    AddFactoidPhone(data, factoid, false);
    if (TString avatar = data["base_info"]["image"]["avatar"].GetString()) {
        AddDivCardImage(avatar, 120, 120, factoid);
    }
    factoid["hostname"] = GetHostName(data);

    Ctx.GetFeatures().SetFoundEntityFact(1);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);
    Ctx.AddRenderedCard(factoid);

    return true;
}

bool TSearchFactsScenario::AddDistanceFact(const TSearchResult& searchResult, size_t& pos) {
    LOG_INFO(Ctx.GetLogger()) << "Checking distance fact";
    TMaybe<NJson::TJsonValue> factoidData = FindFactoidInWizplaces(searchResult, TStringBuf("distance_fact"));
    pos = 0;
    if (!factoidData) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
        return ResetFeatures();
    }
    Ctx.GetFeatures().SetFactFromWizplaces(1);
    const NJson::TJsonValue& data = *factoidData;
    const auto& distanceJSON = data["data"]["distance"];
    if (!IsNumber(distanceJSON)) {
        LOG_WARN(Ctx.GetLogger()) << "Unexpected distance type: " << distanceJSON;
        return ResetFeatures();
    }
    double distance = distanceJSON.GetDouble();
    // TODO(kolyakolya): tts???
    TStringBuf prefix = "Примерно ";
    TStringBuf suffix;
    if (distance > 1000) {
        distance = std::round(distance / 1000.0);
        suffix = TStringBuf(" км");
    } else if (distance > 1) {
        suffix = TStringBuf(" м");
    } else if (distance > 0.01) {
        distance = std::round(distance * 100.0);
        suffix = TStringBuf(" см");
    } else if (distance > 0.0001) {
        distance = std::round(distance * 1000.0);
        suffix = TStringBuf(" мм");
    } else {
        distance = std::round(distance * 1000000000);
        suffix = TStringBuf(" нм");
    }
    TString text = TStringBuilder() << prefix << ToString(distance) << suffix;
    NJson::TJsonValue factoid;
    factoid["text"] = text;
    factoid["snippet_type"] = TStringBuf("distance_fact");
    factoid["serp_data"] = data;
    factoid["source"] = data["source"];
    Ctx.GetFeatures().SetFoundDistanceFact(1);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);
    Ctx.AddRenderedCard(factoid);

    return true;
}

bool TSearchFactsScenario::AddCalculator(const TSearchResult& searchResult, size_t& pos) {
    LOG_INFO(Ctx.GetLogger()) << "Checking for calculator";
    const auto snippet = FindFactoidInDocs(searchResult, TStringBuf("calculator"), /* maxPos */ 10, ESS_SNIPPETS_FULL, pos);
    Ctx.GetFeatures().SetFactFromDocs(snippet.Defined());
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
        return ResetFeatures();
    }
    const NJson::TJsonValue& data = *snippet;

    const TString& result = data["result"].GetString();
    if (result.empty()) {
        return ResetFeatures();
    }
    Ctx.SetResultSlot(TStringBuf("calculator"), TryRoundFloat(result));
    Ctx.GetFeatures().SetFoundCalculatorFact(1);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);
    AddSerp(true);
    return true;
}

bool TSearchFactsScenario::AddUnitsConverter(const TSearchResult& searchResult, size_t& pos) {
    LOG_INFO(Ctx.GetLogger()) << "Checking for units converter";
    const auto factoidData = FindFactoidInDocs(searchResult, TStringBuf("units_converter"), 1,
                                               ESS_SNIPPETS_FULL, pos);
    if (!factoidData) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
        return ResetFeatures();
    }
    Ctx.GetFeatures().SetFactFromDocs(1);
    const NJson::TJsonValue& data = factoidData.GetRef()["data"];
    LOG_DEBUG(Ctx.GetLogger()) << "units converter data: " << data;

    NJson::TJsonValue factoid;

    const auto& voiceInfo = GetFilteredVoiceInfo(data);
    const TString& tts = voiceInfo["text"].GetString();
    if (tts.empty()) {
        LOG_DEBUG(Ctx.GetLogger()) << "no tts in units converter";
        return ResetFeatures();
    }

    factoid["tts"] = tts;
    factoid["text"] = tts;
    factoid["voice_info"] = voiceInfo;
    factoid["source"] = data["source"];
    factoid["snippet_type"] = TStringBuf("units_converter");
    Ctx.GetFeatures().SetFoundConverterFact(1);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);
    Ctx.AddRenderedCard(factoid);
    return true;
}

bool TSearchFactsScenario::AddRichFact(const TSearchResult& searchResult, size_t& pos) {
    LOG_INFO(Ctx.GetLogger()) << "Checking rich fact";
    TMaybe<NJson::TJsonValue> snippet = FindFactoidInDocs(searchResult, RICH_FACT, /* maxPos */ 1,
                                                          ESS_CONSTRUCT, pos);

    Ctx.GetFeatures().SetFactFromDocs(snippet.Defined());
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Checking wizplaces";
        snippet = FindFactoidInWizplaces(searchResult, RICH_FACT);
        Ctx.GetFeatures().SetFactFromWizplaces(snippet.Defined());
        pos = 0;
    }
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
        return ResetFeatures();
    }

    return AddSuggestFactImpl(*snippet);
}

bool TSearchFactsScenario::AddSuggestFact(const TSearchResult& searchResult, size_t& pos) {
    LOG_INFO(Ctx.GetLogger()) << "Checking suggest fact";
    TMaybe<NJson::TJsonValue> snippet = FindFactoidInDocs(searchResult, SUGGEST_FACT, /* maxPos */ 1,
                                                          ESS_CONSTRUCT, pos);

    Ctx.GetFeatures().SetFactFromDocs(snippet.Defined());
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Checking wizplaces";
        snippet = FindFactoidInWizplaces(searchResult, SUGGEST_FACT);
        Ctx.GetFeatures().SetFactFromWizplaces(snippet.Defined());
        pos = 0;
    }
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
        return ResetFeatures();
    }

    if (Ctx.GetRequest().HasExpFlag(NExperiments::SUMMARIZATION_TRIGGER_FACTSNIP)
        && (*snippet)["source"].GetString() == "fact_snippet"
        && !ShouldAnswerWithSummarization(searchResult))
    {
        return ResetFeatures();
    }

    return AddSuggestFactImpl(*snippet);
}

bool TSearchFactsScenario::AddSuggestFactImpl(const NJson::TJsonValue& factoidData) {
    const TStringBuf subtype = factoidData["subtype"].GetString();
    TString text;
    TString ttsOverride;
    if (subtype.empty() || SUGGEST_FACT == subtype || TStringBuf("wikipedia_fact") == subtype) {
        const NJson::TJsonValue& node = factoidData["text"];
        if (node.IsString()) {
            text = RemoveHighlight(node.GetString());
        } else if (node.IsArray()) {
            TStringBuilder sb;
            for (const auto& line : node.GetArray()) {
                if (line.IsString()) {
                    if (!sb.empty())
                        sb << "\n";
                    sb << RemoveHighlight(line.GetString());
                }
            }
            text = sb;
        }
    } else if (TStringBuf("quotes") == subtype) {
        text = factoidData["text"].GetString();
        if (text.empty()) {
            const auto& textArray = factoidData["text"].GetArray();
            if (textArray.size() > 0 && textArray[0].GetArray().size() > 0) {
                const auto& node = textArray[0].GetArray()[0];
                if (!node.IsNull()) {
                    const TStringBuf currency = node["currency"].GetString();
                    const TString amount = ForceString(node["text"]);
                    if (currency && amount) {
                        text = TStringBuilder() << amount << ' ' << currency;
                    }
                }
            }
        }
    } else if (TStringBuf("list_featured_snippet") == subtype && !Ctx.GetRequest().HasExpFlag(NExperiments::DISABLE_FACT_LISTS)) {
        const TString& prefix = RemoveHighlight(factoidData["text"].GetString());
        bool isOrdered = factoidData["list_type"].GetString() == TStringBuf("ol");

        const auto& jsonItems = factoidData["list_items"].GetArray();
        TVector<TString> items;
        items.reserve(jsonItems.size());
        for (const auto& json: jsonItems) {
            items.push_back(RemoveHighlight(json.GetString()));
        }
        text = JoinListFact(prefix, items, isOrdered);
        ttsOverride = JoinListFact(prefix, items, isOrdered, /*isTts = */ true);
    }

    if (text.empty() && factoidData.Has("goodwinResponse")) {
        text = factoidData["goodwinResponse"]["text"].GetString();
    }

    if (text.empty() || !IsUtf(text)) {
        LOG_DEBUG(Ctx.GetLogger()) << "Unsupported suggest_fact: " << factoidData;
        return ResetFeatures();
    }

    const TString source = factoidData["source"].GetString();
    const TString hostName = GetHostName(factoidData);

    NJson::TJsonValue voiceInfo = GetFilteredVoiceInfo(factoidData);
    voiceInfo["read_source_before_text"] = ShouldReadSourceBeforeText(source, hostName)
        && !Ctx.GetRequest().HasExpFlag(NExperiments::VOICE_FACTS_SOURCE_AFTER_TEXT);
    if (!ttsOverride.empty()) {
        voiceInfo["text"] = ttsOverride;
    }

    if (voiceInfo["text"].GetString().empty() && factoidData.Has("goodwinResponse")) {
        voiceInfo["text"] = factoidData["goodwinResponse"]["speech"].GetString();
    }

    if(voiceInfo["text"].GetString().empty() &&
       !Ctx.GetRequest().BaseRequestProto().GetInterfaces().GetCanRenderDivCards())
    {
        LOG_DEBUG(Ctx.GetLogger()) << "Missing voice_info: " << factoidData;
        return ResetFeatures();
    }

    NJson::TJsonValue factoid;
    factoid["url"] = factoidData["url"].GetString();
    if (!factoid["url"].GetString().empty()) {
        AddFactoidSrcSuggest(Ctx, factoid);
    }

    factoid["tts"] = voiceInfo["text"].GetString();
    factoid["text"] = text;
    factoid["source"] = source;
    if (factoid["source"] == "wizard:calend") {
        Ctx.GetFeatures().SetFoundSuggestFactDate(1);
    }

    AddFactoidPhone(factoidData, factoid, true);
    AddRelatedFactPromo(factoidData, factoid);

    factoid["voice_info"] = voiceInfo;
    if (TStringBuf article = factoidData["article_name"].GetString()) {
        factoid["title"] = article;
    } else if (TStringBuf question = factoidData["question"].GetString()) {
        factoid["title"] = question;
    } else if (TString headline = RemoveHighlight(factoidData["headline"].GetString()); !headline.empty()) {
        factoid["title"] = headline;
    }
    factoid["snippet_type"] = TStringBuf("suggest_fact");
    factoid["serp_data"] = factoidData;

    if (factoidData["gallery"].GetArray().size() > 0) {
        const auto gallery0 = factoidData["gallery"].GetArray()[0];
        if (gallery0.IsNull() || !AddDivCardImage(ForceString(gallery0["thmb_href"]),
                                                static_cast<ui16>(ForceInteger(gallery0["thmb_w_orig"])),
                                                static_cast<ui16>(ForceInteger(gallery0["thmb_h_orig"])),
                                                factoid))
        {
            AddDivCardImage(CreateAvatarIdImageSrc(factoidData, 122, 122), 122, 122, factoid);
        }
    }

    factoid["hostname"] = hostName;
    factoid["child_search"]["is_child_utterance"] = factoidData["is_child_utterance"];
    factoid["child_search"]["is_child_answer"] = factoidData["is_child_answer"];

    Ctx.GetFeatures().SetFoundSuggestFact(1);
    Ctx.AddRenderedCard(factoid);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);
    return true;
}

bool TSearchFactsScenario::ShouldAnswerWithSummarization(const TSearchResult& searchResult) const {
    // Summarization trigger (besides experiments and other answers priority).
    if (!(Ctx.GetRequest().ClientInfo().IsSmartSpeaker()
        || Ctx.GetRequest().ClientInfo().IsTvDevice()
        || Ctx.GetRequest().ClientInfo().IsLegatus()
        || Ctx.GetRequest().HasExpFlag(NAlice::NExperiments::ENABLE_ALL_PLATFORMS_HOLLYWOOD_SUMMARIZATION)))
    {
        LOG_INFO(Ctx.GetLogger()) << "Disabling summarization because of app.";
        return false;
    }

    // Query is not commercial.
    if (Ctx.GetRequest().ClientInfo().IsTouch()) {
        // We simply know that there are banners.
        if (searchResult.Banner) {
            const auto& data = searchResult.Banner->GetData();
            if (data.GetDirectPremium().size() + data.GetDirectHalfpremium().size() > 0) {
                LOG_INFO(Ctx.GetLogger()) << "Disabling summarization because of ads.";
                return false;
            }
        }

        if (searchResult.Wizard) {
            const TStringBuf relev = searchResult.Wizard->GetRelev();
            // Extract cm2 from wizard/relev string.
            static constexpr TStringBuf CM2 = "cm2=";
            TStringBuf left;
            TStringBuf right;
            if (relev.TrySplit(CM2, left, right)){
                double score = FromStringWithDefault(right.NextTok(";"), 0.0);
                if (score > CM2_SUMMARIZATION_THRESHOLD) {
                    LOG_INFO(Ctx.GetLogger()) << "Disabling summarization because query is commercial. cm2=" << score;
                    return false;
                }
            }
        }
    }

    if (searchResult.Wizard && searchResult.Wizard->GetRearr().Contains("wizdetection_politota=1")) {
        LOG_INFO(Ctx.GetLogger()) << "Disabling summarization because query is politota.";
        return false;
    }

    // Condition: query score should be good enough and query is not banned.
    return searchResult.Summarization->GetQueryIsGood() == "1";
}

bool TSearchFactsScenario::AddSummarizationSyncRequest(const TSearchResult& searchResult, TRTLogger& logger) {
    LOG_INFO(logger) << "Trying to prepare sync summarization request";
    const auto& summarization = *searchResult.Summarization;
    const auto& snippets = summarization.GetSnippets();
    const auto& urls = summarization.GetAnswersUrls();
    const auto& headlines = summarization.GetAnswersHeadlines();

    if (snippets.Empty() || urls.Empty())
    {
        LOG_INFO(logger) << "No summarization sync request fields in search result";
        return false;
    }

    TSummarizationRequest requestProto;

    requestProto.SetQuery(Ctx.GetTaggerQuery());

    const auto factSnippets = JsonFromString(Base64Decode(snippets)).GetArray();
    const auto factUrls = JsonFromString(Base64Decode(urls)).GetArray();
    auto factHeadlines = JsonFromString(Base64Decode(headlines)).GetArray();
    factHeadlines.resize(factSnippets.size(), {});

    for (const auto& [snip, url, headline] : Zip(factSnippets, factUrls, factHeadlines)) {
        auto* doc = requestProto.AddSerpResults();
        for (const auto& s : snip.GetArray()) {
            doc->SetSnippet(TString(s.GetString()));
            break;
        }
        doc->SetUrl(TString(url.GetString()));
        doc->SetHeadline(TString(headline.GetString()));
    }

    Ctx.SetSummarizationRequest(requestProto);
    LOG_INFO(logger) << "Sync summarization request is prepared.";

    return true;
}

bool TSearchFactsScenario::AddSummarizationAsyncRequest(const TSearchResult& searchResult, TRTLogger& logger) {
    const auto& data = *searchResult.Summarization;
    if (data.GetUri().Empty() || data.GetRequestId().Empty()) {
        LOG_WARN(Ctx.GetLogger()) << "Required summarization fields are absent.";
        return false;
    }

    const TStringBuf reqId = (*searchResult.Summarization).GetRequestId();

    NUri::TUri uri;
    if (!NNetwork::TryParseUri(searchResult.Summarization->GetUri(), uri)) {
        LOG_ERROR(logger) << "Failed to parse uri for serp summarization" << Endl;
        return false;
    }

    TStringBuilder summarizationHostPort = TStringBuilder{} << uri.GetField(NUri::TUri::TField::FieldHost) << ':';

    if (const TStringBuf port = uri.GetField(NUri::TUri::TField::FieldPort); !port.empty()) {
        summarizationHostPort << port;
    }

    Ctx.SetSummarizationHostPort(summarizationHostPort);

    // Prepare request.
    TSummarizationAsyncRequest requestProto;
    requestProto.SetRequestId(reqId.data());

    Ctx.SetSummarizationAsyncRequest(
        PrepareHttpRequest(uri.GetField(NUri::TUri::TField::FieldPath), Ctx.GetRequestMeta(), Ctx.GetLogger(),
            "summarization", requestProto.SerializeAsString(), NAppHostHttp::THttpRequest::Post));

    return true;
}

void TSearchFactsScenario::AddSummarizationRequest(const TSearchResult& searchResult, TRTLogger& logger) {
    if (searchResult.Summarization == nullptr) {
        LOG_INFO(logger) << "No summarization response";
        return;
    }

    if (!Ctx.GetRequest().HasExpFlag(NAlice::NExperiments::SERP_SUMMARIZATION_BE_BRAVE) && !ShouldAnswerWithSummarization(searchResult)) {
        LOG_INFO(logger) << "Query is bad, skipping summarization";
        return;
    }

    if (!AddSummarizationAsyncRequest(searchResult, logger)) {
        AddSummarizationSyncRequest(searchResult, logger);
    }
}

bool TSearchFactsScenario::AddSummarizationAnswer(TScenarioHandleContext& handleCtx) {
    TSummarizationEndResponse response;
    if (handleCtx.ServiceCtx.HasProtobufItem("summarization")) {
        response = GetOnlyProtoOrThrow<NSearch::TSummarizationEndResponse>(handleCtx.ServiceCtx, "summarization");
    } else if (const auto maybeResponse = RetireHttpResponseProtoMaybe<NSearch::TSummarizationEndResponse>(handleCtx)) {
        response = *maybeResponse;
    } else {
        return false;
    }
    if (response.GetFailed()) {
        LOG_DEBUG(Ctx.GetLogger()) << "Summarization failed to find answer." << Endl;
        return false;
    }

    // Construct fact data.
    NJson::TJsonValue serpData;
    serpData["text"] = response.GetOutput().GetSummary();
    serpData["type"] = "suggest_fact";
    serpData["source"] = "summarization";
    serpData["url"] = response.GetOutput().GetUrl();

    NJson::TJsonValue voiceInfo;
    voiceInfo["text"] = response.GetOutput().GetSummary();
    if (!response.GetOutput().GetVoicedSource().empty()) {
        voiceInfo["source"] = response.GetOutput().GetVoicedSource();
    } else {
        voiceInfo["trusted_source"] = false;
    }
    serpData["voiceInfo"]["ru"][0] = voiceInfo;

    // Stub for OTBET-89
    serpData["path"]["items"][0]["text"] = GetOnlyHost(response.GetOutput().GetUrl());

    // Interpret as suggest fact wizard.
    return AddSuggestFactImpl(serpData);
}

bool TSearchFactsScenario::AddTimeDifference(const TSearchResult& searchResult, size_t& pos) {
    LOG_INFO(Ctx.GetLogger()) << "Checking for time difference";
    TMaybe<NJson::TJsonValue> snippet = FindFactoidInDocs(searchResult, TStringBuf("time"), /* maxPos */ 1,
                                                          ESS_SNIPPETS_FULL, pos);
    Ctx.GetFeatures().SetFactFromDocs(snippet.Defined());
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
        return ResetFeatures();
    }

    const NJson::TJsonValue& data = *snippet;

    TStringBuf date1 = data["date1"].GetString();
    TStringBuf date2 = data["date2"].GetString();
    TIso8601DateTimeParser parser1;
    TIso8601DateTimeParser parser2;
    if (!parser1.ParsePart(date1.data(), date1.size()) || !parser2.ParsePart(date2.data(), date2.size())) {
        LOG_WARN(Ctx.GetLogger()) << "Cant parse ISO8601 datetime: " << date1 << ", " << date2 << Endl;
        return ResetFeatures();
    }
    i32 diff = parser2.GetDateTimeFields().ZoneOffsetMinutes - parser1.GetDateTimeFields().ZoneOffsetMinutes;
    TString text = FormatTimeDifference(diff, Ctx.GetLangName());

    NJson::TJsonValue factoid;
    factoid["text"] = text;
    factoid["tts"] = GetFilteredVoiceInfo(data)["text"].GetString();
    factoid["snippet_type"] = TStringBuf("time");
    factoid["serp_data"] = data;
    factoid["source"] = data["source"].GetString();
    LOG_DEBUG(Ctx.GetLogger()) << "time difference: " << factoid;
    if (data["format"] == "single") {
        Ctx.GetFeatures().SetFoundTimeFact(1);
    } else {
        Ctx.GetFeatures().SetFoundTimeDifferenceFact(1);
    }
    Ctx.AddRenderedCard(factoid);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);
    return true;
}

bool TSearchFactsScenario::AddZipCode(const TSearchResult& searchResult, size_t& pos) {
    LOG_INFO(Ctx.GetLogger()) << "Checking for zip code";
    TMaybe<NJson::TJsonValue> snippet = FindFactoidInDocs(searchResult, TStringBuf("post_indexes"), /* maxPos */ 2,
                                                          ESS_SNIPPETS_FULL, pos);
    Ctx.GetFeatures().SetFactFromDocs(snippet.Defined());
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
        return ResetFeatures();
    }
    const NJson::TJsonValue& data = *snippet;
    const TString& text = data["obj"]["index"].GetString();
    if (text.empty()) {
        LOG_DEBUG(Ctx.GetLogger()) << "Empty text in zip code fact";
        return ResetFeatures();
    }
    NJson::TJsonValue factoid;
    factoid["text"] = text;
    factoid["tts"] = text;
    factoid["snippet_type"] = TStringBuf("post_indexes");
    factoid["serp_data"] = data;
    LOG_DEBUG(Ctx.GetLogger()) << "zipcode: " << factoid;
    Ctx.GetFeatures().SetFoundZipCodeFact(1);
    Ctx.AddRenderedCard(factoid);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);
    return true;
}

//TODO(tolyandex) is this factoid still needed?
bool TSearchFactsScenario::AddTableFact(const TSearchResult& searchResult, size_t& pos) {
    if (!Ctx.GetRequest().HasExpFlag("table_fact")) {
        LOG_INFO(Ctx.GetLogger()) << "Table fact is disabled";
        return false;
    }
    LOG_INFO(Ctx.GetLogger()) << "Checking for table fact";
    TMaybe<NJson::TJsonValue> snippet = FindFactoidInWizplaces(searchResult, TStringBuf("table_fact"));
    Ctx.GetFeatures().SetFactFromWizplaces(snippet.Defined());
    pos = 0;
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
        return ResetFeatures();
    }
    // TODO(tolyandex) doesn't work
    const NJson::TJsonValue& data = *snippet;
    NJson::TJsonValue factoid;
    factoid["snippet_type"] = TStringBuf("table_fact");
    factoid["serp_data"] = data;
    factoid["source"] = data["source"];
    Ctx.AddRenderedCard(factoid);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);
    return true;
}

bool TSearchFactsScenario::AddSportLivescore(const TSearchResult& searchResult, size_t& pos) {
    LOG_INFO(Ctx.GetLogger()) << "Checking for sport live score";
    if (AddSportLivescoreWizplaces(searchResult)) {
        pos = 0;

        return true;
    }
    if (AddSportLivescoreDocs(searchResult, pos)) {
        return true;
    }
    LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
    return false;
}

bool TSearchFactsScenario::AddSportLivescoreDocs(const TSearchResult& docs, size_t& pos) {
    const auto snippet = FindFactoidInDocs(docs, TStringBuf("sport/livescore"), /* maxPos */ 1, ESS_SNIPPETS_FULL, pos);
    if (snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Found livescore in docs";
        Ctx.GetFeatures().SetFactFromDocs(snippet.Defined());
        return AddSportLivescoreImpl(*snippet);
    }
    return false;
}

bool TSearchFactsScenario::CheckLiveScoreDoc(const ::google::protobuf::Struct& doc) {
    const auto* typePtr = FindField(doc, "type");
    const auto* subtypePtr = FindField(doc, "subtype");
    return typePtr && subtypePtr
        && CheckSportsType(typePtr->string_value())
        && CheckSportsSubtype(subtypePtr->string_value())
        && AddSportLivescoreImpl(JsonFromProto(doc));
}

bool TSearchFactsScenario::AddSportLivescoreWizplaces(const TSearchResult& search) {
    if (search.WizplacesLight) {
        for (const auto& doc : search.WizplacesLight->GetImportant()) {
            if (CheckSportsType(doc.GetType()) && CheckSportsSubtype(doc.GetSubtype()) &&
                AddSportLivescoreImpl(JsonFromProto(doc)))
            {
                Ctx.GetFeatures().SetFactFromWizplaces(1);
                return true;
            }
        }
    }
    return false;
}

bool TSearchFactsScenario::AddSportLivescoreImpl(const NJson::TJsonValue& data) {
    NJson::TJsonValue factoid;
    const auto& voiceInfo = GetFilteredVoiceInfo(data["data"]);
    // Euro 2021 campaign
    if (data["data"]["competition_id"].GetString() == "3270") {
        LOG_DEBUG(Ctx.GetLogger()) << "Disable livescore fact for euro 2021";
        Ctx.GetFeatures().SetFoundSportLiveScoreFact(1);
        Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName("serp");
        Ctx.SetIntent("serp");
        return AddSerp(true, true);
    }
    if (TStringBuf text = voiceInfo["text"].GetString()) {
        factoid["text"] = text;
        factoid["tts"] = text;
        factoid["voice_info"] = voiceInfo;
        factoid["source"] = "sport/livescore";

        Ctx.GetFeatures().SetFoundSportLiveScoreFact(1);
        Ctx.SetResultSlot(TStringBuf("factoid"), factoid);
        Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);
        AddSerp(true);
        return true;
    }
    LOG_DEBUG(Ctx.GetLogger()) << "no sport text in: " << data;
    return ResetFeatures();
}

bool TSearchFactsScenario::AddCaloriesFact(const TSearchResult& searchResult, size_t& pos) {
    LOG_INFO(Ctx.GetLogger()) << "Checking for calories fact";
    TMaybe<NJson::TJsonValue> snippet = FindFactoidInWizplaces(searchResult, CALORIES_FACT);
    Ctx.GetFeatures().SetFactFromWizplaces(snippet.Defined());
    pos = 0;
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Checking docs";
        snippet = FindFactoidInDocs(searchResult, CALORIES_FACT, /* maxPos */ 1, ESS_CONSTRUCT, pos);
        Ctx.GetFeatures().SetFactFromDocs(snippet.Defined());
    }
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
        return ResetFeatures();
    }

    const NJson::TJsonValue& data = *snippet;
    NJson::TJsonValue factoid;

    factoid["text"] = data["data"]["calories1"].GetString();
    factoid["tts"] = GetFilteredVoiceInfo(data)["text"].GetString();
    factoid["url"] = data["url"].GetString();
    if (!factoid["url"].GetString().empty()) {
        AddFactoidSrcSuggest(Ctx, factoid);
    }
    AddDivCardImage(CreateAvatarIdImageSrc(data, 91, 91), 91, 91, factoid);
    factoid["snippet_type"] = TStringBuf("calories_fact");
    factoid["serp_data"] = data;
    factoid["source"] = data["source"].GetString();
    factoid["hostname"] = GetHostName(data);
    Ctx.GetFeatures().SetFoundCaloriesFact(1);
    Ctx.AddRenderedCard(factoid);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);
    return true;
}

bool TSearchFactsScenario::AddRelatedFactFallback() {
    const TStringBuf relatedFactJson = Ctx.GetState().GetRelatedFactoidSerpData();
    const TStringBuf relatedFactoidQuery = Ctx.GetState().GetRelatedFactoidQuery();

    if (!relatedFactJson.empty()
        && !relatedFactoidQuery.empty()
        && relatedFactoidQuery == Ctx.GetTaggerQuery())
    {
        const NJson::TJsonValue serpData = JsonFromString(relatedFactJson);
        // TODO: actually could be entity fact etc.
        //       Separate and reuse this logic from AddFactoid.
        if (AddSuggestFactImpl(serpData)) {
            Ctx.GetAnalyticsInfoBuilder().AddObject(
                "related_fact_fallback_success",
                "related fact fallback successful",
                "");
            return true;
        }
        Ctx.GetAnalyticsInfoBuilder().AddObject(
            "related_fact_fallback_fail",
            "related fact fallback failed",
            "");
    }
    return false;
}

bool TSearchFactsScenario::ProcessBannedWizards(const TSearchResult& searchResult, size_t& pos) {
    if (!Ctx.GetRequest().BaseRequestProto().GetInterfaces().GetCanOpenLink()) {
        return ResetFeatures();
    }

    TMaybe<NJson::TJsonValue> snippet;
    for (const auto& wizard : BANNED_WIZARDS_TYPES) {
        snippet = FindFactoidInDocs(searchResult, wizard.first, /* maxPos */ 2, ESS_CONSTRUCT, pos);
        if (!snippet) {
            continue;
        }
        const auto& subtype = wizard.second;
        if (!subtype.Defined() || (*snippet)["subtype"] == *subtype) {
            break;
        }
    }

    if (!snippet) {
        return ResetFeatures();
    }

    LOG_DEBUG(Ctx.GetLogger()) << "Found banned for alice wizard";
    Ctx.GetFeatures().SetFactFromDocs(1);
    Ctx.GetFeatures().SetFoundSuggestFact(1);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName("serp");
    Ctx.SetIntent("serp");
    return AddSerp(true, true);
}

bool TSearchFactsScenario::AddFactoid(const TSearchResult& searchResult, size_t& pos) {
    bool found = ProcessBannedWizards(searchResult, pos)
                ||  AddSuggestFact(searchResult, pos)
                ||  AddRichFact(searchResult, pos)
                ||  AddEntityFact(searchResult, pos)
                ||  AddCalculator(searchResult, pos)
                ||  AddUnitsConverter(searchResult, pos)
                ||  AddDistanceFact(searchResult, pos)
                ||  AddTimeDifference(searchResult, pos)
                ||  AddZipCode(searchResult, pos)
                ||  AddTableFact(searchResult, pos)
                ||  AddSportLivescore(searchResult, pos)
                ||  AddCaloriesFact(searchResult, pos)
                ||  AddRelatedFactFallback();

    LOG_INFO(Ctx.GetLogger()) << "Found factoid: " << found;
    return found;
}

bool TSearchFactsScenario::AddObjectAsFact(const TSearchResult& searchResult, size_t& pos) {
    LOG_INFO(Ctx.GetLogger()) << "Checking object answer";
    TMaybe<NJson::TJsonValue> snippet = FindFactoidInDocs(searchResult, ENTITY_SEARCH, MAX_ENTITY_SEARCH_POS,
                                                          ESS_SNIPPETS_FULL, pos);
    Ctx.GetFeatures().SetFactFromDocs(snippet.Defined());
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Checking docs right";
        snippet = FindFactoidInDocsRight(searchResult, ENTITY_SEARCH, MAX_ENTITY_SEARCH_POS,
                                         ESS_SNIPPETS_FULL, pos);
        Ctx.GetFeatures().SetFactFromRightDocs(snippet.Defined());
    }
    if (!snippet) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
        return ResetFeatures();
    }

    if (!snippet->Has("data")) {
        LOG_DEBUG(Ctx.GetLogger()) << "Snippet is empty";
        return ResetFeatures();
    }
    const NJson::TJsonValue& data = snippet.GetRef()["data"];
    if (ForceInteger(data["display_options"]["show_as_fact"]) == 0) {
        LOG_DEBUG(Ctx.GetLogger()) << "Object shouldn't be showed as fact";
        return ResetFeatures();
    }
    const NJson::TJsonValue& baseInfo = data["base_info"];
    const TString& name = baseInfo["name"].GetString();
    const TString& titleOrName = baseInfo.Has("title") ? baseInfo["title"].GetString() : name;
    if (titleOrName.empty()) {
        LOG_ERR(Ctx.GetLogger()) << "OO without title: " << data;
        return ResetFeatures();
    }
    if (name.empty()) {
        LOG_ERR(Ctx.GetLogger()) << "OO without name: " << data;
        return ResetFeatures();
    }
    const TString& description = baseInfo["description"].GetString();
    if (description.empty()) {
        LOG_ERR(Ctx.GetLogger()) << "OO without description: " << data;
        return ResetFeatures();
    }
    NJson::TJsonValue factoid;
    const auto& voiceInfo = GetFilteredVoiceInfo(data);
    factoid["title"] = titleOrName;
    factoid["text"] = description;
    factoid["tts"] = voiceInfo["text"];
    factoid["voice_info"] = voiceInfo;
    factoid["url"] = baseInfo["description_source"]["url"].GetString();
    factoid["hostname"] = ParseHostName(factoid["url"].GetString());
    factoid["serp_url"] = Ctx.GenerateSearchUri(titleOrName,
            TCgiParameters({std::make_pair(TString{"entref"}, TString{baseInfo["entref"].GetString()})})
    );
    if (TString avatar = baseInfo["image"]["avatar"].GetString()) {
        AddDivCardImage(avatar, 120, 120, factoid);
    }
    if (const auto* imageGallery = data.GetValueByPath("view.image_gallery")) {
        factoid["image_gallery"] = NJson::TJsonArray();
        for (const auto& image : imageGallery->GetArray()) {
            NJson::TJsonValue galleryImage;
            galleryImage["thmb_href"] = image["thmb_href"];
            galleryImage["thmb_h_orig"] = image["thmb_h_orig"];
            galleryImage["thmb_w_orig"] = image["thmb_w_orig"];
            factoid["image_gallery"].AppendValue(galleryImage);
        }
    }

    if (Ctx.GetRequest().HasExpFlag(NExperiments::OBJECT_AS_FACT_LONG_TTS)
        && factoid["url"].GetString().Contains("ru.wikipedia.org"))
    {
        // Replace original voiceInfo with full text.
        const TString longTts = TStringBuilder() << "По данным русской википедии: " << factoid["text"].GetString();
        factoid["tts"] = longTts;
        factoid["voice_info"]["text"] = longTts;
    }

    Ctx.GetFeatures().SetFoundObjectAsFact(1);
    Ctx.AddRenderedCard(factoid, TStringBuf("object"), TStringBuf("search_object"));
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);

    auto nlgSlots = Ctx.GetNlgData().Form["slots"];
    if (nlgSlots.GetArray().size() > 0) {
        nlgSlots[0]["value"]["entity_name"] = name;
    } else {
        LOG_ERROR(Ctx.GetLogger()) << "Search nlg form is empty!";
    }
    return true;
}

void TSearchFactsScenario::AddFactoidPhone(const NJson::TJsonValue& factoidData, NJson::TJsonValue& factoid, bool addSuggest) {
    const TStringBuf phone = factoidData["data"]["phone"].GetString();
    if (Ctx.GetRequest().ClientInfo().IsTouch() && !phone.empty()) {
        factoid["phone"] = phone;
        factoid["phone_uri"] = GeneratePhoneUri(Ctx.GetRequest().ClientInfo(), phone);
        AddFactoidCallButton(Ctx, factoid["phone_uri"].GetString());
        if (addSuggest) {
            Ctx.AddSuggest(TStringBuf("search__phone_call"));
        }
    }
}

void TSearchFactsScenario::AddRelatedFactPromo(const NJson::TJsonValue& factoidData, NJson::TJsonValue& factoid) const {

    // Check if enabled.
    if (!(Ctx.GetRequest().ClientInfo().IsSmartSpeaker() || Ctx.GetRequest().ClientInfo().IsTvDevice() || Ctx.GetRequest().ClientInfo().IsLegatus())
        || Ctx.GetRequest().HasExpFlag(NExperiments::DISABLE_RELATED_FACTS_PROMO)
        || Ctx.HasPostroll)
    {
        return;
    }

    // Get current query.
    TString currentQuery = Ctx.GetTaggerQuery();
    if(currentQuery.empty()) {
        currentQuery = Ctx.GetRequest().Input().Utterance();
    }

    // Get history.
    THashSet<TString> bayanQueries;
    {
        const auto& history = Ctx.GetState().GetRelatedQueriesHistory();
        bayanQueries.reserve(history.size() + 2);
        for(const auto& query: history) {
            bayanQueries.insert(TString(query));
        }
    }

    // Find related fact.
    TRelatedFact relatedFact;
    if (!IsBadRelatedFactQuery(currentQuery)) {

        // All related facts available.
        TVector<TRelatedFact> relatedFacts;
        if (!Ctx.GetRequest().HasExpFlag(NExperiments::RELATED_FACTS_DONT_USE_DISCOVERY)) {
            const TSearchResult& searchResult = NSearch::GetSearchReport(Ctx.GetRequest());
            relatedFacts = GetRelatedFactsFromDiscovery(searchResult);
        } else {
            relatedFacts = GetRelatedFactsFromFactoid(factoidData);
        }

        // Filter by query.
        const auto queryFilter = [this, &bayanQueries](const TRelatedFact& fact) {
            return fact.Query.empty()
                || bayanQueries.contains(fact.Query)
                || IsBadRelatedFactQuery(fact.Query);
        };
        relatedFacts.erase(
            std::remove_if(relatedFacts.begin(), relatedFacts.end(), queryFilter),
            relatedFacts.end());

        // Select one of.
        if (!relatedFacts.empty()) {
            if (Ctx.GetRequest().HasExpFlag(NExperiments::RELATED_FACTS_DISABLE_SHUFFLE)) {
                relatedFact = relatedFacts[0];
            } else {
                relatedFact = relatedFacts[Ctx.GetRng().RandomInteger(relatedFacts.size())];
            }
        }
    }

    // Check probability.
    if (!relatedFact.Query.empty()) {
        const TMaybe<TStringBuf> probaStr = GetExperimentValueWithPrefix(
            Ctx.GetRequest().ExpFlags(),
            NExperiments::RELATED_FACTS_PROMO_PROBA_PREFIX);

        double proba = 0.0;
        if (!probaStr.Empty() && TryFromString(probaStr.GetRef(), proba)) {
            const double rnd = Ctx.GetRng().RandomDouble();
            if (rnd < proba) {
                relatedFact = {};
            }
        }
    }

    // Add related query.
    if (!relatedFact.Query.empty()) {
        factoid["related_query"] = relatedFact.Query;
        Ctx.HasPostroll = true;

        // Confirm action
        {
            NScenarios::TFrameAction actionAgree;
            actionAgree.MutableNluHint()->SetFrameName("alice.search.related_agree");

            TParsedUtterance& parsedUtterance = *actionAgree.MutableParsedUtterance();
            parsedUtterance.SetUtterance(relatedFact.Query);

            TSearchSemanticFrame& searchFrame =
                *parsedUtterance.MutableTypedSemanticFrame()->MutableSearchSemanticFrame();
            searchFrame.MutableQuery()->SetStringValue(relatedFact.Query);

            TAnalyticsTrackingModule& analytics = *parsedUtterance.MutableAnalytics();
            analytics.SetProductScenario(ONTOFACTS_SCENARIO);
            analytics.SetOrigin(TAnalyticsTrackingModule::Scenario);
            analytics.SetPurpose("related_promo_agree");

            Ctx.AddAction("confirm", std::move(actionAgree));
        }

        // Decline action
        if (!Ctx.GetRequest().HasExpFlag(NExperiments::RELATED_FACTS_DISABLE_NOTHING_ON_DECLINE)) {
            Ctx.AddDoNothingButton();
        }

        Ctx.GetAnalyticsInfoBuilder().AddObject(
            "factoid_related_query",
            "promoted related fact query",
            relatedFact.Query);
    }

    // Update state.
    {
        if (!currentQuery.empty()) {
            bayanQueries.insert(currentQuery);
        }
        if (!relatedFact.Query.empty()) {
            bayanQueries.insert(relatedFact.Query);
        }

        TVector<TStringBuf> newHistoryOrdered(bayanQueries.begin(), bayanQueries.end());
        Sort(newHistoryOrdered);

        Ctx.GetState().ClearRelatedQueriesHistory();
        Ctx.GetState().MutableRelatedQueriesHistory()->Reserve(newHistoryOrdered.size());
        for (const TStringBuf& query: newHistoryOrdered) {
            *Ctx.GetState().AddRelatedQueriesHistory() = query;
        }

        Ctx.GetState().ClearRelatedFactoidSerpData();
        Ctx.GetState().ClearRelatedFactoidQuery();
        if (relatedFact.SerpData.IsDefined()) {
            Ctx.GetState().SetRelatedFactoidSerpData(JsonToString(relatedFact.SerpData));
            Ctx.GetState().SetRelatedFactoidQuery(relatedFact.Query);
        }

        Ctx.SetIsUsingState(true);
    }
}

bool TSearchFactsScenario::ResetFeatures() {
    Ctx.GetFeatures().SetFactoidPosition(1.);
    Ctx.GetFeatures().SetFactFromWizplaces(0);
    Ctx.GetFeatures().SetFactFromRightDocs(0);
    Ctx.GetFeatures().SetFactFromDocs(0);
    return false;
}

const NJson::TJsonValue& TSearchFactsScenario::GetVoiceInfo(const NJson::TJsonValue& snippet) {
    const NJson::TJsonValue& voiceInfo = snippet["voiceInfo"][Ctx.GetLangName()][0];
    if (voiceInfo.IsNull()) {
        // Try fallback case with ru TTS. It fixes cases when tld=kz but tts exists only for ru.
        return snippet["voiceInfo"]["ru"][0];
    }

    return voiceInfo;
}

NJson::TJsonValue TSearchFactsScenario::GetFilteredVoiceInfo(const NJson::TJsonValue& snippet) {
    const NJson::TJsonValue& voiceInfo = GetVoiceInfo(snippet);

    if (Ctx.GetRequest().HasExpFlag(NExperiments::DISABLE_READ_FACTOID_SOURCE)) {
        NJson::TJsonValue result;
        result["text"] = voiceInfo["text"]; // Returning only tts
        return result;
    }

    return voiceInfo;
}

bool TSearchFactsScenario::IsBadRelatedFactQuery(const TStringBuf& query) const {

    // Should be done when preparing related facts dictionary.
    if (!Ctx.GetRequest().HasExpFlag(NExperiments::RELATED_FACTS_ENABLE_FILTER)) {
        return false;
    }

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

TVector<TSearchFactsScenario::TRelatedFact> TSearchFactsScenario::GetRelatedFactsFromFactoid(const NJson::TJsonValue& serpData)
{
    TVector<TRelatedFact> result;

    const auto& relatedFacts = serpData["related_facts"].GetArray();
    result.reserve(relatedFacts.size());

    for (const NJson::TJsonValue& fact: relatedFacts) {
        const TString query = fact["query"].GetString();
        result.push_back({query, NJson::TJsonValue{}});
    }

    return result;
}

TVector<TSearchFactsScenario::TRelatedFact> TSearchFactsScenario::GetRelatedFactsFromDiscovery(const TSearchResult& response)
{
    TVector<TRelatedFact> result;

    size_t dummyPos = 0;
    const TMaybe<NJson::TJsonValue> relatedDiscovery =
        FindFactoidInDocs(response, RELATED_DISCOVERY, MAX_RELATED_DISCOVERY_POS, ESS_CONSTRUCT, dummyPos);

    if (relatedDiscovery.Defined()) {
        const auto& docs = (*relatedDiscovery)["docs"].GetArray();
        result.reserve(docs.size());

        for (const NJson::TJsonValue& doc :docs) {
            const TString query = doc["query"].GetString();
            const NJson::TJsonValue serpData = doc["content"];

            // TODO: reuse AddSuggestFactImpl logic of preparing rendered card,
            //       store rendered card instead of serpData.
            if (serpData["text"].GetString().empty()) {
                continue;
            }

            result.push_back({query, serpData});
        }
    }
    return result;
}

bool TSearchFactsScenario::Do(const TSearchResult& response) {
    size_t pos = 0;
    if (AddFactoid(response, pos)) {
        Ctx.GetFeatures().SetFactoidPosition(pos / 10.);
        return true;
    }
    ResetFeatures();
    return false;
}

bool TSearchFactsScenario::DoObject(const TSearchResult& response) {
    size_t pos = 0;
    if (AddObjectAsFact(response, pos)) {
        Ctx.GetFeatures().SetFactoidPosition(pos / 10.);
        return true;
    }
    ResetFeatures();
    return false;
}

bool TSearchFactsScenario::DoSummarization(const TSearchResult& response) {
    LOG_INFO(Ctx.GetLogger()) << "Checking for summarization response";
    AddSummarizationRequest(response, Ctx.GetLogger());
    ResetFeatures();
    return false;
}

} // namespace NAlice::NHollywood::NSearch
