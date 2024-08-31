/*
    FACTOID SEARCH PROCESSOR

    All non-hanldled factiod objects should be prepared with old scenario

    This is a temporary processor, it will be removed when all factoid answers will be  moved to HWF
*/

#include "proc_calculator.h"

#include <alice/protos/data/scenario/search/fact.pb.h>

namespace NAlice::NHollywoodFw::NSearch {

namespace {

const TStringBuf SNIPPET_TYPE = "calculator";
constexpr TStringBuf ONTOFACTS_SCENARIO = "ontofacts";

} // anonymous namespace

ISearchProcessor::EProcessorRet TProcessorCalculator::ProcessSearchObject(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo,
    TSearchResult& results) const
{
    auto& logger = runRequest.Debug().Logger();
    NData::TSearchFactData factCard;

    // Try to extract main answer
    const auto mainSnippet = searchInfo.FindDocSnippetByType(SNIPPET_TYPE);
    if (!mainSnippet) {
        LOG_WARNING(logger) << "calculator snippet is not found, processor is not suitable";
        return ISearchProcessor::EProcessorRet::Unknown;
    }
    const auto& psp = searchInfo.GetProtoStructParserOptions();
    const auto answer = psp.GetValueString(*mainSnippet, "result");
    if (!answer) {
        LOG_WARNING(logger) << "calculator snippet found, but result is absent";
        return ISearchProcessor::EProcessorRet::Unknown;
    }
    LOG_DEBUG(logger) << "calculator snippet found, result is " << *answer;

    factCard.SetSnippetType(TString{SNIPPET_TYPE});
    factCard.SetText(*answer);
    factCard.SetQuestion(runRequest.Input().GetUtterance());
    // Calculator facts doesnt have image factCard.SetImage("");
    // Calculator facts doesnt have factCard.SetSearchUrl("");
    // Calculator facts doesnt have factCard.SetUrl("");
    // Calculator facts doesnt have factCard.SetHostname("");
    // Calculator facts doesnt have factCard.SetTitle("");
    results.SearchFeatures.SetFoundCalculatorFact(1);
    runRequest.AI().OverrideProductScenarioName(ONTOFACTS_SCENARIO);
    runRequest.AI().OverrideIntent("factoid");

    results.RenderArgs.SetVoiceAnswer(*answer);
    results.RenderArgs.SetTextAnswer(*answer);
    results.RenderArgs.SetPhrase("render_fact_calculator");

    *results.ScenarioRenderCard.MutableSearchFactData() = std::move(factCard);
    return ISearchProcessor::EProcessorRet::Success;
}

} // namespace NAlice::NHollywoodFw::NSearch
