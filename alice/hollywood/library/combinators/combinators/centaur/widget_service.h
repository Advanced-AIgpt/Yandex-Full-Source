#pragma once

#include "combinator_context_wrapper.h"
#include "defs.h"

#include <alice/hollywood/library/combinators/request/request.h>
#include <alice/library/logger/logger.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/memento/proto/device_configs.pb.h>
#include <alice/protos/data/scenario/centaur/main_screen.pb.h>
#include <alice/protos/data/scenario/centaur/my_screen/widgets.pb.h>

namespace NAlice::NHollywood::NCombinators {


void CheckRequiredScenarios(TCombinatorContextWrapper& CombinatorContextWrapper);

TWidgetResponses PrepareWidgetResponses(TCombinatorContextWrapper& CombinatorContextWrapper);

void AddUserConfigs(
    NScenarios::TMementoChangeUserObjectsDirective& mementoDirective,
    NMemento::EDeviceConfigKey key,
    const NMemento::TCentaurWidgetsDeviceConfig& value,
    TCombinatorContextWrapper& CombinatorContextWrapper);

void UpdateMementoCentaurWidgetsDeviceConfig(
    NMemento::TCentaurWidgetsDeviceConfig centaurWidgetsDeviceConfig,
    TCombinatorContextWrapper& CombinatorContextWrapper);

NMemento::TCentaurWidgetsDeviceConfig PrepareDefaultCentaurWidgetsDeviceConfig(TCombinatorContextWrapper& CombinatorContextWrapper);

NMemento::TCentaurWidgetsDeviceConfig GetCentaurWidgetDeviceConfig(TCombinatorContextWrapper& CombinatorContextWrapper);

NData::TCentaurWidgetCardData MyScreenMusicCard(const TMaybe<TMusicData>& musicData, TCombinatorContextWrapper& CombinatorContextWrapper);

NData::TCentaurWidgetCardData MyScreenInfoCard();

NData::TCentaurWidgetCardData MyScreenVacantCard(int column, int row, TCombinatorContextWrapper& CombinatorContextWrapper);

TString OpenWidgetGalleryAction(int column, int row, TCombinatorContextWrapper& CombinatorContextWrapper);

google::protobuf::Any OpenWidgetGalleryActionTyped(int column, int row);

TString AddWidgetFromGalleryAction(const NData::TCentaurWidgetConfigData& widgetConfigData,
    const int column, const int row, TCombinatorContextWrapper& CombinatorContextWrapper);

google::protobuf::Any AddWidgetFromGalleryActionTyped(const NData::TCentaurWidgetConfigData& widgetConfigData, int column, int row);

TString DeleteWidgetAction(int column, int row, TCombinatorContextWrapper& CombinatorContextWrapper);

google::protobuf::Any DeleteWidgetActionTyped(int column, int row);

NData::TCentaurWidgetCardData MyScreenCard(
    const NData::TCentaurWidgetConfigData& widgetConfigData,
    const int column,
    const int row,
    const TWidgetResponses& WidgetResponses,
    TCombinatorContextWrapper& CombinatorContextWrapper,
    const TMaybe<TMusicData>& musicData = Nothing());

NData::TCentaurWidgetCardData MyScreenWidgetCard(
    const NData::TCentaurWidgetConfigData& widgetConfigData,
    const TWidgetResponses& WidgetResponses,
    TRTLogger& logger);

template <typename TScenarioResponse>
void AddCentaurScenarioWidgetData(
    TWidgetResponses& widgetResponses,
    const ::google::protobuf::Map<TString, TScenarioResponse>& scenarioResponses);

TMaybe<TMusicScenarioDataBlock> FindMusicDataBlock(const TMaybe<TMusicData>& musicData, TString blockType);
} // NAlice::NHollywood::NCombinators
