/*
    FACTOID SEARCH PROCESSOR

    Location/POI facts processing
*/

#include "proc_factentity.h"

#include <alice/protos/data/scenario/search/fact.pb.h>

namespace NAlice::NHollywoodFw::NSearch {

namespace {

constexpr TStringBuf SNIPPET_TYPE = "entity-fact";
constexpr TStringBuf ONTOFACTS_SCENARIO = "ontofacts";

} // anonymous namespace

ISearchProcessor::EProcessorRet TProcessorFactEntity::ProcessSearchObject(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo,
    TSearchResult& results) const
{
    auto& logger = runRequest.Debug().Logger();
    // Try to extract factiod for
    auto mainFact = searchInfo.FindFactoidByType(SNIPPET_TYPE);
    if (mainFact) {
        results.SearchFeatures.SetFactFromWizplaces(1);
    } else {
        mainFact = searchInfo.FindDocSnippetByType(SNIPPET_TYPE);
        if (!mainFact) {
            LOG_DEBUG(logger) << "`entity-fact` fact is not found, processor is not suitable";
            return ISearchProcessor::EProcessorRet::Unknown;
        }
        results.SearchFeatures.SetFactFromDocs(1);
    }
    const auto& psp = searchInfo.GetProtoStructParserOptions();
    TStringBuilder textResult;
    psp.EnumerateKeys(*mainFact, "requested_facts.item.0.value.[]", [psp, &textResult](const google::protobuf::Struct& obj) -> bool {
        const auto text = psp.GetValueString(obj, "text");
        if (text) {
            if (textResult.Empty()) {
                textResult << *text;
            } else {
                textResult << ", " << *text;
            }
        }
        // Continue enumeration
        return false;
    });
    if (textResult.Empty()) {
        LOG_DEBUG(logger) << "`entity_fact` fact is found, but no text available";
        return ISearchProcessor::EProcessorRet::Unknown;
    }

    NData::TSearchFactData factCard;
    factCard.SetSnippetType(TString{SNIPPET_TYPE});
    factCard.SetText(textResult);
    factCard.SetQuestion(runRequest.Input().GetUtterance());

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
    factCard.SetSearchUrl(psp.GetValueString(*mainFact, "requested_fact.item.0.value.0.search_request", ""));
    factCard.SetUrl(psp.GetValueString(*mainFact, "base_info.url", ""));
    factCard.SetHostname(psp.GetValueString(*mainFact, "requested_fact.item.0.value.0.onto_source", ""));
    factCard.SetTitle(psp.GetValueString(*mainFact, "requested_fact.item.0.value.0.text", ""));
    results.SearchFeatures.SetFoundEntityFact(1);
    runRequest.AI().OverrideProductScenarioName(ONTOFACTS_SCENARIO);
    runRequest.AI().OverrideIntent("factoid");

    results.RenderArgs.SetVoiceAnswer(textResult);
    results.RenderArgs.SetTextAnswer(textResult);

    *results.ScenarioRenderCard.MutableSearchFactData() = std::move(factCard);
    return ISearchProcessor::EProcessorRet::Success;
}

} // namespace NAlice::NHollywoodFw::NSearch
