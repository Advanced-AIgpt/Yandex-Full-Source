#include "add_widget_from_gallery.h"

#include <alice/library/proto/protobuf.h>

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/memento/proto/device_configs.pb.h>
#include <alice/protos/data/scenario/centaur/main_screen.pb.h>
#include <alice/protos/data/scenario/centaur/my_screen/widgets.pb.h>

namespace NAlice::NHollywood::NCombinators {

using namespace NAlice::NScenarios;

TAddWidgetFromGallery::TAddWidgetFromGallery(THwServiceContext& ctx, const TCombinatorRequest& combinatorRequest)
    : Ctx(ctx),
      Request(combinatorRequest, Ctx.Logger(), Ctx),
      CombinatorContextWrapper(Ctx, Request, ResponseForRenderer)
{
}

void TAddWidgetFromGallery::Do(const TSemanticFrame& addWidgetSemanticFrame) {
    TFrame frame = TFrame::FromProto(addWidgetSemanticFrame);
    const auto columnSlot = frame.FindSlot("column");
    const auto rowSlot = frame.FindSlot("row");
    const auto widgetConfigDataSlot = frame.FindSlot("widget_config_data_slot");

    Y_ENSURE(columnSlot.IsValid() && rowSlot.IsValid() && widgetConfigDataSlot.IsValid(),
        "Invalid data in semantic frame " << CENTAUR_ADD_WIDGET_FROM_GALLERY_FRAME << " (CENTAUR_ADD_WIDGET_FROM_GALLERY_FRAME).");

    const auto& newWidgetColumn = columnSlot.Get()->Value.As<i32>().GetRef();
    const auto& newWidgetRow = rowSlot.Get()->Value.As<i32>().GetRef();
    const auto& newWidgetConfigData = addWidgetSemanticFrame.GetTypedSemanticFrame().GetCentaurAddWidgetFromGallerySemanticFrame()
                                .GetWidgetConfigDataSlot().GetWidgetConfigDataValue();

    NMemento::TCentaurWidgetsDeviceConfig centaurWidgetsDeviceConfig;
    const auto sourceColumns = GetCentaurWidgetDeviceConfig(CombinatorContextWrapper).GetColumns();
    for (int column = 0; column < sourceColumns.size(); column++) {
        auto& destColumn = *centaurWidgetsDeviceConfig.AddColumns();
        const auto& sourceWidgets = sourceColumns[column].GetWidgetConfigs();
        for (int row = 0; row < sourceWidgets.size(); row++) {
            if (column == newWidgetColumn && row == newWidgetRow) {
                *destColumn.AddWidgetConfigs() = newWidgetConfigData;
            } else {
                *destColumn.AddWidgetConfigs() = sourceWidgets[row];
            }
        }
    }

    UpdateMementoCentaurWidgetsDeviceConfig(centaurWidgetsDeviceConfig, CombinatorContextWrapper);

    if (Request.HasExpFlag(PATCH_MAIN_SCREEN_EXP_FLAG_NAME)) { 
        *ResponseForRenderer.MutableResponseBody()->MutableLayout()->AddDirectives() =
            PrepareCollectMainScreenWithWidgetConfigCallbackDirective(newWidgetConfigData, newWidgetColumn, newWidgetRow);
    } else {
        *ResponseForRenderer.MutableResponseBody()->MutableLayout()->AddDirectives() = PrepareCollectMainScreenCallbackDirective();
    }

    Ctx.AddProtobufItemToApphostContext(ResponseForRenderer, RESPONSE_ITEM);
}

TDirective TAddWidgetFromGallery::PrepareCollectMainScreenCallbackDirective() {
    TParsedUtterance payload;
    payload.MutableTypedSemanticFrame()->MutableCentaurCollectMainScreenSemanticFrame();
    auto& analytics = *payload.MutableAnalytics();
    analytics.SetProductScenario(CENTAUR_MAIN_SCREEN_COMBINATOR_PSN);
    analytics.SetPurpose("refresh_main_screen");
    analytics.SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

    TDirective directive;
    auto* callBackDirective = directive.MutableCallbackDirective();
    callBackDirective->SetName("@@mm_semantic_frame");
    callBackDirective->SetIgnoreAnswer(false);
    *callBackDirective->MutablePayload() = MessageToStruct(payload);
    return directive;
}

TDirective TAddWidgetFromGallery::PrepareCollectMainScreenWithWidgetConfigCallbackDirective(const ::NAlice::NData::TCentaurWidgetConfigData& widgetConfigData, int column, int row) {
    TParsedUtterance payload;
    auto* frame = payload.MutableTypedSemanticFrame()->MutableCentaurCollectMainScreenSemanticFrame();
    frame->MutableWidgetConfigDataSlot()->MutableWidgetConfigDataValue()->CopyFrom(widgetConfigData);
    NData::TWidgetPosition mainScreenPosition;
    mainScreenPosition.SetColumn(column);
    mainScreenPosition.SetRow(row);
    frame->MutableWidgetMainScreenPosition()->MutableWidgetPositionValue()->CopyFrom(mainScreenPosition);
    
    auto& analytics = *payload.MutableAnalytics();
    analytics.SetProductScenario(CENTAUR_MAIN_SCREEN_COMBINATOR_PSN);
    analytics.SetPurpose("reload_widget");
    analytics.SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

    TDirective directive;
    auto* callBackDirective = directive.MutableCallbackDirective();
    callBackDirective->SetName("@@mm_semantic_frame");
    callBackDirective->SetIgnoreAnswer(false);
    *callBackDirective->MutablePayload() = MessageToStruct(payload);
    return directive;
}

} // namespace NAlice::NHollywood::NCombinators
