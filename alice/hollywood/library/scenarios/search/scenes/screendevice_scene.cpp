#include "screendevice_scene.h"

#include <alice/hollywood/library/scenarios/search/scenes/processors/proc_calculator.h>
#include <alice/hollywood/library/scenarios/search/scenes/processors/proc_calories.h>
#include <alice/hollywood/library/scenarios/search/scenes/processors/proc_factdist.h>
#include <alice/hollywood/library/scenarios/search/scenes/processors/proc_factentity.h>
#include <alice/hollywood/library/scenarios/search/scenes/processors/proc_factrich.h>
#include <alice/hollywood/library/scenarios/search/scenes/processors/proc_factsuggest.h>
#include <alice/hollywood/library/scenarios/search/scenes/processors/proc_factold.h>
#include <alice/hollywood/library/scenarios/search/scenes/processors/proc_generic.h>
#include <alice/hollywood/library/scenarios/search/scenes/processors/proc_location.h>
#include <alice/hollywood/library/scenarios/search/scenes/processors/proc_porno.h>

#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/hollywood/library/scenarios/search/utils/debug_md.h>
#include <alice/hollywood/library/util/service_context.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/search/fact.pb.h>
#include <alice/protos/data/scenario/search/richcard.pb.h>
#include <alice/protos/data/scenario/search/search_object.pb.h>

namespace NAlice::NHollywoodFw::NSearch {

namespace {

const TStringBuf EXP_FLAG_SEARCH_CENTAUR_RETURN_NOTHING = "search_centaur_return_nothing";

const TStringBuf RICHCARD_DIV_NAME = "search.rich.div.card";
const TStringBuf SEARCHCARD_DIV_NAME = "search.object.div.card";
const TStringBuf FACTCARD_DIV_NAME = "search.factoid.div.card";

} // anonymous namespace

TSearchScreenDeviceScene::TSearchScreenDeviceScene(const TScenario* owner)
    : TScene(owner, "screendevice_scene")
{
    RegisterRenderer(&TSearchScreenDeviceScene::Render);

    // Register all serach processors
    // Note ORDER OF REGISTRATION IS IMPORTANT!
    TSearchProcessorInstance::Instance().Register<TProcessorSearchPorno>();
    TSearchProcessorInstance::Instance().Register<TProcessorFactDistance>();
    TSearchProcessorInstance::Instance().Register<TProcessorFactEntity>();
    TSearchProcessorInstance::Instance().Register<TProcessorFactCalories>();
    TSearchProcessorInstance::Instance().Register<TProcessorFactSuggest>();
    TSearchProcessorInstance::Instance().Register<TProcessorFactRich>();
    TSearchProcessorInstance::Instance().Register<TProcessorFactOldFlow>();
    TSearchProcessorInstance::Instance().Register<TProcessorCalculator>();
    TSearchProcessorInstance::Instance().Register<TProcessorSearchGeneric>();
    TSearchProcessorInstance::Instance().Register<TProcessorSearchLocation>();
}

/*
    Main login for search engine for Centaur surface
*/
TRetMain TSearchScreenDeviceScene::Main(const NHollywood::TSearchEmptyProto& emptyArgs, const TRunRequest& request, TStorage&, const TSource&) const {
    // Prepare search data to process
    TSearchResultParser searchResults(request.Debug().Logger());
    searchResults.AttachDataSource(request.GetDataSource(EDataSourceType::WEB_SEARCH_DOCS, false));
    searchResults.AttachDataSource(request.GetDataSource(EDataSourceType::WEB_SEARCH_DOCS_RIGHT, false));
    searchResults.AttachDataSource(request.GetDataSource(EDataSourceType::WEB_SEARCH_WIZPLACES, false));
    searchResults.AttachDataSource(request.GetDataSource(EDataSourceType::WEB_SEARCH_WIZARD, false));

    // Temporary code for fast debugging
    if (request.Flags().IsExperimentEnabled("search_dump_card_as_md")) {
        searchResults.Dump(TSearchResultParser::EDumpMode::Json);
    }
    if (!searchResults.IsDataFound()) {
        // TODO [DD] Probably, this code can be moved to dispatcher?
        return TReturnValueRender("search", "centaur_nothing", emptyArgs).MakeIrrelevantAnswerFromScene();
    }

    //
    // Analyze searchResults object
    //
    TSearchResult res = {
        /* ScenarioData  */ NData::TScenarioData{},
        /* RenderArgs    */ NHollywood::TSearchRenderProto{},
        /* SearchFeatures*/ NScenarios::TSearchFeatures{},
        /* AiObject      */ NScenarios::TAnalyticsInfo::TObject{}};

    const auto rc = TSearchProcessorInstance::Instance().ProcessSearchObject(request, searchResults, res);
    switch (rc) {
        case ISearchProcessor::EProcessorRet::Unknown:
            // Not recognized, temporary switching to old flow scenario
            ReturnOldOrUnsupported(request, "No suitable search processor");
            if (!request.Flags().IsExperimentEnabled(EXP_FLAG_SEARCH_CENTAUR_RETURN_NOTHING)) {
                return TReturnValueDo();
            }
            // don't break
        case ISearchProcessor::EProcessorRet::Success:
            // Working with results (see below)
            break;
        case ISearchProcessor::EProcessorRet::OldFlow:
            ReturnOldOrUnsupported(request, "Request to switch to old flow");
        case ISearchProcessor::EProcessorRet::Irrelevant:
            return TReturnValueRender("search", "centaur_nothing", emptyArgs).MakeIrrelevantAnswerFromScene();
    }

    TRunFeatures features;
    features.Set(res.SearchFeatures);

    // Check what card was filled in processors
    if (res.ScenarioRenderCard.HasSearchRichCardData()) {
        const auto& richCard = res.ScenarioRenderCard.GetSearchRichCardData();
        if (request.Flags().IsExperimentEnabled("search_dump_card_as_md")) {
            DumpAsMd(request.Debug().Logger(), richCard);
        }
        res.RenderArgs.SetDivCardName(TString{RICHCARD_DIV_NAME});
        NRenderer::TDivRenderData divr;
        divr.SetCardId(TString{RICHCARD_DIV_NAME});
        *divr.MutableScenarioData() = std::move(res.ScenarioRenderCard);
        auto ret = TReturnValueRender(&TSearchScreenDeviceScene::Render, res.RenderArgs, std::move(features));
        return ret.AddDivRender(std::move(divr));
    }
    if (res.ScenarioRenderCard.HasSearchObjectData()) {
        if (request.Flags().IsExperimentEnabled("search_dump_card_as_md")) {
            LOG_INFO(request.Debug().Logger()) << JsonStringFromProto(res.ScenarioRenderCard);
        }
        res.RenderArgs.SetDivCardName(TString{SEARCHCARD_DIV_NAME});
        NRenderer::TDivRenderData divr;
        divr.SetCardId(TString{SEARCHCARD_DIV_NAME});
        *divr.MutableScenarioData() = std::move(res.ScenarioRenderCard);
        auto ret = TReturnValueRender(&TSearchScreenDeviceScene::Render, res.RenderArgs, std::move(features));
        return ret.AddDivRender(std::move(divr));
    }
    if (res.ScenarioRenderCard.HasSearchFactData()) {
        if (request.Flags().IsExperimentEnabled("search_dump_card_as_md")) {
            LOG_INFO(request.Debug().Logger()) << JsonStringFromProto(res.ScenarioRenderCard);
        }
        res.RenderArgs.SetDivCardName(TString{FACTCARD_DIV_NAME});
        NRenderer::TDivRenderData divr;
        divr.SetCardId(TString{FACTCARD_DIV_NAME});
        *divr.MutableScenarioData() = std::move(res.ScenarioRenderCard);
        auto ret = TReturnValueRender(&TSearchScreenDeviceScene::Render, res.RenderArgs, std::move(features));
        return ret.AddDivRender(std::move(divr));
    }
    if (!request.Flags().IsExperimentEnabled(EXP_FLAG_SEARCH_CENTAUR_RETURN_NOTHING)) {
        return TReturnValueDo();
    }
    return ReturnOldOrUnsupported(request, "Processor doesn't provide any card");
}

/*
    Render search answer using DIV card and response
*/
TRetResponse TSearchScreenDeviceScene::Render(const NHollywood::TSearchRenderProto& renderArgs, TRender& render) const {
    const TString phrase = renderArgs.GetPhrase().Empty() ? "render_hwf_answer" : renderArgs.GetPhrase();
    render.CreateFromNlg("search", phrase, renderArgs);

    // Add directives
    if (!renderArgs.GetDivCardName().Empty()) {
        NAlice::NScenarios::TShowViewDirective showViewDirective;
        showViewDirective.SetName("show_view");
        showViewDirective.SetDoNotShowCloseButton(true);
        // TODO Probably need to set this value in depends of answer (object/factiod?)
        showViewDirective.SetInactivityTimeout(NScenarios::TShowViewDirective_EInactivityTimeout_Long);
        *showViewDirective.MutableLayer()->MutableDialog() = NScenarios::TShowViewDirective::TLayer::TDialogLayer{};
        showViewDirective.SetCardId(renderArgs.GetDivCardName());
        render.Directives().AddShowViewDirective(std::move(showViewDirective));
    }

    NScenarios::TTtsPlayPlaceholderDirective ttsDirective;
    ttsDirective.SetName("tts_play_placeholder"); // To keep compatibility with old caconized data
    render.Directives().AddTtsPlayPlaceholderDirective(std::move(ttsDirective));
    render.SetShouldListen(true);
    return TReturnValueSuccess();
}

// Временная заглушка, которая на нераспознанные данные отвечает "Невозможно".
// Требуется для тестирования чтобы можно было распознать фразы, которые по каким-то причинам не обработались
// новой сценой
// Будет удалено позднее
TRetMain TSearchScreenDeviceScene::ReturnOldOrUnsupported(const TRunRequest& request, const TStringBuf details) const {
    if (request.Flags().IsExperimentEnabled("search_new_richcard_centaur")) {
        NHollywood::TSearchRenderProto renderArgs;
        renderArgs.SetTextAnswer("Поиск не распознал datasource");
        renderArgs.SetVoiceAnswer("Поиск не распознал datasource");
        return TReturnValueRender(&TSearchScreenDeviceScene::Render, renderArgs);
    }
    LOG_INFO(request.Debug().Logger()) << "Switch to old flow: " << details;
    return TReturnValueDo();
}

} // namespace NAlice::NHollywoodFw::NSearch
