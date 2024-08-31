#include "widget_service.h"

#include <alice/hollywood/library/combinators/metrics/names.h>
#include <util/random/random.h>

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <google/protobuf/wrappers.pb.h>

namespace NAlice::NHollywood::NCombinators {

using namespace NAlice::NScenarios;

void CheckRequiredScenarios(TCombinatorContextWrapper& CombinatorContextWrapper) {
    for (const auto& scenario : REQUIRED_SCENARIOS) {
        if (!CombinatorContextWrapper.Request().GetScenarioRunResponses().contains(scenario) &&
            !CombinatorContextWrapper.Request().GetScenarioContinueResponses().contains(scenario))
        {
            CombinatorContextWrapper.Ctx().Sensors().IncRate(NMetrics::LabelsCombinatorMissedScenarios("CentaurMainScreen", scenario));
        }
    }
}

NMemento::TCentaurWidgetsDeviceConfig GetCentaurWidgetDeviceConfig(TCombinatorContextWrapper& CombinatorContextWrapper) {
    const auto fullMementoData = CombinatorContextWrapper.Ctx().GetProtoOrThrow<NMemento::TRespGetAllObjects>(AH_ITEM_FULL_MEMENTO_DATA);
    const auto& deviceId = CombinatorContextWrapper.Request().BaseRequestProto().GetClientInfo().GetDeviceId();
    const auto& surfaceConfigs = fullMementoData.GetSurfaceConfigs();
    const auto& deviceSurfaceConfigs = surfaceConfigs.find(deviceId);
    bool validConfigExists = false;
    
    NMemento::TCentaurWidgetsDeviceConfig centaurWidgetsDeviceConfig;
    if (deviceSurfaceConfigs != surfaceConfigs.end() && deviceSurfaceConfigs->second.HasCentaurWidgetsDeviceConfig()) {
        centaurWidgetsDeviceConfig = deviceSurfaceConfigs->second.GetCentaurWidgetsDeviceConfig();
        auto columns = centaurWidgetsDeviceConfig.GetColumns();
        if (columns.size() != 0 && !columns[0].MutableWidgetConfigs()->empty()) {
            validConfigExists = true;
        }
    }
    return validConfigExists ? centaurWidgetsDeviceConfig : PrepareDefaultCentaurWidgetsDeviceConfig(CombinatorContextWrapper);
}

NMemento::TCentaurWidgetsDeviceConfig PrepareDefaultCentaurWidgetsDeviceConfig(TCombinatorContextWrapper& CombinatorContextWrapper) {

    NMemento::TCentaurWidgetsDeviceConfig centaurWidgetsDeviceConfig;
    
    {
        auto& column = *centaurWidgetsDeviceConfig.AddColumns();
        auto& widgetConfigData = *column.AddWidgetConfigs();
        widgetConfigData.SetWidgetType("music");
        widgetConfigData.SetFixed(true);
    }
    {
        auto& column = *centaurWidgetsDeviceConfig.AddColumns();
        {
            auto& widgetConfigData = *column.AddWidgetConfigs();
            widgetConfigData.SetWidgetType("notification");
        }
        {
            auto& widgetConfigData = *column.AddWidgetConfigs();
            widgetConfigData.SetWidgetType("weather");
        }
    }
    {
        auto& column = *centaurWidgetsDeviceConfig.AddColumns();
        {
            auto& widgetConfigData = *column.AddWidgetConfigs();
            widgetConfigData.SetWidgetType("webview");
        }
        {
            auto& widgetConfigData = *column.AddWidgetConfigs();
            widgetConfigData.SetWidgetType("vacant");
        }
    }

    UpdateMementoCentaurWidgetsDeviceConfig(centaurWidgetsDeviceConfig, CombinatorContextWrapper);
    return centaurWidgetsDeviceConfig;
}

void UpdateMementoCentaurWidgetsDeviceConfig(NMemento::TCentaurWidgetsDeviceConfig centaurWidgetsDeviceConfig,
    TCombinatorContextWrapper& CombinatorContextWrapper) {
    auto& mementoDirective = *CombinatorContextWrapper.ResponseForRenderer().MutableResponseBody()
                                    ->AddServerDirectives()
                                    ->MutableMementoChangeUserObjectsDirective();
    AddUserConfigs(mementoDirective, NMemento::EDeviceConfigKey::DCK_CENTAUR_WIDGETS, centaurWidgetsDeviceConfig, CombinatorContextWrapper);
}

void AddUserConfigs(TMementoChangeUserObjectsDirective& mementoDirective, NMemento::EDeviceConfigKey key,
    const NMemento::TCentaurWidgetsDeviceConfig& value, TCombinatorContextWrapper& CombinatorContextWrapper) {
    NMemento::TDeviceConfigs deviceConfigs;
    deviceConfigs.SetDeviceId(CombinatorContextWrapper.Request().BaseRequestProto().GetClientInfo().GetDeviceId());
    auto& deviceConfigsKeyValuePair = *deviceConfigs.AddDeviceConfigs();
    deviceConfigsKeyValuePair.SetKey(key);

    if (deviceConfigsKeyValuePair.MutableValue()->PackFrom(value)) {
        *mementoDirective.MutableUserObjects()->AddDevicesConfigs() = std::move(deviceConfigs);
    } else {
        LOG_ERROR(CombinatorContextWrapper.Logger()) << "PackFrom failed for user config";
    }
}

NData::TCentaurWidgetCardData MyScreenCard(const NData::TCentaurWidgetConfigData& widgetConfigData,
    const int column, const int row, const TWidgetResponses& WidgetResponses,
    TCombinatorContextWrapper& CombinatorContextWrapper,const TMaybe<TMusicData>& musicData) {

    NData::TCentaurWidgetCardData card;
    TString widgetType = widgetConfigData.GetWidgetType();
    if (widgetType == "vacant") {
        card = MyScreenVacantCard(column, row, CombinatorContextWrapper);
    } else if (widgetType == "music") {
        card = MyScreenMusicCard(musicData, CombinatorContextWrapper);
    } else {
        card = MyScreenWidgetCard(widgetConfigData, WidgetResponses, CombinatorContextWrapper.Logger());
    }

    if (!widgetConfigData.GetFixed()) {
        if (CombinatorContextWrapper.Request().HasExpFlag(DELETE_WIDGET_EXP_FLAG_NAME)) {
            if (CombinatorContextWrapper.Request().HasExpFlag(TYPED_ACTION_EXP_FLAG_NAME)) {
                *card.MutableDeleteWidgetTypedAction() = DeleteWidgetActionTyped(column, row);
            } else {
                card.SetDeleteWidgetAction(DeleteWidgetAction(column, row, CombinatorContextWrapper));
            }
        } else {
            if (CombinatorContextWrapper.Request().HasExpFlag(TYPED_ACTION_EXP_FLAG_NAME)) {
                *card.MutableLongTapTypedAction() = OpenWidgetGalleryActionTyped(column, row);
            } else {
                card.SetLongTapAction(OpenWidgetGalleryAction(column, row, CombinatorContextWrapper));
            }
        }
    }

    NData::TWidgetPosition mainScreenPosition;
    mainScreenPosition.SetColumn(column);
    mainScreenPosition.SetRow(row);
    *card.MutableMainScreenPosition() = std::move(mainScreenPosition);
    
    return card;
}

NData::TCentaurWidgetCardData MyScreenWidgetCard(const NData::TCentaurWidgetConfigData& widgetConfigData, const TWidgetResponses& WidgetResponses,
    TRTLogger& logger) {
    NData::TCentaurWidgetCardData card;

    TString widgetType = widgetConfigData.GetWidgetType();
    const auto widgetTypeResponse = WidgetResponses.find(widgetType);
    if (widgetTypeResponse != WidgetResponses.end() && !widgetTypeResponse->second.empty()) {
        const auto& widgetCards = widgetTypeResponse->second;
        const auto& widgetCard = widgetCards.find(widgetConfigData.GetId());
        if (widgetCard != widgetCards.end()) {
            card = widgetCard->second;
        } else {
            LOG_ERROR(logger) << "No widgetCard for " << widgetType << " widget (id: " << widgetConfigData.GetId() << ")";
        }
    } else {
        LOG_ERROR(logger) << "No scenario response for " << widgetType << " main screen widget";
    }

    return card;
}

NData::TCentaurWidgetCardData MyScreenMusicCard(const TMaybe<TMusicData>& musicData, TCombinatorContextWrapper& CombinatorContextWrapper) {
    NData::TCentaurWidgetCardData card;
    auto& musicCardData = *card.MutableMusicCardData();

    const auto& musicDataBlock = FindMusicDataBlock(musicData, MY_SCREEN_TAB_MUSIC_BLOCK_TYPE);
    if (musicDataBlock.Defined() && !musicDataBlock->MusicScenarioData.empty()) {
        auto seed = CombinatorContextWrapper.Request().BaseRequestProto().GetRandomSeed();
        SetRandomSeed(seed);
        const auto rand = RandomNumber<unsigned short>(musicDataBlock->MusicScenarioData.size());
        const auto& musicScenarioData = musicDataBlock->MusicScenarioData[rand];
        musicCardData.SetName(musicScenarioData.Title);

        if (musicScenarioData.Modified.Defined()) {
            musicCardData.MutableModified()->set_value(*musicScenarioData.Modified);
        }
        musicCardData.SetCover(musicScenarioData.ImgUrl);

        if (CombinatorContextWrapper.Request().HasExpFlag(TYPED_ACTION_EXP_FLAG_NAME)) {
            if (musicScenarioData.TypedAction.Defined()) {
                *card.MutableTypedAction() = *musicScenarioData.TypedAction;
            }
        } else {
            card.SetAction(musicScenarioData.Action);
        }

    }

    return card;
}

NData::TCentaurWidgetCardData MyScreenInfoCard() {
    NData::TCentaurWidgetCardData card;
    card.MutableInfoCardData();

    return card;
}

NData::TCentaurWidgetCardData MyScreenVacantCard(int column, int row, TCombinatorContextWrapper& CombinatorContextWrapper) {
    NData::TCentaurWidgetCardData card;
    card.MutableVacantCardData();

    if (const auto mainScreenSemanticFrame = CombinatorContextWrapper.Request().Input().FindSemanticFrame(CENTAUR_COLLECT_MAIN_SCREEN_FRAME)) {
        TFrame frame = TFrame::FromProto(*mainScreenSemanticFrame);
        if (!frame.FindSlot("widget_gallery_position")) {
            if (CombinatorContextWrapper.Request().HasExpFlag(TYPED_ACTION_EXP_FLAG_NAME)) {
                *card.MutableTypedAction() = OpenWidgetGalleryActionTyped(column, row);
            } else {
                card.SetAction(OpenWidgetGalleryAction(column, row, CombinatorContextWrapper));
            }
        }
    }

    return card;
}

TString OpenWidgetGalleryAction(int column, int row, TCombinatorContextWrapper& CombinatorContextWrapper) {
    TFrameAction openWidgetGalleryAction;
    auto* parsedUtterance = openWidgetGalleryAction.MutableParsedUtterance();
    auto* frame = parsedUtterance->MutableTypedSemanticFrame()->MutableCentaurCollectMainScreenSemanticFrame();
    NData::TWidgetPosition widgetPosition;
    widgetPosition.SetColumn(column);
    widgetPosition.SetRow(row);
    frame->MutableWidgetGalleryPosition()->MutableWidgetPositionValue()->CopyFrom(widgetPosition);

    auto* analytics = parsedUtterance->MutableAnalytics();
    analytics->SetProductScenario(CENTAUR_MAIN_SCREEN_COMBINATOR_PSN);
    analytics->SetPurpose("open_widget_gallery_from_centaur_main_screen");
    analytics->SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

    const TString frameActionId = TStringBuilder{} << "OpenWidgetGalleryFromCentaurMainScreen_" << ToString(column) << ToString(row); 
    (*CombinatorContextWrapper.ResponseForRenderer().MutableResponseBody()->MutableFrameActions())[frameActionId] = openWidgetGalleryAction;

    return "@@mm_deeplink#" + frameActionId;
}

google::protobuf::Any OpenWidgetGalleryActionTyped(int column, int row) {
    TTypedSemanticFrame tsf;
    auto* frame = tsf.MutableCentaurCollectMainScreenSemanticFrame();

    NData::TWidgetPosition widgetPosition;
    widgetPosition.SetColumn(column);
    widgetPosition.SetRow(row);
    frame->MutableWidgetGalleryPosition()->MutableWidgetPositionValue()->CopyFrom(widgetPosition);

    google::protobuf::Any typedAction;
    typedAction.PackFrom(std::move(tsf));
    return typedAction;
}

TString AddWidgetFromGalleryAction(const NData::TCentaurWidgetConfigData& widgetConfigData,
    const int column, const int row, TCombinatorContextWrapper& CombinatorContextWrapper) {

    TFrameAction addWidgetFromGalleryAction;
    auto* parsedUtterance = addWidgetFromGalleryAction.MutableParsedUtterance();
    auto* frame = parsedUtterance->MutableTypedSemanticFrame()->MutableCentaurAddWidgetFromGallerySemanticFrame();
    frame->MutableColumn()->SetNumValue(column);
    frame->MutableRow()->SetNumValue(row);
    frame->MutableWidgetConfigDataSlot()->MutableWidgetConfigDataValue()->CopyFrom(widgetConfigData);

    auto* analytics = parsedUtterance->MutableAnalytics();
    analytics->SetProductScenario(CENTAUR_MAIN_SCREEN_COMBINATOR_PSN);
    analytics->SetPurpose("add_widget_from_gallery");
    analytics->SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

    const TString frameActionId = TStringBuilder{} << "AddWidgetFromGallery_" << ToString(column) << ToString(row)
            << widgetConfigData.GetWidgetType() << "_" << widgetConfigData.GetId();
    (*CombinatorContextWrapper.ResponseForRenderer().MutableResponseBody()->MutableFrameActions())[frameActionId] = addWidgetFromGalleryAction;

    return "@@mm_deeplink#" + frameActionId;
}

google::protobuf::Any AddWidgetFromGalleryActionTyped(const NData::TCentaurWidgetConfigData& widgetConfigData, int column, int row) {
    TTypedSemanticFrame tsf;
    auto* frame = tsf.MutableCentaurAddWidgetFromGallerySemanticFrame();
    frame->MutableColumn()->SetNumValue(column);
    frame->MutableRow()->SetNumValue(row);
    frame->MutableWidgetConfigDataSlot()->MutableWidgetConfigDataValue()->CopyFrom(widgetConfigData);

    google::protobuf::Any typedAction;
    typedAction.PackFrom(std::move(tsf));
    return typedAction;
}

TString DeleteWidgetAction(int column, int row, TCombinatorContextWrapper& CombinatorContextWrapper) {
    NData::TCentaurWidgetConfigData vacantWidgetConfig;
    vacantWidgetConfig.SetWidgetType("vacant");
    return AddWidgetFromGalleryAction(vacantWidgetConfig, column, row, CombinatorContextWrapper);
}

google::protobuf::Any DeleteWidgetActionTyped(int column, int row) {
    NData::TCentaurWidgetConfigData vacantWidgetConfig;
    vacantWidgetConfig.SetWidgetType("vacant");
    return AddWidgetFromGalleryActionTyped(vacantWidgetConfig, column, row);
}

TWidgetResponses PrepareWidgetResponses(TCombinatorContextWrapper& CombinatorContextWrapper) {
    TWidgetResponses widgetResponses;

    //run responses
    AddCentaurScenarioWidgetData<TScenarioRunResponse>(widgetResponses, CombinatorContextWrapper.Request().GetScenarioRunResponses());

    //continue responses
    AddCentaurScenarioWidgetData<TScenarioContinueResponse>(widgetResponses, CombinatorContextWrapper.Request().GetScenarioContinueResponses());

    THashMap<TString, NData::TCentaurWidgetCardData> widgetCards;
    widgetCards[""] = MyScreenInfoCard();
    widgetResponses.emplace("notification", std::move(widgetCards));
    
    return widgetResponses;
}

template <typename TScenarioResponse>
void AddCentaurScenarioWidgetData(TWidgetResponses& widgetResponses, const ::google::protobuf::Map<TString, TScenarioResponse>& scenarioResponses) {
    for (const auto& response : scenarioResponses) {
        auto& scenarioData = response.second.GetResponseBody().GetScenarioData();
        if (scenarioData.HasCentaurScenarioWidgetData()) {
            auto scenarioWidgetData = scenarioData.GetCentaurScenarioWidgetData();
            THashMap<TString, NData::TCentaurWidgetCardData> widgetCards;
            for (const auto& widgetCard : scenarioWidgetData.GetWidgetCards()) {
                widgetCards[widgetCard.GetId()] = widgetCard;
            }
            widgetResponses.emplace(scenarioWidgetData.GetWidgetType(), std::move(widgetCards));
        }
    }
}

TMaybe<TMusicScenarioDataBlock> FindMusicDataBlock(const TMaybe<TMusicData>& musicData, TString blockType) {
    if (!musicData.Defined()) {
        return Nothing();
    }
    for (const auto& musicDataBlock : musicData->musicDataBlocks) {
        if (musicDataBlock.Type == blockType) {
            return musicDataBlock;
        }
    }
    return Nothing();
}

} // NAlice::NHollywood::NCombinators
