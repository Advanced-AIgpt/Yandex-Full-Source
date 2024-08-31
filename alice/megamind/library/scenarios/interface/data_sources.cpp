#include "blackbox.h"
#include "data_sources.h"
#include "quasar_devices_info.h"

#include <alice/megamind/library/raw_responses/raw_responses.h>
#include <alice/megamind/library/scenarios/utils/begemot_fixlist_converter.h>
#include <alice/megamind/library/scenarios/utils/begemot_item_selector.h>
#include <alice/megamind/library/scenarios/utils/markup_converter.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/common/quasar_devices.pb.h>
#include <alice/megamind/protos/scenarios/begemot.pb.h>
#include <alice/megamind/protos/scenarios/notification_state.pb.h>

#include <alice/protos/data/device/info.pb.h>
#include <alice/protos/data/location/room.pb.h>
#include <alice/protos/data/location/group.pb.h>

#include <alice/library/geo/protos/user_location.pb.h>
#include <alice/library/json/json.h>
#include <alice/library/scenarios/data_sources/data_sources.h>

#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>

#include <util/generic/hash.h>

namespace NAlice::NMegamind {

namespace {

void FillEntitySearchData(NScenarios::TEntitySearch& entitySearch, const TEntitySearchResponse& response) {
    entitySearch.SetRawJson(JsonToString(response.RawResponse()));
}

void FillVinsWizardRules(NScenarios::TVinsWizardRules& vinsWizardRules, const NJson::TJsonValue& rules) {
    vinsWizardRules.SetRawJson(JsonToString(rules));
}

void AddDataSourceToContext(EDataSourceType type, const google::protobuf::Message& proto, TItemProxyAdapter& itemProxyAdapter) {
    itemProxyAdapter.PutIntoContext(proto, NScenarios::GetDataSourceContextName(type));
}

const THashSet<EDataSourceType> UNSENT_DATA_SOURCES = {
    EDataSourceType::EMPTY_DEVICE_STATE,
    EDataSourceType::BEGEMOT_BEGGINS_RESULT,
    EDataSourceType::WEB_SEARCH_DOCS,
    EDataSourceType::WEB_SEARCH_DOCS_RIGHT,
    EDataSourceType::WEB_SEARCH_WIZPLACES,
    EDataSourceType::WEB_SEARCH_SUMMARIZATION,
    EDataSourceType::WEB_SEARCH_RENDERRER,
    EDataSourceType::WEB_SEARCH_WIZARD,
    EDataSourceType::WEB_SEARCH_BANNER,
};

} // namespace

const NScenarios::TDataSource& TDataSources::GetDataSource(EDataSourceType type) {
    std::call_once(DataSources[type].Flag, [this, type]() { CreateDataSource(type); });
    return *DataSources[type].DataSource;
}

void TDataSources::CreateDataSource(EDataSourceType type) {
    auto& dataSource = DataSources[type].DataSource.ConstructInPlace();
    if (UNSENT_DATA_SOURCES.contains(type)) {
        return;
    }
    TStringBuilder err{};
    switch (type) {
        case EDataSourceType::BLACK_BOX: {
            if (!Responses) {
                err << "no BlackBox response";
                break;
            }
            *dataSource.MutableUserInfo() = CreateBlackBoxData(Responses->BlackBoxResponse());
            break;
        }
        case EDataSourceType::USER_LOCATION: {
            if (UserLocation) {
                dataSource.MutableUserLocation()->CopyFrom(UserLocation->BuildProto());
                break;
            }
            err << "no UserLocation";
            break;
        }
        case EDataSourceType::BEGEMOT_EXTERNAL_MARKUP: {
            if (!Responses) {
                err << "no Wizard response";
                break;
            }
            if (Responses->WizardResponse().GetProtoResponse().GetExternalMarkup().HasJSON()) {
                NMegamind::ConvertExternalMarkup(
                    Responses->WizardResponse().GetProtoResponse().GetExternalMarkup().GetJSON(),
                    *dataSource.MutableBegemotExternalMarkup());
            } else {
                LOG_ERR(Logger) << "Failed to find data for datasource BEGEMOT_EXTERNAL_MARKUP";
            }
            break;
        }
        case EDataSourceType::DIALOG_HISTORY: {
            if (DialogHistory) {
                NScenarios::TDialogHistoryDataSource dialogHistoryDataSource;
                for (const auto& dialogTurn : DialogHistory->GetDialogTurns()) {
                    *dialogHistoryDataSource.AddPhrases() = dialogTurn.Request;
                    *dialogHistoryDataSource.AddPhrases() = dialogTurn.Response;

                    auto& dialogTurnProto = *dialogHistoryDataSource.AddDialogTurns();
                    dialogTurnProto.SetRequest(dialogTurn.Request);
                    dialogTurnProto.SetRewrittenRequest(dialogTurn.RewrittenRequest);
                    dialogTurnProto.SetResponse(dialogTurn.Response);
                    dialogTurnProto.SetScenarioName(dialogTurn.ScenarioName);
                    dialogTurnProto.SetServerTimeMs(dialogTurn.ServerTimeMs);
                    dialogTurnProto.SetClientTimeMs(dialogTurn.ClientTimeMs);
                }
                *dataSource.MutableDialogHistory() = std::move(dialogHistoryDataSource);
                break;
            }
            err << "no DialogHistory";
            break;
        }
        case EDataSourceType::RESPONSE_HISTORY: {
            if (Layout) {
                NScenarios::TResponseHistoryDataSource responseHistoryDataSource;
                auto* prevResponse = responseHistoryDataSource.MutablePrevResponse();
                *prevResponse->MutableLayout() = *Layout;
                if (Actions)
                    *prevResponse->MutableActions() = *Actions;

                *dataSource.MutableResponseHistory() = std::move(responseHistoryDataSource);
                break;
            }
            err << "no ResponseHistory";
            break;
        }
        case EDataSourceType::BEGEMOT_IOT_NLU_RESULT: {
            if (!Responses) {
                err << "no Wizard response";
                break;
            }
            if (Responses->WizardResponse().GetProtoResponse().GetAliceIot().HasResult()) {
                *dataSource.MutableBegemotIotNluResult() = Responses->WizardResponse().GetProtoResponse().GetAliceIot().GetResult();
            } else {
                LOG_ERR(Logger) << "Failed to find data for datasource IOT_NLU_RESULT";
            }
            break;
        }
        case EDataSourceType::GC_MEMORY_STATE: {
            if (!Responses) {
                err << "no Wizard response";
                break;
            }
            if (Responses->WizardResponse().GetProtoResponse().GetAliceGcMemoryStateUpdater().HasMemoryState()) {
                *dataSource.MutableBegemotGcMemoryState() = Responses->WizardResponse().GetProtoResponse().GetAliceGcMemoryStateUpdater().GetMemoryState();
            } else {
                LOG_ERR(Logger) << "Failed to find data for datasource GC_MEMORY_STATE";
            }
            break;
        }
        case EDataSourceType::BEGEMOT_ITEM_SELECTOR_RESULT: {
            if (!Responses) {
                err << "no Wizard response";
                break;
            }

            if (Responses->WizardResponse().GetProtoResponse().HasAliceItemSelector()) {
                ConvertBegemotItemSelector(
                    Responses->WizardResponse().GetProtoResponse().GetAliceItemSelector(),
                    *dataSource.MutableBegemotItemSelectorResult());
            } else {
                LOG_ERR(Logger) << "Failed to find data for datasource BEGEMOT_ITEM_SELECTOR_RESULT";
            }
            break;
        }
        case EDataSourceType::BEGEMOT_FIXLIST_RESULT: {
            if (!Responses) {
                err << "no Wizard response";
                break;
            }
            if (Responses->WizardResponse().GetProtoResponse().HasAliceFixlist()) {
                ConvertBegemotFixlist(
                    Responses->WizardResponse().GetProtoResponse().GetAliceFixlist(),
                    *dataSource.MutableBegemotFixlistResult());
            } else {
                LOG_ERR(Logger) << "Failed to find data for datasource BEGEMOT_FIXLIST_RESULT";
            }
            break;
        }
        case EDataSourceType::WEB_SEARCH_DOCS: {
            break;
        }
        case EDataSourceType::WEB_SEARCH_DOCS_RIGHT: {
            break;
        }
        case EDataSourceType::WEB_SEARCH_WIZPLACES: {
            break;
        }
        case EDataSourceType::WEB_SEARCH_SUMMARIZATION: {
            break;
        }
        case EDataSourceType::WEB_SEARCH_RENDERRER: {
            break;
        }
        case EDataSourceType::ENTITY_SEARCH: {
            if (!Responses) {
                err << "no EntitySearch";
                break;
            }
            FillEntitySearchData(*dataSource.MutableEntitySearch(), Responses->EntitySearchResponse());
            break;
        }
        case EDataSourceType::VINS_WIZARD_RULES: {
            if (Responses) {
                NJson::TJsonValue rules{};
                if (UpdateVinsWizardRules(rules, Responses->WizardResponse().RawResponse())) {
                    FillVinsWizardRules(*dataSource.MutableVinsWizardRules(), rules);
                    break;
                }
            }
            err << "no VinsWizardRules";
            break;
        }
        case EDataSourceType::SMART_HOME_INFO: {
            if (SmartHomeInfo) {
                dataSource.MutableSmartHomeInfo()->CopyFrom(*SmartHomeInfo);
            } else {
                err << "no SmartHomeInfo";
            }
            break;
        }
        case EDataSourceType::VIDEO_VIEW_STATE: {
            if (VideoViewState) {
                dataSource.MutableVideoViewState()->MutableViewState()->CopyFrom(*VideoViewState);
            } else {
                err << "no VideoViewState";
            }
            break;
        }
        case EDataSourceType::NOTIFICATION_STATE: {
            if (NotificationState) {
                dataSource.MutableNotificationState()->CopyFrom(*NotificationState);
            } else {
                err << "no NotificationState";
            }
            break;
        }
        case EDataSourceType::ALICE4BUSINESS_DEVICE: {
            if (AuxiliaryConfig && AuxiliaryConfig->HasAlice4Business()) {
                dataSource.MutableAlice4BusinessConfig()->CopyFrom(AuxiliaryConfig->GetAlice4Business());
            } else {
                err << "no Alice4Business";
            }
            break;
        }
        case EDataSourceType::WEB_SEARCH_WIZARD: {
            break;
        }
        case EDataSourceType::WEB_SEARCH_BANNER: {
            break;
        }
        case EDataSourceType::DEVICE_STATE_NAVIGATOR: {
            if (DeviceState.HasNavigator()) {
                dataSource.MutableDeviceStateNavigator()->CopyFrom(DeviceState.GetNavigator());
                break;
            }
            err << "no device state navigator";
            break;
        }
        case EDataSourceType::SKILL_DISCOVERY_GC: {
            if (SkillDiscoverySaasCandidates && !SkillDiscoverySaasCandidates->GetSaasCandidate().empty()) {
                dataSource.MutableSkillDiscoveryGcSaasCandidates()->CopyFrom(*SkillDiscoverySaasCandidates);
            } else {
                err << "no SkillDiscoverySaasCandidates";
            }
            break;
        }
        case EDataSourceType::IOT_USER_INFO: {
            if (IoTUserInfo) {
                dataSource.MutableIoTUserInfo()->CopyFrom(*IoTUserInfo);
            } else {
                err << "no IoTUserInfo";
            }
            break;
        }
        case EDataSourceType::QUASAR_DEVICES_INFO: {
            if (IoTUserInfo) {
                TQuasarDevicesInfo qdi;
                if (const auto status = CreateQuasarDevicesInfo(*IoTUserInfo, qdi)) {
                    err << status->ErrorMsg;
                } else {
                    qdi.Swap(dataSource.MutableQuasarDevicesInfo());
                }
            } else {
                err << "no IoTUserInfo to copy to QuasarDevicesInfo";
            }
            break;
        }
        case EDataSourceType::APP_INFO: {
            if (AppInfo) {
                dataSource.MutableAppInfo()->CopyFrom(*AppInfo);
            } else {
                err << "no AppInfo";
            }
            break;
        }
        case EDataSourceType::BEGEMOT_BEGGINS_RESULT:
            break;
        case EDataSourceType::RAW_PERSONAL_DATA: {
            if (RawPersonalData.Empty()) {
                err << "no raw personal data";
            } else {
                dataSource.SetRawPersonalData(RawPersonalData);
            }
            break;
        }
        case EDataSourceType::VIDEO_CURRENTLY_PLAYING: {
            if (VideoCurrentlyPlaying) {
                dataSource.MutableVideoCurrentlyPlaying()->MutableCurrentlyPlaying()->CopyFrom(*VideoCurrentlyPlaying);
            } else {
                err << "no VideoCurrentlyPlaying";
            }
            break;
        }
        case EDataSourceType::CONTACTS_LIST: {
            if (ContactsList) {
                dataSource.MutableContactsList()->CopyFrom(*ContactsList);
            } else {
                err << "no ContactsList";
            }
            break;
        }
        case EDataSourceType::ENVIRONMENT_STATE: {
            if (EnvironmentState) {
                dataSource.MutableEnvironmentState()->CopyFrom(*EnvironmentState);
            } else {
                err << "no EnvironmentState";
            }
            break;
        }
        case EDataSourceType::TANDEM_ENVIRONMENT_STATE: {
            if (TandemEnvironmentState) {
                dataSource.MutableTandemEnvironmentState()->CopyFrom(*TandemEnvironmentState);
            } else {
                err << "no TandemEnvironmentState";
            }
            break;
        }
        case EDataSourceType::WEB_SEARCH_REQUEST_META: {
            dataSource.MutableWebSearchRequestMeta()->SetQuery(WebSearchQuery);
            break;
        }
        case EDataSourceType::WHISPER_INFO: {
            if (WhisperInfo.Defined()) {
                auto& whisperInfoDataSource = *dataSource.MutableWhisperInfo();
                whisperInfoDataSource.SetIsAsrWhisper(WhisperInfo->IsAsrWhisper());
                whisperInfoDataSource.SetIsWhisperResponseAvailable(WhisperInfo->IsWhisper());
            } else {
                err << "no WhisperInfo";
            }
            break;
        }
        case EDataSourceType::GUEST_DATA: {
            if (GuestData.Defined()) {
                *dataSource.MutableGuestData() = *GuestData;
            } else {
                err << "no GuestData";
            }
            break;
        }
        case EDataSourceType::GUEST_OPTIONS: {
            if (GuestOptions.Defined()) {
                *dataSource.MutableGuestOptions() = *GuestOptions;
            } else {
                err << "no GuestOptions";
            }
            break;
        }
        default: {
            Y_ASSERT(false);
            err << "is unhandled data source";
        }
    }
    if (err) {
        LOG_ERROR(Logger) << "Failed to construct " << EDataSourceType_Name(type) << " data source: " << err;
        return;
    }
    AddDataSourceToContext(type, dataSource, ItemProxyAdapter);
}

} // namespace NAlice::NMegamind
