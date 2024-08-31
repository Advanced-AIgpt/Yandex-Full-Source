/*
    FACTOID SEARCH PROCESSOR

    Location/POI facts processing
*/

#include "proc_factsuggest.h"
#include <alice/hollywood/library/scenarios/search/utils/utils.h>

#include <alice/protos/data/scenario/search/fact.pb.h>

#include <util/charset/utf8.h>

namespace NAlice::NHollywoodFw::NSearch {

namespace {

constexpr TStringBuf SNIPPET_TYPE = "suggest_fact";
constexpr TStringBuf ONTOFACTS_SCENARIO = "ontofacts";

} // anonymous namespace

ISearchProcessor::EProcessorRet TProcessorFactSuggest::ProcessSearchObject(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo,
    TSearchResult& results) const
{
    auto& logger = runRequest.Debug().Logger();
    // Try to extract factiod for
    const auto mainFact = searchInfo.FindFactoidByType(SNIPPET_TYPE);
    if (!mainFact) {
        LOG_DEBUG(logger) << "`suggest_fact` fact is not found, processor is not suitable";
        return ISearchProcessor::EProcessorRet::Unknown;
    }
    return TProcessorFactSuggest::Parse(runRequest, searchInfo, *mainFact, results);
}

ISearchProcessor::EProcessorRet TProcessorFactSuggest::Parse(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo, const google::protobuf::Struct& mainFact,
    TSearchResult& results)
{
    auto& logger = runRequest.Debug().Logger();
    const auto& psp = searchInfo.GetProtoStructParserOptions();
    TMaybe<TString> text;
    const auto subtype = psp.GetValueString(mainFact, "subtype", "");
    if (subtype.Empty() || subtype == "suggest_fact" || subtype == "wikipedia_fact") {
        if (psp.TestKey(mainFact, "text") == TProtoStructParser::EResult::String) {
            text = psp.GetValueString(mainFact, "text");
        } else {
            text = psp.EnumerateStringArray(psp.GetArray(mainFact, "text"), "\n");
        }
    } else if (subtype == "quotes") {
        if (psp.TestKey(mainFact, "text") == TProtoStructParser::EResult::String) {
            text = psp.GetValueString(mainFact, "text");
        } else {
            // TODO: Check this is real request!
            TStringBuilder sb;
            psp.EnumerateKeys(mainFact, "text.0.[]", [psp, &sb](const google::protobuf::Struct& obj) -> bool {
                const auto currency = psp.GetValueString(obj, "currency");
                const auto amount = psp.GetValueString(obj, "text");
                if (currency && amount) {
                    sb << *amount << ' ' << *currency;
                }
                return false;
            });
            if (!sb.Empty()) {
                text = TString{sb};
            }
        }
    } else {
        LOG_WARNING(logger) << "suggest_fact` fact: undefined subtype: '" << subtype << "'. Processor is not suitable";
        return ISearchProcessor::EProcessorRet::Unknown;
    }

    if (!text || text->Empty()) {
        auto text2 = psp.GetValueString(mainFact, "goodwinResponse.text");
        if (text2) {
            text = text2;
        }
    }
    if (!text || text->Empty() || !IsUtf(*text)) {
        LOG_WARNING(logger) << "suggest_fact` fact: undefined text. Processor is not suitable";
        return ISearchProcessor::EProcessorRet::Unknown;
    }
    text = NHollywood::NSearch::RemoveHighlight(*text);
    LOG_DEBUG(logger) << "suggest_fact`: found text '" << *text << '\'';

    // Prepare card
    NData::TSearchFactData factCard;
    factCard.SetSnippetType(TString{SNIPPET_TYPE});
    factCard.SetText(*text);
    factCard.SetQuestion(runRequest.Input().GetUtterance());
    factCard.SetHostname(psp.GetValueString(mainFact, "path.items.0.text", ""));

    // Select an image: from object answer, from factoid, from gallery (?)
    auto mainSnippet = searchInfo.FindDocSnippetByType("entity_search");
    TMaybe<NData::TSingleImage> image;
    if (mainSnippet) {
        image = searchInfo.GetMainAvatar(*mainSnippet);
    }
    if (!image) {
        mainSnippet = searchInfo.FindDocSnippetByType("companies");
        if (mainSnippet) {
            image = searchInfo.GetMainAvatar(*mainSnippet, "data.GeoMetaSearchData.features.0.properties.Photos.Photos.0");
        }
    }
    if (image) {
        results.AiObject.MutableSearchGenericInfo()->SetImageObjectAnswer(image->GetUrlAvatar().GetUrl());
    }
    if (!image) {
        image = searchInfo.GetMainAvatar(mainFact, "gallery[0].image");
        if (image) {
            results.AiObject.MutableSearchGenericInfo()->SetImageFactoid(image->GetUrlAvatar().GetUrl());
        }
    }
    if (!image) {
        // Looking for image in other possible places
        const auto construct = searchInfo.FindConstruct(
            {TSearchResultParser::EUseDatasource::Docs,
             TSearchResultParser::EUseDatasource::DocsRight},
            "image_snippet_one");
        image = searchInfo.GetMainAvatar(construct, "images.0");
        if (image) {
            results.AiObject.MutableSearchGenericInfo()->SetImageOther(image->GetUrlAvatar().GetUrl());
        }
    }
    if (image) {
        LOG_DEBUG(logger) << "Added image from " << image->GetUrlAvatar().GetUrl();
        factCard.SetImage(image->GetUrlAvatar().GetUrl());
    }

    factCard.SetSearchUrl("");
    factCard.SetUrl(psp.GetValueString(mainFact, "url", ""));

    auto title = psp.GetValueString(mainFact, "article_name");
    if (!title) {
        title = psp.GetValueString(mainFact, "question");
    }
    if (!title) {
        title = psp.GetValueString(mainFact, "headline");
    }
    if (title) {
        factCard.SetTitle(*title);
    }
    const auto voiceInfo = psp.GetValueString(mainFact, "voiceInfo.ru.0.text");

    if (voiceInfo) {
        results.RenderArgs.SetVoiceAnswer(*voiceInfo);
    }
    results.RenderArgs.SetTextAnswer(*text);

    // AddFactoidPhone(factoidData, factoid, true);
    // AddRelatedFactPromo(factoidData, factoid);
    results.SearchFeatures.SetFactFromWizplaces(1);
    results.SearchFeatures.SetFoundSuggestFact(1);
    runRequest.AI().OverrideProductScenarioName(ONTOFACTS_SCENARIO);

    *results.ScenarioRenderCard.MutableSearchFactData() = std::move(factCard);
    return ISearchProcessor::EProcessorRet::Success;
}

} // namespace NAlice::NHollywoodFw::NSearch
