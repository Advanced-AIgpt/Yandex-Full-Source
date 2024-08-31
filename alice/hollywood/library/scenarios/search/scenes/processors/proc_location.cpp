/*
    LOCATION/POI SEARCH PROCESSOR
*/

#include "proc_location.h"

#include <alice/protos/data/scenario/search/richcard.pb.h>

namespace NAlice::NHollywoodFw::NSearch {

namespace {

const TStringBuf SNIPPET_TYPE = "companies";

} // anonymous namespace

ISearchProcessor::EProcessorRet TProcessorSearchLocation::ProcessSearchObject(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo,
    TSearchResult& results) const
{
    // Try to extract main answer
    const auto mainSnippet = searchInfo.FindDocSnippetByType(SNIPPET_TYPE);
    if (!mainSnippet) {
        return ISearchProcessor::EProcessorRet::Unknown;
    }

    const auto& psp = searchInfo.GetProtoStructParserOptions();
    LOG_DEBUG(runRequest.Debug().Logger()) << "Processing snippet : " << SNIPPET_TYPE << "; subtype: " << psp.GetValueString(*mainSnippet, "subtype", "");
    const auto companyDefList = psp.GetArray(*mainSnippet, "data.GeoMetaSearchData.features");
    if (companyDefList.values().size() == 0) {
        return ISearchProcessor::EProcessorRet::Unknown;
    }
    // Note companyDefList may contains multiple objects, right now we are working with 1st object only!
    NData::TSearchRichCardData card;
    auto& logger = runRequest.Debug().Logger();
    const auto companyDef = psp.GetKey(*mainSnippet, "data.GeoMetaSearchData.features.0");

    //
    // Prepare header
    //
    {
        NData::TSearchRichCardData::THeader header;

        header.SetText(psp.GetValueString(companyDef, "properties.name", ""));
        TString extraText;
        psp.EnumerateKeys(companyDef, "properties.CompanyMetaData.Categories.[]", [psp, &extraText](const google::protobuf::Struct& obj) -> bool {
            const auto extra = psp.GetValueString(obj, "name");
            if (extra) {
                if (!extraText.Empty()) {
                    extraText.append(", ");
                }
                extraText.append(*extra);
            }
            return false;
        });
        header.SetExtraText(extraText);
        const auto img = searchInfo.GetImage(psp.GetKey(companyDef, "properties.BusinessImages.Logo.urlTemplate"));
        if (img) {
            *header.MutableImage() = std::move(img->GetUrlAvatar());
        }
        header.SetSearchUrl(psp.GetValueString(companyDef, "properties.search_request", ""));
        header.SetUrl(psp.GetValueString(companyDef, "properties.CompanyMetaData.url", ""));
        // N/A header.SetHostname();
        // N/A header.SetTitle();
        header.SetRating(psp.GetValueString(companyDef, "properties.BusinessRating.score", ""));
        header.SetExtraText2(psp.GetValueString(companyDef, "properties.CompanyMetaData.Hours.State.text", ""));
        *card.MutableHeader() = std::move(header);
    }

    //
    // Prepare fact list
    //
    {
        NData::TSearchRichCardData::TBlock textBlock;
        textBlock.SetBlockType(NData::TSearchRichCardData_TBlock::Main);
        NData::TFactList factList;
        auto siteUrl = psp.GetValueString(companyDef, "properties.CompanyMetaData.url");
        if (!siteUrl) {
            siteUrl = psp.GetValueString(companyDef, "properties.CompanyMetaData.urls.0");
        }
        if (siteUrl) {
            NData::TFactList::TFact fact;
            // TODO [DD] need Localization
            fact.SetFactText("Сайт");
            fact.SetTextAnswer(*siteUrl);
            *factList.AddFacts() = std::move(fact);
        }
        const auto phone0 = psp.GetValueString(companyDef, "properties.CompanyMetaData.Phones.0.formatted");
        if (phone0) {
            NData::TFactList::TFact fact;
            // TODO [DD] need Localization
            fact.SetFactText("Телефон");
            fact.SetTextAnswer(*phone0);
            *factList.AddFacts() = std::move(fact);
        }
        auto addr = psp.GetValueString(companyDef, "properties.CompanyMetaData.Address.formatted");
        if (!addr) {
            addr = psp.GetValueString(companyDef, "properties.CompanyMetaData.address");
        }
        if (addr) {
            NData::TFactList::TFact fact;
            // TODO [DD] need Localization
            fact.SetFactText("Адрес");
            fact.SetTextAnswer(*addr);
            *factList.AddFacts() = std::move(fact);
        }
        TString features;
        psp.EnumerateKeys(companyDef, "properties.CompanyMetaData.Features.[]", [psp, &features, &factList, &logger](const google::protobuf::Struct& obj) -> bool {
            const auto type = psp.GetValueString(obj, "type", "");
            const auto name = psp.GetValueString(obj, "name", "");
            if (type == "bool") {
                if (!name.Empty() && psp.GetValueBool(obj, "value", false)) {
                    if (!features.Empty()) {
                        features.append(", ");
                        features.append(name);
                    }
                }
            } else if (type == "text") {
                // Add as is
                NData::TFactList::TFact fact;
                fact.SetFactText(name);
                fact.SetTextAnswer(psp.GetValueString(obj, "value", ""));
                *factList.AddFacts() = std::move(fact);
            } else if (type == "enum") {
                // Add list of internal values
                const auto valueList = psp.GetArray(obj, "values");
                const auto text = psp.EnumerateStringArray(valueList, ", ");
                if (text) {
                    NData::TFactList::TFact fact;
                    fact.SetFactText(name);
                    fact.SetTextAnswer(*text);
                    *factList.AddFacts() = std::move(fact);
                }
            } else {
                LOG_WARN(logger) << "Unsupported feature type: " << type;
            }
            return false;
        });
        if (!features.Empty()) {
            NData::TFactList::TFact fact;
            fact.SetFactText("Особенности");
            fact.SetTextAnswer(features);
            *factList.AddFacts() = std::move(fact);
        }

        // Subway stops
        NData::TFactList::TMultiText multiText;
        psp.EnumerateKeys(companyDef, "properties.Stops.items.[]", [psp, &multiText](const google::protobuf::Struct& obj) -> bool {
            const auto stationName = psp.GetValueString(obj, "name");
            const auto metrolineName = psp.GetValueString(obj, "Line.name");
            const auto metrolineColor = psp.GetValueString(obj, "Style.color");
            if (stationName && metrolineName && metrolineColor) {
                NData::TSimpleText text;
                text.SetText(*stationName);
                text.MutableSymbol()->SetType(NData::TSimpleText_TTextSymbol_ESymbolType_Subway);
                text.MutableSymbol()->SetColor(*metrolineColor);
                *multiText.AddTextAnswer() = std::move(text);
            }
            return false;
        });
        if (!multiText.GetTextAnswer().empty()) {
            NData::TFactList::TFact fact;
            fact.SetFactText("Метро");
            *fact.MutableMultiTextAnswer() = std::move(multiText);
            *factList.AddFacts() = std::move(fact);
        }

        if (!factList.GetFacts().empty()) {
            NData::TSearchRichCardData::TBlock::TSection section;
            *section.MutableFactList() = std::move(factList);
            *textBlock.AddSections() = std::move(section);
            *card.AddBlocks() = std::move(textBlock);
        }
    }

    //
    // Add image gallery
    //
    {
        const auto imgg = searchInfo.GetImagesGallery(companyDef, "properties.Photos.Photos.[]");
        if (imgg) {
            NData::TSearchRichCardData::TBlock block;
            NData::TSearchRichCardData::TBlock::TSection section;
            block.SetTitle(""); // TODO psp.GetValueString(*mainSnippet, "data.base_info.subtitle", ""));
            block.SetBlockType(NData::TSearchRichCardData_TBlock::Gallery);
            *section.MutableGallery() = std::move(*imgg);
            *block.AddSections() = std::move(section);
            *card.AddBlocks() = std::move(block);
        }
    }

    //
    // Add see also
    //
    {
        NData::TCompanies similarPlaces;
        psp.EnumerateKeys(companyDef, "properties.RelatedPlaces.SimilarPlaces.Company.[]", [psp, searchInfo, &similarPlaces](const google::protobuf::Struct& obj) -> bool {
            NData::TCompany company;

            company.SetId(psp.GetValueString(obj, "logId", ""));
            company.SetName(psp.GetValueString(obj, "name", ""));
            const auto imgg = searchInfo.GetImage(psp.GetKey(obj, "photoUrlTemplate"));
            if (imgg) {
                *company.MutableImage() = imgg->GetUrlAvatar();
            }
            company.SetUrl(psp.GetValueString(obj, "uri", ""));
            company.SetDescription(""); // N/A for SimilarPlaces
            company.SetAddress(psp.GetValueString(obj, "address", ""));
            company.SetWorkingHours(psp.GetValueString(obj, "Hours.text", ""));
            company.SetRating(psp.GetValueString(obj, "rating", ""));
            *similarPlaces.AddCompanies() = std::move(company);
            // Continue enumeration
            return false;
        });

        if (!similarPlaces.GetCompanies().empty()) {
            NData::TSearchRichCardData::TBlock block;
            NData::TSearchRichCardData::TBlock::TSection section;
            block.SetTitle("См. также");
            block.SetBlockType(NData::TSearchRichCardData_TBlock::Companies);
            *section.MutableCompanies() = std::move(similarPlaces);
            *block.AddSections() = std::move(section);
            *card.AddBlocks() = std::move(block);
        }
    }

    *results.ScenarioRenderCard.MutableSearchRichCardData() = std::move(card);
    return ISearchProcessor::EProcessorRet::Success;
}

} // namespace NAlice::NHollywoodFw::NSearch
