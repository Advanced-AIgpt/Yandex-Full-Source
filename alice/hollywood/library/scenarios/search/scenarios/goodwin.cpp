#include "goodwin.h"

#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>
#include <alice/megamind/protos/scenarios/features/search.pb.h>
#include <alice/hollywood/library/scenarios/search/scenarios/facts.h>
#include <alice/hollywood/library/scenarios/search/utils/serp_helpers.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NSearch {
    TEntitySearchGoodwinScenario::TEntitySearchGoodwinScenario(TSearchContext& ctx)
        : TSearchScenario{ctx}
        , RunHandler{
              MakeSimpleHttpRequester(),
              [](const NFrameFiller::TSearchDocMeta& meta) {
                  return meta.Type == "entity_search";
              }} {
    }

    bool TEntitySearchGoodwinScenario::Do(const TSearchResult& searchResult) {
        size_t pos;
        if (Ctx.GetRequest().HasExpFlag(NExperiments::FORCE_SEARCH_GOODWIN)) {
            LOG_DEBUG(Ctx.GetLogger()) << "Goodwin is forced";
            pos = 0;
        } else if (Ctx.GetRequest().Interfaces().GetSupportsShowView()) {
            return false;
        } else {
            TMaybe<NJson::TJsonValue> snippet = FindFactoidInDocs(searchResult, "entity_search", 5, ESS_SNIPPETS_FULL, pos);
            if (!snippet) {
                LOG_DEBUG(Ctx.GetLogger()) << "Checking docs right";
                snippet = FindFactoidInDocsRight(searchResult, "entity_search", 5, ESS_SNIPPETS_FULL, pos);
                Ctx.GetFeatures().SetFactFromRightDocs(true);
            } else {
                Ctx.GetFeatures().SetFactFromDocs(true);
            }
            if (!snippet) {
                LOG_DEBUG(Ctx.GetLogger()) << "Snippet wasn't found";
                return false;
            }
            if (!snippet->Has("data")) {
                LOG_DEBUG(Ctx.GetLogger()) << "Snippet is empty";
                return false;
            }
            const NJson::TJsonValue& data = snippet.GetRef()["data"];
            if (data["display_options"]["show_as_fact"].GetIntegerRobust() == 0) {
                LOG_DEBUG(Ctx.GetLogger()) << "Object shouldn't be showed as fact";
                return false;
            }
        }

        LOG_INFO(Ctx.GetLogger()) << "Checking EntitySearch goodwin";
        if (Ctx.GetRequest().Input().Proto().HasCallback()) {
            LOG_INFO(Ctx.GetLogger()) << "Skip EntitySearch goodwin callback request";
            return false;
        }

        auto goodwinResponse = NFrameFiller::Run(Ctx.GetRequest(), RunHandler, Ctx.GetLogger());
        if (goodwinResponse.GetResponseBody().GetLayout().CardsSize() == 0 || goodwinResponse.GetFeatures().GetIsIrrelevant()) {
            LOG_DEBUG(Ctx.GetLogger()) << "EntitySearch GoodwinResponse is empty";
            return false;
        }
        NJson::TJsonValue card;
        card["text"] = goodwinResponse.GetResponseBody().GetLayout().GetCards(0).GetText();
        card["tts"] = goodwinResponse.GetResponseBody().GetLayout().GetOutputSpeech();
        Ctx.AddRenderedCard(card);
        Ctx.GetAnalyticsInfoBuilder().SetIntentName(goodwinResponse.GetResponseBody().GetAnalyticsInfo().GetIntent());
        Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(goodwinResponse.GetResponseBody().GetAnalyticsInfo().GetProductScenarioName());
        for (auto [key, action] : goodwinResponse.GetResponseBody().GetFrameActions()) {
            Ctx.AddAction(key, std::move(action));
            Ctx.HasPostroll = true;
        }
        Ctx.GetFeatures().SetFoundObjectAsFact(true);
        Ctx.GetFeatures().SetFactoidPosition(pos / 10.);
        return true;
    }

    TGoodwinApplyScenario::TGoodwinApplyScenario(TContext& ctx)
        : Ctx{ctx} {
    }

    TMaybe<NScenarios::TScenarioApplyResponse> TGoodwinApplyScenario::Do(const TScenarioApplyRequestWrapper& request) {
        LOG_INFO(Ctx.Logger()) << "Applying Search goodwin";
        return NFrameFiller::Apply(request, RunHandler, Ctx.Logger());
    }

    TGoodwinCommitScenario::TGoodwinCommitScenario(TContext& ctx)
        : Ctx{ctx}
        , RunHandler{MakeSimpleHttpRequester()} {
    }

    TMaybe<NScenarios::TScenarioCommitResponse> TGoodwinCommitScenario::Do(const TScenarioApplyRequestWrapper& request) {
        LOG_INFO(Ctx.Logger()) << "Applying Search goodwin";
        return NFrameFiller::Commit(request, RunHandler, Ctx.Logger());
    }
}
