#include "search_dispatcher.h"

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/scenarios/search/nlg/register.h>
#include <alice/hollywood/library/scenarios/search/proto/search.pb.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/json/json.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywoodFw::NSearch {

HW_REGISTER(TSearchScenario);

TSearchScenario::TSearchScenario()
    : TScenario(NProductScenarios::SEARCH)
{
    Register(&TSearchScenario::Dispatch);
    RegisterScene<TSearchOldFlowScene>([this]() {
        RegisterSceneFn(&TSearchOldFlowScene::Main);
    });
    RegisterScene<TSearchScreenDeviceScene>([this]() {
        RegisterSceneFn(&TSearchScreenDeviceScene::Main);
    });
    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NSearch::NNlg::RegisterAll);

    // Note apphost graph for Search has Dispatch+SceneMain -> Render flow!
    SetApphostGraph(ScenarioRequest() >>
                    NodeRun("prepare") >>
                    ScenarioResponse());

    // Temporary use DIV render outside Hollywood graph
    SetDivRenderMode(TScenario::EDivRenderMode::PrepareForOutsideMerge);
}

/*
    New SEARCH scenario dispatcher (HWF)
    Pass execution to following scenes:
    * TSearchScreenDeviceScene (for Centaur or if exp "search_new_richcard_centaur" enabled)
    * TSearchOldFlowScene (for all other cases, execution will be forwarded into old scenario)
*/
TRetScene TSearchScenario::Dispatch(const TRunRequest& request, const TStorage& storage, const TSource& source) const {
    Y_UNUSED(storage);
    Y_UNUSED(source);

    if (!request.Flags().IsExperimentEnabled("search_new_richcard_centaur") ||
         request.Flags().IsExperimentEnabled("search_old_richcard_centaur")) {
        LOG_DEBUG(request.Debug().Logger()) << "Flag search_new_richcard_centaur is absent";
        return TReturnValueScene<TSearchOldFlowScene>(NHollywood::TSearchEmptyProto{});
    }

    if (request.Client().GetInterfaces().GetSupportsShowView() ||
        request.Flags().IsExperimentEnabled("search_new_richcard_centaur"))
    {
        LOG_DEBUG(request.Debug().Logger()) << "New search pocessing for centaur, utterance: " << request.Input().GetUtterance();
        return TReturnValueScene<TSearchScreenDeviceScene>(NHollywood::TSearchEmptyProto{});
    }
    LOG_DEBUG(request.Debug().Logger()) << "Old search pocessing for all other surfaces";
    return TReturnValueScene<TSearchOldFlowScene>(NHollywood::TSearchEmptyProto{});
}

void TSearchScenario::Hook(THookInputInfo& info, NScenarios::TScenarioRunResponse& runResponse) const {
    if (info.NewContext->RunRequest->Flags().IsExperimentEnabled("search_dump_card_as_md")) {
        LOG_DEBUG(info.NewContext->RunRequest->Debug().Logger()) << "Search scenario run response: " << JsonFromProto(runResponse);
    }
}

} // namespace NAlice::NHollywoodFw::NSearch
