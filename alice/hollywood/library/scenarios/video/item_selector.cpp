#include "item_selector.h"

#include <alice/megamind/protos/scenarios/begemot.pb.h>

using namespace NAlice::NHollywoodFw::NVideo;
namespace NAlice::NHollywood::NVideo {

TMaybe<ItemSelector> ItemSelector::TrySelectFromGallery(const NHollywoodFw::TRunRequest& runRequest, const TDeviceState& deviceState) {
    auto& logger = runRequest.Debug().Logger();

    TStringBuf semanticFrameName;
    TMaybe<ui32> number;
    double confidence;

    TFrameSelectVideoByNumber frameSelectByNumber(runRequest.Input());
    TFrameSelectVideoByText frameSelectByText(runRequest.Input());

    auto galleries = GetSelectionGalleriesFromDeviceState(deviceState);
    if (!galleries) {
        LOG_WARN(logger) << "Failed to get galleries from device state";
        return Nothing();
    }

    if (frameSelectByNumber.Defined()) {
        number = frameSelectByNumber.SlotVideoNumber.Value;
        confidence = 1.0; // default value for number always
        semanticFrameName = NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_NUMBER;
    } else if (frameSelectByText.Defined() && runRequest.GetDataSource(EDataSourceType::BEGEMOT_ITEM_SELECTOR_RESULT)->HasBegemotItemSelectorResult()) {
        const NScenarios::TBegemotItemSelectorResult& result = runRequest.GetDataSource(EDataSourceType::BEGEMOT_ITEM_SELECTOR_RESULT)->GetBegemotItemSelectorResult();
        if (auto begemotResult = GetNumberFromBegemotItemSelectorResult(result, logger)) {
            number = begemotResult->first;
            confidence = begemotResult->second;
            semanticFrameName = NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT;
        }
    } else {
        LOG_ERROR(logger) << "Unknown type of selection";
        return Nothing();
    }

    if (!number) {
        LOG_DEBUG(logger) << "Failed to get item number for gallery select";
        return Nothing();
    }
    LOG_INFO(logger) << "Item selection number = " << number;

    if (auto selectedItem = SelectItemFromGalleryByNumber(*galleries, *number)) {
        if (selectedItem->HasVideoItem() && selectedItem->GetVideoItem().GetProviderItemId().Empty()) {
            LOG_ERR(logger) << "Item from gallery has not providerItemId: " << selectedItem->GetVideoItem().ShortUtf8DebugString();
            return Nothing();
        }

        return ItemSelector(*selectedItem, semanticFrameName, confidence); // success!
    } else {
        LOG_WARN(logger) << "Selected item not found in gallery";
        return Nothing();
    }
}

TMaybe<NProtoBuf::RepeatedPtrField<TSearchResultGallery>> ItemSelector::GetSelectionGalleriesFromDeviceState(const TDeviceState& deviceState) {
    if (!deviceState.GetVideo().HasTvInterfaceState()) {
        return Nothing();
    } else if (deviceState.GetVideo().GetTvInterfaceState().HasSearchResultsScreen()) {
        return deviceState.GetVideo().GetTvInterfaceState().GetSearchResultsScreen().GetGalleries();
    } else if (deviceState.GetVideo().GetTvInterfaceState().HasExpandedCollectionScreen()) {
        return deviceState.GetVideo().GetTvInterfaceState().GetExpandedCollectionScreen().GetGalleries();
    } else return Nothing();
}

TMaybe<std::pair<ui32, double>> ItemSelector::GetNumberFromBegemotItemSelectorResult(const NScenarios::TBegemotItemSelectorResult& begemotSelectorResult, TRTLogger& logger) {
    for (const auto& gallery : begemotSelectorResult.GetGalleries()) {
        if (gallery.GetGalleryName() == "video_gallery") {
            for (const auto& item : gallery.GetItems()) {
                if (item.GetIsSelected()) {
                    LOG_INFO(logger) << "Selected index is "<< item.GetAlias() << " with confidence " << item.GetScore();
                    return std::make_pair(FromString<ui32>(item.GetAlias()), item.GetScore());
                }
            }
        }
    }
    LOG_WARN(logger) << "No item index in BegemotItemSelectorResult";
    return Nothing();
}

TMaybe<TSearchResultItem> ItemSelector::SelectItemFromGalleryByNumber(const google::protobuf::RepeatedPtrField<TSearchResultGallery>& galleries, int number) {
    for (const auto& gallery: galleries) {
        if (!gallery.GetVisible()) {
            continue;
        }
        for (const auto& item : gallery.GetItems()) {
            if (item.GetNumber() == number) {
                return item;
            }
        }
    }
    return Nothing();
}

TVideoItem EnrichedVideoItem(const TVideoItem& origItem) {
    auto item = origItem;
    if (item.GetSourceHost() == "youtube.com") {
        *item.MutableSourceHost() = "www.youtube.com";
    }
    return item;
}

TOpenItemSceneArgs ItemSelector::MakePreselectArgs() const {
    TOpenItemSceneArgs args;
    if (SelectedItem.HasVideoItem()) {
        *args.MutableVideoItem() = EnrichedVideoItem(SelectedItem.GetVideoItem());
    } else if (SelectedItem.HasPersonItem()) {
        *args.MutableCarouselItem()->MutablePersonItem() = SelectedItem.GetPersonItem();
    } else if (SelectedItem.HasCollectionItem()) {
        *args.MutableCarouselItem()->MutableCollectionItem() = SelectedItem.GetCollectionItem();
    }

    args.MutablePreselectedInfo()->SetIntent(ToString(SemanticFrameName));
    args.MutablePreselectedInfo()->SetConfidence(Confidence);
    return args;
}

} // namespace NAlice::NHollywood::NVideo
