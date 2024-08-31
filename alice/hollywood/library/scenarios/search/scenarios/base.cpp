#include "base.h"
#include "facts.h"
#include "ellipsis_intents.h"

#include <alice/megamind/protos/scenarios/features/search.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>

namespace NAlice::NHollywood::NSearch {

TSearchScenario::TSearchScenario(TSearchContext& ctx)
    : Ctx(ctx)
{
}

bool TSearchScenario::Do(const TSearchResult& /* response */) {
    if (AddSerp(true, true)) {
        Ctx.GetFeatures().SetResponseWithSerp(1);
        Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName("serp");
        Ctx.SetIntent("serp");
        return true;
    }
    return false;
}

bool TSearchScenario::AddSerp(bool addSerpSuggest, bool autoAction, const TStringBuf query) {
    if (!Ctx.IsSerpSupported()) {
        return false;
    }

    NJson::TJsonValue result;
    result["url"] = query ? Ctx.GenerateSearchUri(query) : Ctx.GenerateSearchUri();

    Ctx.SetResultSlot(TStringBuf("serp"), result, /* setIntent */false);

    AddSerpVoiceButton(Ctx);
    if (addSerpSuggest) {
        LOG_INFO(Ctx.GetLogger()) << "Add serp result with suggest";
        if (Ctx.GetRequest().Interfaces().GetHasNavigator() && autoAction) {
            AddSerpConfirmationButton(Ctx);
            Ctx.AddSuggest(TStringBuf("search__serp"), /* autoaction */false);
            Ctx.AddAttention("ask_confirmation");
        } else {
            Ctx.AddSuggest(TStringBuf("search__serp"), autoAction);
            Ctx.SetShouldListen(!autoAction);
        }
    } else {
        LOG_INFO(Ctx.GetLogger()) << "Add serp result";
    }

    return true;
}

bool AppendRelatedFacts(TSearchContext& ctx, const TSearchResult& searchResult) {
    TMaybe<NJson::TJsonValue> snippet = FindFactoidInWizplaces(searchResult, TStringBuf("suggest_fact"));
    if (snippet.Empty()){
        size_t pos = 0;
        snippet = FindFactoidInDocs(searchResult, TStringBuf("suggest_fact"), 1, ESS_CONSTRUCT, pos);
    }
    if (snippet.Empty()){
        return false;
    }

    const auto& relatedFacts = snippet.GetRef()["related_facts"].GetArray();
    for (const auto& dict : relatedFacts) {
        ctx.AddRelatedSuggest(dict["query"].GetString());
    }
    return !relatedFacts.empty();
}

bool AppendRelatedSuggestQueries(TSearchContext& ctx, const TSearchResult& searchResult) {
    LOG_INFO(ctx.GetLogger()) << "Adding related suggests";
    if (searchResult.WizplacesLight) {
        for (const auto& related : searchResult.WizplacesLight->GetRelated()) {
            if (related.GetType() == TStringBuf("request_extensions") && !related.GetItems().empty()) {
                for (const auto& text : related.GetItems()[0].GetText()) {
                    ctx.AddRelatedSuggest(text);
                }
                return true;
            }
        }
    }
    if (!searchResult.Wizplaces) {
        return false;
    }
    const auto* related = FindField(*searchResult.Wizplaces, "related");
    if (!related) {
        return false;
    }

    for (const auto& entry : related->list_value().values()) {
        const auto* type = FindField(entry.struct_value(), "type");
        if (!type || type->string_value() != TStringBuf("request_extensions")) {
            continue;
        }
        const auto* items = FindField(entry.struct_value(), "items");
        if (!items || items->list_value().values().size() < 1) {
            return false;
        }
        const auto* texts = FindField(items->list_value().values()[0].struct_value(), "text");
        for (const auto& text : texts->list_value().values()) {
            ctx.AddRelatedSuggest(text.string_value());
        }
        return true;
    }
    return false;
}

bool TSearchScenario::AddRelated(TSearchContext& ctx, const TSearchResult& searchResult) {
    if (!ctx.IsSuggestSupported()) {
        return false;
    }
    bool success = false;
    if (!ctx.GetRequest().HasExpFlag(NExperiments::DISABLE_RELATED_FACTS_SUGGESTS)) {
        success |= AppendRelatedFacts(ctx, searchResult);
    }
    success |= AppendRelatedSuggestQueries(ctx, searchResult);
    return success;
}

void TSearchScenario::PostProcessAnswer(TSearchContext& ctx, const TSearchResult& response) {
    AddRelated(ctx, response);
    ctx.AddResult();
    if (ctx.GetRequest().HasExpFlag(NExperiments::FLAG_FACT_SNIP_ADDITIONAL_DATA)) {
        TMaybe<NJson::TJsonValue> construct;
        if (response.WizplacesLight && !response.WizplacesLight->GetImportant().empty()) {
            construct = JsonFromProto(response.WizplacesLight->GetImportant()[0].GetConstruct()[0]);
        } else if (response.Wizplaces) {
            construct = GetFirstWizplacesImportantConstruct(*response.Wizplaces);
        }
        if (construct.Defined()) {
            auto sourceEvent = ctx.GetAnalyticsInfoBuilder().AddRequestSourceEvent(TInstant::Now(),
                                                                                   "fact_snip_additional_data");
            sourceEvent->SetResponseCode(200, true).SetResponseBody(JsonToString(*construct));
        }
    }
}

} // namespace NAlice::NHollywood::NSearch
