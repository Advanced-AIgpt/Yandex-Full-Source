/*
    FACTOID SEARCH PROCESSOR

    All non-hanldled factiod objects should be prepared with old scenario

    Calories fact checker
*/

#include "proc_calories.h"

#include <alice/protos/data/scenario/search/fact.pb.h>

namespace NAlice::NHollywoodFw::NSearch {

namespace {

const TStringBuf SNIPPET_TYPE = "calories_fact";
constexpr TStringBuf ONTOFACTS_SCENARIO = "ontofacts";

} // anonymous namespace

ISearchProcessor::EProcessorRet TProcessorFactCalories::ProcessSearchObject(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo,
    TSearchResult& results) const
{
    auto& logger = runRequest.Debug().Logger();
    NData::TSearchFactData factCard;

    // Try to extract main answer
    auto mainFact = searchInfo.FindFactoidByType(SNIPPET_TYPE);
    if (!mainFact) {
        mainFact = searchInfo.FindDocSnippetByType(SNIPPET_TYPE);
    }
    if (!mainFact) {
        LOG_WARNING(logger) << "calories_fact snippet is not found, processor is not suitable";
        return ISearchProcessor::EProcessorRet::Unknown;
    }
    const auto& psp = searchInfo.GetProtoStructParserOptions();
    const auto answer = psp.GetValueString(*mainFact, "data.calories1");
    if (!answer) {
        LOG_WARNING(logger) << "calories_fact snippet found, but result is absent";
        return ISearchProcessor::EProcessorRet::Unknown;
    }
    LOG_DEBUG(logger) << "calories_fact snippet found, result is " << *answer;


    factCard.SetSnippetType(TString{SNIPPET_TYPE});
    factCard.SetText(*answer);
    factCard.SetQuestion(runRequest.Input().GetUtterance());
    factCard.SetUrl(psp.GetValueString(*mainFact, "url", ""));

    // Select an image: from object answer, from factoid, from gallery (?)
    auto mainSnippet = searchInfo.FindDocSnippetByType("entity_search");
    TMaybe<NData::TSingleImage> image;
    if (mainSnippet) {
        image = searchInfo.GetMainAvatar(*mainSnippet);
    }
    if (!image) {
        image = searchInfo.GetMainAvatar(*mainFact, "base_info.image");
    }
    if (image) {
        LOG_DEBUG(logger) << "Added image from " << image->GetUrlAvatar().GetUrl();
        factCard.SetImage(image->GetUrlAvatar().GetUrl());
    }

    auto title = psp.GetValueString(*mainFact, "article_name");
    if (!title) {
        title = psp.GetValueString(*mainFact, "question");
    }
    if (!title) {
        title = psp.GetValueString(*mainFact, "headline");
    }
    if (title) {
        factCard.SetTitle(*title);
    }
    // Calories facts doesnt have factCard.SetSearchUrl("");
    factCard.SetHostname(psp.GetValueString(*mainFact, "path.items.0.text", ""));
    const auto voiceInfo = psp.GetValueString(*mainFact, "voiceInfo.ru.0.text");
    if (voiceInfo) {
        results.RenderArgs.SetVoiceAnswer(*voiceInfo);
    }
    results.RenderArgs.SetTextAnswer(*answer);

    results.SearchFeatures.SetFoundCaloriesFact(1);
    runRequest.AI().OverrideProductScenarioName(ONTOFACTS_SCENARIO);
    runRequest.AI().OverrideIntent("factoid");

    *results.ScenarioRenderCard.MutableSearchFactData() = std::move(factCard);
    return ISearchProcessor::EProcessorRet::Success;
}

} // namespace NAlice::NHollywoodFw::NSearch
