/*
    GENERIC SEARCH PROCESSOR

    type: entity_search
*/

#include "proc_generic.h"

#include <alice/protos/data/scenario/search/richcard.pb.h>

#include <util/generic/set.h>

namespace NAlice::NHollywoodFw::NSearch {

namespace {

const TStringBuf SNIPPET_TYPE = "entity_search";

} // anonymous namespace

ISearchProcessor::EProcessorRet TProcessorSearchGeneric::ProcessSearchObject(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo,
    TSearchResult& results) const
{
    auto& logger = runRequest.Debug().Logger();
    NData::TSearchRichCardData card;

    // Try to extract main answer
    const auto mainSnippet = searchInfo.FindDocSnippetByType(SNIPPET_TYPE);
    if (!mainSnippet) {
        LOG_WARNING(logger) << "entity_search snippet is not found, processor is not suitable";
        return ISearchProcessor::EProcessorRet::Unknown;
    }

    const auto& psp = searchInfo.GetProtoStructParserOptions();
    const auto type = psp.GetValueString(*mainSnippet, "data.base_info.type");
    if (!type) {
        LOG_WARNING(logger) << "Base infotype is not found, try to continue as is";
    } else {
        LOG_INFO(logger) << "Base infotype is: " << *type;
        card.SetCardType(*type);
    }
    // Note type may have following values:
    // - Hum
    // - ... (TODO)

    // TODO: need to check also "data.voiceInfo.ru.0.lang" - should be "ru-RU"
    const auto mainAnswer = psp.GetValueString(*mainSnippet, "data.voiceInfo.ru.0.text");
    if (!mainAnswer) {
        LOG_DEBUG(logger) << "Main answer in snippet is not found, Base Search Scenario is not applicable";
        return ISearchProcessor::EProcessorRet::Unknown;
    }
    results.RenderArgs.SetVoiceAnswer(*mainAnswer);
    results.RenderArgs.SetTextAnswer(*mainAnswer);

    //
    // Prepare header
    //
    {
        NData::TSearchRichCardData::THeader header;

        header.SetText(psp.GetValueString(*mainSnippet, "data.base_info.name", ""));
        header.SetExtraText(psp.GetValueString(*mainSnippet, "data.base_info.subtitle", ""));
        const auto img = searchInfo.GetImage(psp.GetKey(*mainSnippet, "data.base_info.image"));
        if (img) {
            *header.MutableImage() = std::move(img->GetUrlAvatar());
        }
        // TODO: header.SetSearchUrl();
        auto url = psp.GetValueString(*mainSnippet, "data.base_info.description_source.url");
        if (!url) {
            url = psp.GetValueString(*mainSnippet, "data.base_info.source.url");
        }
        if (!url) {
            url = psp.GetValueString(*mainSnippet, "data.base_info.sources.0.url");
        }
        if (url) {
            header.SetUrl(*url);
        } else {
            LOG_WARNING(logger) << "Url information can't be found for all possible pathes";
        }

        auto hostname = psp.GetValueString(*mainSnippet, "data.base_info.description_source.name");
        if (!hostname) {
            hostname = psp.GetValueString(*mainSnippet, "data.base_info.source.name");
        }
        if (!hostname) {
            hostname = psp.GetValueString(*mainSnippet, "data.base_info.sources.0.name");
        }
        if (hostname) {
            header.SetHostname(*hostname);
        } else {
            LOG_WARNING(logger) << "Hostname information can't be found for all possible pathes";
        }
        header.SetSearchUrl(psp.GetValueString(*mainSnippet, "data.base_info.search_request", ""));
        header.SetTitle(psp.GetValueString(*mainSnippet, "data.base_info.title", ""));
        header.SetRating(psp.GetValueString(*mainSnippet, "data.base_info.ysr_org_rating.score", ""));
        // TODO: header.SetExtraText2()
        *card.MutableHeader() = std::move(header);
    }

    //
    // Prepare main answer and fact table
    //
    {
        const auto factAnswer = psp.GetValueString(*mainSnippet, "data.base_info.description");
        if (!factAnswer) {
            LOG_WARNING(logger) << "Main block is not found, Base Search Scenario is not applicable";
            return ISearchProcessor::EProcessorRet::Unknown;
        }
        NData::TSimpleText baseAnswer;
        baseAnswer.SetText(*factAnswer);
        auto img = searchInfo.GetMainAvatar(*mainSnippet);
        if (img) {
            *baseAnswer.MutableImage() = std::move(img->GetUrlAvatar());
        }
        baseAnswer.SetSnippetType(psp.GetValueString(*mainSnippet, "data.base_info.type", ""));
        baseAnswer.SetSearchUrl(psp.GetValueString(*mainSnippet, "data.base_info.search_request", ""));
        baseAnswer.SetUrl(psp.GetValueString(*mainSnippet, "data.base_info.description_source.url", ""));
        baseAnswer.SetHostname(psp.GetValueString(*mainSnippet, "data.base_info.description_source.name", ""));
        baseAnswer.SetTitle(psp.GetValueString(*mainSnippet, "data.base_info.title", ""));
        // TODO - check and set OriginalLanguage

        NData::TSearchRichCardData::TBlock::TSection section;
        *section.MutableText() = std::move(baseAnswer);

        NData::TSearchRichCardData::TBlock textBlock;
        textBlock.SetBlockType(NData::TSearchRichCardData_TBlock::Main);
        textBlock.SetTitle("Главное"); // TODO: NLG
        *textBlock.AddSections() = std::move(section);

        // Add facts table
        NData::TFactList factList;
        psp.EnumerateKeys(*mainSnippet, "data.wiki_snippet.item.[]", [psp, &factList](const google::protobuf::Struct& obj) -> bool {
            const auto sizeOfAnswers = psp.GetArray(obj, "value").values_size();
            if (sizeOfAnswers <= 0) {
                return false;
            }
            NData::TFactList::TFact fact;
            fact.SetFactText(psp.GetValueString(obj, "name",""));
            if (sizeOfAnswers == 1) {
                fact.SetTextAnswer(psp.GetValueString(obj, "value.0.text",""));
            } else {
                // Use multiple text answers to format output
                NData::TFactList::TMultiText multiText;
                psp.EnumerateKeys(obj, "value.[]", [psp, &multiText](const google::protobuf::Struct& objfact) -> bool {
                    NData::TSimpleText textAnswer;
                    textAnswer.SetSnippetType("");
                    textAnswer.SetText(psp.GetValueString(objfact, "text",""));
                    textAnswer.SetSearchUrl(psp.GetValueString(objfact, "search_request",""));
                    *multiText.AddTextAnswer() = std::move(textAnswer);
                    return false;
                });
                *fact.MutableMultiTextAnswer() = std::move(multiText);
            }
            *factList.AddFacts() = std::move(fact);
            // Continue Enumeration
            return false;
        });
        if (!factList.GetFacts().empty()) {
            NData::TSearchRichCardData::TBlock::TSection section;
            *section.MutableFactList() = std::move(factList);
            *textBlock.AddSections() = std::move(section);
        }
        *card.AddBlocks() = std::move(textBlock);
    }

    // Save all other related objects to log file
    TString relatedTypes;
    psp.EnumerateKeys(*mainSnippet, "data.related_object.[]", [psp, &relatedTypes](const google::protobuf::Struct& obj) -> bool {
        relatedTypes.append(TStringBuilder{} << '\'' << psp.GetValueString(obj, "type", "") << "\' ");
        // Continue Enumeration
        return false;
    });
    // Probably, see https://a.yandex-team.ru/arcadia/search/wizard/entitysearch/card/objlist.h?rev=r9564126#L28 for all object types
    LOG_INFO(logger) << "Related object types: " << relatedTypes;

    //
    // Add image gallery
    //
    {
        const auto imgg = searchInfo.GetImagesGallery(*mainSnippet);
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
    // Add other related objects
    //
    auto fn = [psp, &searchInfo, &card, &logger](const google::protobuf::Struct& obj) -> bool {
        const TString type = psp.GetValueString(obj, "type", "");
        const TString avatarType = psp.GetValueString(obj, "avatar_type", "");

        NData::TSearchRichCardData::TBlock block;
        NData::TSearchRichCardData::TBlock::TSection section;
        const TString title = psp.GetValueString(obj, "list_name", "");
        block.SetTitle(title);
        block.SetTitleNavigation(title);

        // Select type for block
        NData::TSearchRichCardData_TBlock::EBlockType blockType = NData::TSearchRichCardData_TBlock::Custom;
        TSet<TString> childType;
        psp.EnumerateKeys(obj, "object.[]", [psp, &childType](const google::protobuf::Struct& subobj) -> bool {
            const TMaybe<TString> type = psp.GetValueString(subobj, "type");
            if (type && !childType.contains(*type)) {
                childType.insert(*type);
            }
            return false;
        });
        TString childType0 = "Unrecognized";
        switch (childType.size()) {
            case 0:
                LOG_WARNING(logger) << "Object entry doesn't contain any known objects, skip this block. " <<
                    "Type: " << type << "; avatar_type: " << avatarType << ';';
                return false;
            case 1:
                childType0 = *childType.begin();
                LOG_INFO(logger) << "Found related object, type: " << type << "; avatar_type: " << avatarType <<
                    "; child type: " << childType0 << ';';
                break;
            case 2:
                //
                // Child objects has some different kinds.
                // Now we can process following cases:
                // - Hum + Band (think that this block contain Bands + Single artists)
                // - Org + Geo (think that this block contain Geo + organizations as a geo)
                if (childType.contains("Hum") && childType.contains("Band")) {
                    childType0 = "Band";
                    LOG_INFO(logger) << "Found two related object, type: " << type << "; avatar_type: " << avatarType <<
                        "; child type: Hum + Band, result will be Band";
                    break;
                }
                if (childType.contains("Geo") && childType.contains("Org")) {
                    childType0 = "Band";
                    LOG_INFO(logger) << "Found two related object, type: " << type << "; avatar_type: " << avatarType <<
                        "; child type: Geo + Org, result will be Org";
                    break;
                }
                // don't break
            default: {
                TStringBuilder sb;
                for (const auto& c : childType) {
                    sb << c << "; ";
                }
                LOG_WARNING(logger) << "Object entry contains multiple objects, skip this block. " <<
                    "Type: " << type << "; avatar_type: " << avatarType << "; Objects: " << sb;
                return false;
            }
        }

        if (childType0 == "Film") {
            blockType = NData::TSearchRichCardData_TBlock::Movie;
            const auto movies = searchInfo.GetRelatedMovies(obj, "object.[]");
            if (!movies) {
                return false;
            }
            *section.MutableVideoMovies() = std::move(*movies);
        } else if (childType0 == "Hum") {
            blockType = NData::TSearchRichCardData_TBlock::Persons;
            const auto persons = searchInfo.GetRelatedPersons(obj, "object.[]");
            if (!persons) {
                return false;
            }
            *section.MutablePersons() = std::move(*persons);
        } else if (childType0 == "Music") {
            if (type == "clips") {
                blockType = NData::TSearchRichCardData_TBlock::Clips;
                const auto clips = searchInfo.GetRelatedClips(obj, "object.[]");
                if (!clips) {
                    return false;
                }
                *section.MutableVideoClips() = std::move(*clips);
            } else if (type == "proj") { // ? Albums
                blockType = NData::TSearchRichCardData_TBlock::Albums;
                const auto albums = searchInfo.GetMusicAlbums(obj, "object.[]");
                if (!albums) {
                    return false;
                }
                *section.MutableMusicAlbums() = std::move(*albums);
            } else if (type == "toptracks") {
                blockType = NData::TSearchRichCardData_TBlock::Tracks;
                const auto tracks = searchInfo.GetMusicTracks(obj, "object.[]");
                if (!tracks) {
                    return false;
                }
                *section.MutableMusicTracks() = std::move(*tracks);
            } else {
                LOG_WARNING(logger) << "Undefined child type: " << childType0 <<
                    "; Type: " << type << "; avatar_type: " << avatarType << ';';
                return false;
            }
        } else if (childType0 == "Band") {
            blockType = NData::TSearchRichCardData_TBlock::Band;
            const auto bands = searchInfo.GetRelatedBands(obj, "object.[]", true);
            if (!bands) {
                return false;
            }
            *section.MutableMusicBands() = std::move(*bands);
        } else if (childType0 == "Geo") {
            blockType = NData::TSearchRichCardData_TBlock::Places;
            const auto places = searchInfo.GetGeoPlaces(obj, "object.[]", true);
            if (!places) {
                return false;
            }
            *section.MutableGeoPlaces() = std::move(*places);
        } else if (childType0 == "Org") {
            blockType = NData::TSearchRichCardData_TBlock::Companies;
            const auto comp = searchInfo.GetCompanies(obj, "object.[]");
            if (!comp) {
                return false;
            }
            *section.MutableCompanies() = std::move(*comp);
        } else if (childType0 == "Text" || childType0 == "Book") {
            blockType = NData::TSearchRichCardData_TBlock::Books;
            const auto books = searchInfo.GetBooks(obj, "object.[]");
            if (!books) {
                return false;
            }
            *section.MutableBooks() = std::move(*books);
        } else if (childType0 == "Soft") {
            // Software objects hanlded as books
            blockType = NData::TSearchRichCardData_TBlock::Software;
            const auto soft = searchInfo.GetBooks(obj, "object.[]");
            if (!soft) {
                return false;
            }
            *section.MutableBooks() = std::move(*soft);
        } else {
            // Nothing known
            LOG_WARNING(logger) << "Undefined child type: " << childType0 <<
                "; Type: " << type << "; avatar_type: " << avatarType << ';';
            return false;
        }
        block.SetBlockType(blockType);
        *block.AddSections() = std::move(section);
        *card.AddBlocks() = std::move(block);
        // continue enumeration
        return false;
    };

    // Clips
    psp.EnumerateKeys(*mainSnippet, "data.clips.[]", fn);
    // Tracks
    psp.EnumerateKeys(*mainSnippet, "data.tracks.[]", fn);
    // Other related objects
    psp.EnumerateKeys(*mainSnippet, "data.related_object.[]", fn);

    *results.ScenarioRenderCard.MutableSearchRichCardData() = std::move(card);
    return ISearchProcessor::EProcessorRet::Success;
}

} // namespace NAlice::NHollywoodFw::NSearch
