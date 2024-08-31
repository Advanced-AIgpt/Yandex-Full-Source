#include "begemot_item_selector.h"

namespace NAlice::NMegamind {

void ConvertBegemotItemSelector(const NBg::NProto::TAliceItemSelectorResult& aliceItemSelector,
                                NScenarios::TBegemotItemSelectorResult& begemotItemSelector)
{
    for (const auto& srcGallery : aliceItemSelector.GetGalleries()) {
        auto& dstGallery = *begemotItemSelector.AddGalleries();
        *dstGallery.MutableGalleryName() = srcGallery.GetGalleryName();
        for (const auto& srcItem : srcGallery.GetItems()) {
            auto& dstItem = *dstGallery.AddItems();
            dstItem.SetScore(srcItem.GetScore());
            dstItem.SetIsSelected(srcItem.GetIsSelected());
            dstItem.SetAlias(srcItem.GetAlias());
        }
    }
}

} // namespace NAlice::NMegamind
