#include "item_selector.h"
#include "alice/protos/data/search_result/search_result.pb.h"

#include <alice/library/json/json.h>
#include <alice/library/parsed_user_phrase/parsed_sequence.h>
#include <alice/library/parsed_user_phrase/utils.h>
#include <alice/library/proto/proto_struct.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>

#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

#include <alice/protos/data/video/video.pb.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/hash_set.h>
#include <util/string/builder.h>


namespace NBASS::NVideoCommon {

using namespace NAlice;
using namespace NAlice::NVideoCommon;

namespace {

constexpr float THRESHOLD = 0.47;
constexpr TItemSelectionResult FAILED_ITEM_SELECTION = {-1, -1};

constexpr const ELanguage lang = ELanguage::LANG_RUS; // todo(ddale) take lang from the request or autodetect it

struct TItemToSelect {
    TString Name;
    TString NormalizedName;
    TString ChannelName;
    TString ProgramName;
    int Position;
    bool IsVisible;
};

void FillVisibleItemsFromGallery(const TSearchResultGallery& gallery, THashSet<int>& visibleItems) {
    for (size_t itemIndex = 0; itemIndex < gallery.ItemsSize(); ++itemIndex) {
        const auto& item = gallery.GetItems(itemIndex);

        TString name;
        TString normalizedName;
        if (item.HasVideoItem()) {
            name = item.GetVideoItem().GetName();
            normalizedName = item.GetVideoItem().GetNormalizedName();
        } else if (item.HasPersonItem()) {
            name = item.GetPersonItem().GetName();
        } else if (item.HasCollectionItem()) {
            name = item.GetCollectionItem().GetTitle();
        }

        TItemToSelect currentItem {
            name,
            normalizedName,
            TString(),
            TString(),
            item.GetNumber(),
            item.GetVisible()
        };

        if (currentItem.IsVisible) {
            visibleItems.insert(currentItem.Position);
        }
    }
}

void CollectItemsToSelect(const TDeviceState& deviceStateProto, TVector<TItemToSelect>& itemsToSelect, THashSet<int>& visibleItems) {
    const auto currentScreen = CurrentScreenId(deviceStateProto);

    if (IsWebViewGalleryScreen(currentScreen) || IsWebViewSeasonGalleryScreen(currentScreen) || IsWebViewChannelsScreen(currentScreen)) {
        const google::protobuf::ListValue galleryItems = GetWebViewGalleryItems(deviceStateProto.GetVideo().GetViewState());
        bool isWebViewChannelsScreen = IsWebViewChannelsScreen(currentScreen);
        for (const auto& elem : galleryItems.values()) {
            const auto& item = elem.struct_value();
            TProtoStructParser parser;
            TItemToSelect currentItem{parser.GetValueString(item, "title", /* defaultValue= */ ""),
                                      TString(),
                                      (isWebViewChannelsScreen ? parser.GetValueString(item, "name", /* defaultValue= */ "") : TString()),
                                      (isWebViewChannelsScreen ? parser.GetValueString(item, "tv_episode_name", /* defaultValue= */ "") : TString()),
                                      parser.GetValueInt(item, "number", /* defaultValue= */ 0),
                                      parser.GetValueBool(item, "active", /* defaultValue= */ false)};
            if (currentItem.IsVisible) {
                visibleItems.insert(currentItem.Position);
            }
            itemsToSelect.emplace_back(currentItem);
        }
    } else if (currentScreen == EScreenId::SearchResults && deviceStateProto.GetVideo().HasTvInterfaceState()) {
        const auto& tvInterfaceStateProto = deviceStateProto.GetVideo().GetTvInterfaceState();
        if (!tvInterfaceStateProto.HasSearchResultsScreen()) {
            return;
        }
        const auto& searchResults = tvInterfaceStateProto.GetSearchResultsScreen();

        for (size_t galleryIndex = 0; galleryIndex < searchResults.GalleriesSize(); ++galleryIndex) {
            FillVisibleItemsFromGallery(searchResults.GetGalleries(galleryIndex), visibleItems);
        }
    } else if (currentScreen == EScreenId::TvExpandedCollection && deviceStateProto.GetVideo().HasTvInterfaceState()) {
        const auto& tvInterfaceStateProto = deviceStateProto.GetVideo().GetTvInterfaceState();
        if (!tvInterfaceStateProto.HasExpandedCollectionScreen()) {
            return;
        }
        const auto& expandedCollections = tvInterfaceStateProto.GetExpandedCollectionScreen();

        for (size_t galleryIndex = 0; galleryIndex < expandedCollections.GalleriesSize(); ++galleryIndex) {
            FillVisibleItemsFromGallery(expandedCollections.GetGalleries(galleryIndex), visibleItems);
        }
    } else {
        const auto& screenStateProto = deviceStateProto.GetVideo().GetScreenState();
        if (screenStateProto.VisibleItemsSize()) {
            for (const auto visibleItemId : screenStateProto.GetVisibleItems()) {
                visibleItems.insert(visibleItemId + 1);
            }
        }

        int position = 0;
        for (const auto &itemProto : screenStateProto.GetRawItems()) {
            itemsToSelect.emplace_back(TItemToSelect{itemProto.GetName(),
                                                     itemProto.GetNormalizedName(),
                                                     TString(),
                                                     TString(),
                                                     ++position,
                                                     visibleItems.contains(position)});
        }
    }
}

} // namespace

TItemSelectionResult SelectVideoFromGallery(const TDeviceState& deviceStateProto,
                                            const TString& videoText,
                                            const TLogAdapter& logger)
{
    THashSet<int> visibleItems;
    TVector<TItemToSelect> itemsToSelect;
    CollectItemsToSelect(deviceStateProto, itemsToSelect, visibleItems);

    if (itemsToSelect.empty()) {
        return FAILED_ITEM_SELECTION;
    }

    float bestScore = -1;
    int bestItem = -1;
    int bestIndex = -1;
    NParsedUserPhrase::TParsedSequence query(videoText);
    const auto currentScreen = CurrentScreenId(deviceStateProto);

    for (size_t i = 0; i < itemsToSelect.size(); ++i) {
        const auto& item = itemsToSelect[i];
        if (item.Name.empty()) {
            continue;
        }

        TVector<TString> synonyms {
            ToString(item.Position),
            TStringBuilder() << "номер " << item.Position,
            TStringBuilder() << "вариант " << item.Position,
            TStringBuilder() << "цифру " << item.Position,
        };
        if (IsChannelsScreen(currentScreen) || visibleItems.empty() || item.IsVisible) {
            // for non-TV gallery, we want to match invisible items only by number (for less confusion)
            if (!item.NormalizedName.empty()) {
                synonyms.push_back(item.NormalizedName);
            } else if (IsWebViewChannelsScreen(currentScreen)) {
                // push separately channel name and program name
                synonyms.push_back(NNlu::TRequestNormalizer::Normalize(lang, item.ChannelName));
                synonyms.push_back(NNlu::TRequestNormalizer::Normalize(lang, item.ProgramName));
            } else {
                synonyms.push_back(NNlu::TRequestNormalizer::Normalize(lang, item.Name));
            }
            // todo(ddale) for tv gallery, add tv_episode_name as well
        }

        for (const auto& synonym: synonyms) {
            NParsedUserPhrase::TParsedSequence document(synonym);
            float score1 = query.Match(document); // how many query words are in the document
            float score2 = document.Match(query); // how many document words are in the query
            float score = (score1 + score2) / 2;
            LOG_ADAPTER_DEBUG(logger) << "Trying item: " << synonym << " {" << item.Name << "} vs " <<  videoText
                                      << " = " << score1 << " / " << score2 << Endl;
            if (score > bestScore) {
                bestScore = score;
                bestItem = item.Position;
                bestIndex = i;
            }
        }
    }

    LOG_ADAPTER_DEBUG(logger) << "Finally selected item " << bestItem << " with score " << bestScore << " with index " << bestIndex << Endl;

    if (bestScore < THRESHOLD) {
        return FAILED_ITEM_SELECTION;
    }

    return TItemSelectionResult{bestIndex + 1, bestScore}; // best index + 1 as they start from 1
}

} // namespace NBASS::NVideoCommon
