#include "nav.h"

#include <alice/hollywood/library/scenarios/search/utils/utils.h>

#include <alice/megamind/protos/scenarios/web_search_source.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/url_builder/url_builder.h>

namespace NAlice::NHollywood::NSearch {

namespace {

TString GenerateApplicationUri(const TSearchContext& ctx, TStringBuf app, TStringBuf fallbackUrl) {
    const auto& request = ctx.GetRequest();
    return NAlice::GenerateApplicationUri(request.Interfaces().GetCanOpenLinkIntent(), request.ClientInfo(), app, fallbackUrl);
}

NJson::TJsonValue CreateNavigationBlock(TStringBuf text, TStringBuf tts, TStringBuf app, TStringBuf url, const TSearchContext& ctx) {
    NJson::TJsonValue result;
    result["text"] = text;
    result["tts"] = tts;
    result["url"] = app ? GenerateApplicationUri(ctx, app, url) : url;
    return result;
}

} // namespace

bool TSearchNavScenario::Do(const TSearchResult& response) {
    if (Ctx.IsPornoQuery() || !Ctx.GetRequest().HasExpFlag("search_enable_bno")) {
        LOG_INFO(Ctx.GetLogger()) << "Bno intent is disabled";
        return false;
    }
    LOG_INFO(Ctx.GetLogger()) << "Start bno intent";
    if (AddNav(response.DocsLight) || AddNav(response.Docs)) {
        return true;
    }
    LOG_INFO(Ctx.GetLogger()) << "No bno found";
    return false;
}

bool TSearchNavScenario::AddNav(const ::google::protobuf::ListValue* docs) {
    if (!docs) {
        return false;
    }
    const auto& docsArray = docs->values();
    if (docsArray.empty()) {
        return false;
    }
    const auto& docProto = docsArray[0];
    const auto& doc = JsonFromProto(docProto);

    if (!Ctx.IsSerpSupported()) {
        return false;
    }

    return AddNavImpl(doc, docProto);
}

bool TSearchNavScenario::AddNav(const NScenarios::TWebSearchDocs* docs) {
    if (!Ctx.IsSerpSupported() || !docs || docs->GetDocs().empty()) {
        return false;
    }
    const auto& docProto = docs->GetDocs()[0];
    const auto& doc = JsonFromProto(docProto);

    return AddNavImpl(doc, docProto);
}

template<typename TDoc>
bool TSearchNavScenario::AddNavImpl(const NJson::TJsonValue& doc, TDoc& docProto) {
    LOG_INFO(Ctx.GetLogger()) << "Searching for bno snippet";
    const auto bno = FindSnippet(docProto, TStringBuf("bno"), ESS_CONSTRUCT);
    const auto generic = FindSnippet(docProto, TStringBuf("generic"), ESS_SNIPPETS_MAIN);
    if (!bno.Defined()) {
        return false;
    }

    TString text;
    if (generic.Defined()) {
        if (!generic->GetArray().empty()) {
            text = RemoveHighlight(generic.GetRef()["passages"].GetArray()[0].GetString());
        }
        if (text.empty()) {
            text = RemoveHighlight(generic.GetRef()["headline"].GetString());
        }
    }

    if (text.empty()) {
        text = RemoveHighlight(doc["doctitle"].GetString());
    }

    if (text.empty()) {
        LOG_DEBUG(Ctx.GetLogger()) << "Cannot find text for bno: " << doc << Endl;
        return false;
    }

    TStringBuf app;
    if (Ctx.GetRequest().ClientInfo().IsAndroid()) {
        app = bno.GetRef()["mobile_apps"]["gplay"]["id"].GetString();
    } else if (Ctx.GetRequest().ClientInfo().IsIOS()) {
        app = bno.GetRef()["mobile_apps"]["itunes"]["id"].GetString();
    }

    const NJson::TJsonValue& result = CreateNavigationBlock(text, TStringBuf(""), app, doc["url"].GetString(), Ctx);
    Ctx.SetResultSlot(TStringBuf("nav"), result);
    Ctx.AddSuggest(TStringBuf("search__nav"), true);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName("nav_url");
    return true;
}

} // namespace NAlice::NHollywood::NSearch
