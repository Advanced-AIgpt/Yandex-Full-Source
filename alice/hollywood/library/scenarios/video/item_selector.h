#pragma once
#include "video_utils.h"

#include <alice/hollywood/library/framework/framework.h>
#include <alice/protos/data/search_result/search_result.pb.h>
#include <alice/hollywood/library/scenarios/video/proto/video_scene_args.pb.h>

namespace NAlice::NHollywood::NVideo {

namespace {

//
// Semantic frame to find video by number
//
struct TFrameSelectVideoByNumber : public NHollywoodFw::TFrame {
    TFrameSelectVideoByNumber(const NHollywoodFw::TRequest::TInput& input)
        : TFrame(input, NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_NUMBER)
        , SlotVideoNumber(this, NAlice::NVideoCommon::SLOT_VIDEO_NUMBER)
    {
    }
    NHollywoodFw::TOptionalSlot<ui32> SlotVideoNumber;
};

struct TFrameSelectVideoByText : public NHollywoodFw::TFrame {
    TFrameSelectVideoByText(const NHollywoodFw::TRequest::TInput& input)
        : TFrame(input, NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT)
        , SlotVideoNumber(this, NAlice::NVideoCommon::SLOT_VIDEO_NUMBER)
    {
    }
    NHollywoodFw::TOptionalSlot<ui32> SlotVideoNumber;
};

} // anonimous namespace


class ItemSelector final {
public:
    static TMaybe<ItemSelector> TrySelectFromGallery(const NHollywoodFw::TRunRequest& runRequest, const TDeviceState& deviceState);

    NHollywoodFw::NVideo::TOpenItemSceneArgs MakePreselectArgs() const;
private:
    static TMaybe<NProtoBuf::RepeatedPtrField<TSearchResultGallery>> GetSelectionGalleriesFromDeviceState(const TDeviceState& deviceState);
    static TMaybe<std::pair<ui32, double>> GetNumberFromBegemotItemSelectorResult(const NScenarios::TBegemotItemSelectorResult& begemotSelectorResult, TRTLogger& logger);
    static TMaybe<TSearchResultItem> SelectItemFromGalleryByNumber(const google::protobuf::RepeatedPtrField<TSearchResultGallery>& galleries, int number);

    explicit ItemSelector(const TSearchResultItem& selectedItem, TStringBuf semanticFrameName, double confidence)
        : SelectedItem(selectedItem)
        , SemanticFrameName(semanticFrameName)
        , Confidence(confidence)
    {}

    const TSearchResultItem SelectedItem;
    const TStringBuf SemanticFrameName;
    const double Confidence;
};

} // namespace NAlice::NHollywood::NVideo
