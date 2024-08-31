/*
    FACTOID SEARCH PROCESSOR

    Location/POI facts processing
*/

#include "proc_factdist.h"

#include <alice/protos/data/scenario/search/fact.pb.h>

namespace NAlice::NHollywoodFw::NSearch {

namespace {

constexpr TStringBuf SNIPPET_TYPE = "distance_fact";
constexpr TStringBuf ONTOFACTS_SCENARIO = "ontofacts";

} // anonymous namespace

ISearchProcessor::EProcessorRet TProcessorFactDistance::ProcessSearchObject(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo, TSearchResult& results) const
{
    auto& logger = runRequest.Debug().Logger();
    // Try to extract factiod for
    const auto mainFact = searchInfo.FindFactoidByType(SNIPPET_TYPE);
    if (!mainFact) {
        LOG_DEBUG(logger) << "`distance_fact` fact is not found, processor is not suitable";
        return ISearchProcessor::EProcessorRet::Unknown;
    }
    const auto& psp = searchInfo.GetProtoStructParserOptions();
    const auto distanceMeters = psp.GetValueDouble(*mainFact, "data.distance");
    if (!distanceMeters) {
        LOG_WARNING(logger) << "`distance_fact` fact is found, but data.distance is absent";
        return ISearchProcessor::EProcessorRet::Unknown;
    }
    LOG_DEBUG(logger) << "`distance_fact` found, answer is " << *distanceMeters;

    NJson::TJsonValue val(NJson::EJsonValueType::JSON_MAP);
    val["value"] = *distanceMeters;
    const auto answer = runRequest.Nlg().RenderPhrase("search", "render_fact_distance", val);
    LOG_DEBUG(logger) << "`Final answer is " << answer.Text;

    NData::TSearchFactData factCard;
    factCard.SetSnippetType(TString{SNIPPET_TYPE});
    factCard.SetText(answer.Text);
    factCard.SetQuestion(runRequest.Input().GetUtterance());

    results.SearchFeatures.SetFactFromWizplaces(1);
    results.SearchFeatures.SetFoundDistanceFact(1);
    runRequest.AI().OverrideProductScenarioName(ONTOFACTS_SCENARIO);

    results.RenderArgs.SetVoiceAnswer(answer.Voice);
    results.RenderArgs.SetTextAnswer(answer.Text);

    *results.ScenarioRenderCard.MutableSearchFactData() = std::move(factCard);
    return ISearchProcessor::EProcessorRet::Success;
}

} // namespace NAlice::NHollywoodFw::NSearch
