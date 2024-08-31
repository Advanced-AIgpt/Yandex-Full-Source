#include "request_creator.h"
#include "intents.h"
#include "intent_classifier.h"
#include "utils.h"

#include <alice/bass/libs/logging_v2/bass_logadapter.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/feature_calculator.h>

#include <alice/library/json/json.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>

#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <alice/protos/api/nlu/generated/features.pb.h>

using namespace NAlice::NScenarios;
using namespace NAlice::NVideoCommon;
using namespace NBASS;
using namespace NBASS::NVideoCommon;
using namespace NVideoProtocol;

namespace {

TString GenClientId(const NAlice::NScenarios::TScenarioBaseRequest& request) {
    TStringBuilder builder;
    builder << request.GetClientInfo().GetAppId()
            << "/" << request.GetClientInfo().GetAppVersion();
    builder << " (";
    builder << request.GetClientInfo().GetDeviceManufacturer()
            << " " << request.GetClientInfo().GetDeviceModel();
    builder << "; " << request.GetClientInfo().GetPlatform()
            << " " << request.GetClientInfo().GetOsVersion();
    builder << ")";

    return builder;
}

NSc::TValue ParseClientInfo(const NAlice::TClientInfoProto& requestClientInfo) {
    NSc::TValue clientInfo;
    clientInfo["app_id"] = requestClientInfo.GetAppId();
    clientInfo["app_version"] = requestClientInfo.GetAppVersion();
    clientInfo["device_manufacturer"] = requestClientInfo.GetDeviceManufacturer();
    clientInfo["device_model"] = requestClientInfo.GetDeviceModel();
    clientInfo["platform"] = requestClientInfo.GetPlatform();
    clientInfo["os_version"] = requestClientInfo.GetOsVersion();
    return clientInfo;
}

NSc::TValue ParseLocation(const NAlice::TLocation& requestLocation) {
    NSc::TValue location;
    location["lon"] = requestLocation.GetLon();
    location["lat"] = requestLocation.GetLat();
    location["accuracy"] = requestLocation.GetAccuracy();
    location["recency"] = static_cast<ui64>(requestLocation.GetRecency());
    return location;
}

NSc::TValue ParseExperiments(const google::protobuf::Struct& requestExperiments) {
    NSc::TValue experiments;
    for (const auto& field : requestExperiments.fields()) {
        experiments[field.first] = field.second.string_value();
    }
    return experiments;
}

bool IsPornQuery(const NAlice::NScenarios::TScenarioBaseRequest& request) {
    for (const auto& nluFeature : request.GetNluFeatures()) {
        if (nluFeature.GetFeature() == NNluFeatures::ENluFeature::IsPornQuery) {
            return nluFeature.GetValue() > 0;
        }
    }
    return false;
}

TResultValue ConstructMetaFromBaseRequest(const NAlice::NScenarios::TScenarioBaseRequest& request, NSc::TValue& meta) {
    meta["client_info"] = ParseClientInfo(request.GetClientInfo());
    meta["device_state"] = NSc::TValue::FromJson(NAlice::JsonFromProto(request.GetDeviceState()).GetStringRobust());;
    meta["experiments"] = ParseExperiments(request.GetExperiments());
    meta["location"] = ParseLocation(request.GetLocation());

    meta["lang"] = request.GetClientInfo().GetLang();
    meta["epoch"] = FromString<ui64>(request.GetClientInfo().GetEpoch());
    meta["dialog_id"] = request.GetDialogId();
    meta["client_id"] = GenClientId(request);
    meta["client_ip"] = request.GetOptions().GetClientIP();
    meta["user_agent"] = request.GetOptions().GetUserAgent();
    meta["tz"] = request.GetClientInfo().GetTimezone();
    meta["uuid"] = NAlice::SplitID(request.GetClientInfo().GetUuid());
    meta["device_id"] = request.GetClientInfo().GetDeviceId();
    meta["request_id"] = request.GetRequestId();
    meta["rng_seed"] = ToString(request.GetRandomSeed());
    meta["voice_session"] = request.GetInterfaces().GetVoiceSession();
    TString buffer;
    if (google::protobuf::util::MessageToJsonString(request.GetOptions().GetMegamindCookies(), &buffer).ok()) {
        meta["megamind_cookies"] = buffer;
    }

    // Protobuf uint64 fixes
    const auto& activeTimers = request.GetDeviceState().GetTimers().GetActiveTimers();
    auto& metaActiveTimers = meta["device_state"]["timers"]["active_timers"];
    for (int timerIndex = 0; timerIndex < activeTimers.size(); ++timerIndex) {
        metaActiveTimers[timerIndex]["start_timestamp"] = activeTimers[timerIndex].GetStartTimestamp();
    }

    auto& supportedFeatures = meta["client_features"]["supported"].GetArrayMutable();

    if (request.GetInterfaces().GetTtsPlayPlaceholder()) {
        supportedFeatures.push_back("tts_play_placeholder");
    }

    if (request.GetInterfaces().GetSupportsTvOpenCollectionScreenDirective()) {
        supportedFeatures.push_back("tv_open_collection_screen_directive");
    }

    if (request.GetInterfaces().GetSupportsTvOpenDetailsScreenDirective()) {
        supportedFeatures.push_back("tv_open_details_screen_directive");
    }

    if (request.GetInterfaces().GetSupportsTvOpenDetailsScreenDirective()) {
        supportedFeatures.push_back("tv_open_person_screen_directive");
    }

    if (request.GetInterfaces().GetSupportsTvOpenSearchScreenDirective()) {
        supportedFeatures.push_back("tv_open_search_screen_directive");
    }

    if (request.GetInterfaces().GetSupportsTvOpenSeriesScreenDirective()) {
        supportedFeatures.push_back("tv_open_series_screen_directive");
    }

    if (request.GetInterfaces().GetSupportsVerticalScreenNavigation()) {
        supportedFeatures.push_back("vertical_screen_navigation");
    }

    if (request.GetInterfaces().GetSupportsVideoPlayDirective()) {
        supportedFeatures.push_back("video_play_directive");
    }

    if (request.GetInterfaces().GetSupportsVideoProtocol()) {
        supportedFeatures.push_back("video_protocol");
    }

    if (request.GetInterfaces().GetVideoCodecAVC()) {
        supportedFeatures.push_back("video_codec_AVC");
    }

    if (request.GetInterfaces().GetVideoCodecHEVC()) {
        supportedFeatures.push_back("video_codec_HEVC");
    }

    if (request.GetInterfaces().GetVideoCodecVP9()) {
        supportedFeatures.push_back("video_codec_VP9");
    }

    if (request.GetInterfaces().GetAudioCodecAAC()) {
        supportedFeatures.push_back("audio_codec_AAC");
    }

    if (request.GetInterfaces().GetAudioCodecAC3()) {
        supportedFeatures.push_back("audio_codec_AC3");
    }

    if (request.GetInterfaces().GetAudioCodecEAC3()) {
        supportedFeatures.push_back("audio_codec_EAC3");
    }

    if (request.GetInterfaces().GetAudioCodecVORBIS()) {
        supportedFeatures.push_back("audio_codec_VORBIS");
    }

    if (request.GetInterfaces().GetAudioCodecOPUS()) {
        supportedFeatures.push_back("audio_codec_OPUS");
    }

    if (request.GetInterfaces().GetDynamicRangeSDR()) {
        supportedFeatures.push_back("dynamic_range_SDR");
    }

    if (request.GetInterfaces().GetDynamicRangeHDR10()) {
        supportedFeatures.push_back("dynamic_range_HDR10");
    }

    if (request.GetInterfaces().GetDynamicRangeHDR10Plus()) {
        supportedFeatures.push_back("dynamic_range_HDR10Plus");
    }

    if (request.GetInterfaces().GetDynamicRangeDV()) {
        supportedFeatures.push_back("dynamic_range_DV");
    }

    if (request.GetInterfaces().GetDynamicRangeHLG()) {
        supportedFeatures.push_back("dynamic_range_HLG");
    }

    meta["is_porn_query"] = IsPornQuery(request);

    // Not in protocol request:
    // meta["client_features"] = clientFeatures;
    // meta["personal_data"] = "";
    // meta["filtration_level"] = 0;
    // meta["is_banned"] = false;
    // meta["has_image_search_granet"] = false;
    // meta["megamind_cgi_string"] = "";
    // meta["process_id"] = "";
    // meta["region_id"] = 0;
    // meta["request_start_time"] = 0;
    // meta["screen_scale_factor"] = 0;
    // meta["tld"] = "";
    // meta["uid"] = 0;
    // meta["user_agent"] = "";
    // meta["yandex_uid"] = "";
    // meta["permissions"] = permissions;
    // meta["pure_gc"] = false;
    // meta["video_gallery_limit"] = 0;

    return ResultSuccess();
}

void CheckUtteranceForMeta(const NAlice::NScenarios::TScenarioRunRequest& request, NSc::TValue& meta) {
    if (request.GetInput().HasVoice()) {
        auto event = request.GetInput().GetVoice();
        NSc::TValue eventJson;
        eventJson["end_of_utterance"] = true;
        if (event.AsrDataSize() > 0) {
            const auto& bestResult = event.GetAsrData(0);
            if (bestResult.WordsSize() > 0) {
                const auto& bestWords = bestResult.GetWords();
                TStringBuilder utterance;
                for (const auto& elem : bestWords) {
                    if (!utterance.Empty()) {
                        utterance << ' ';
                    }
                    utterance << elem.GetValue();
                }
                eventJson["text"] = utterance;
            } else {
                eventJson["text"] = bestResult.GetUtterance();
            }
        }

        meta["utterance"] = eventJson["text"];
        meta["utterance_data"] = eventJson["payload"];
        //meta["asr_utterance"] = "";
        meta["end_of_utterance"] = eventJson["end_of_utterance"];
    } else if (request.GetInput().HasText()) {
        meta["end_of_utterance"] = true;
        meta["utterance"] = request.GetInput().GetText().GetUtterance();
    }
}

void FillBiometryForMeta(const NAlice::NScenarios::TScenarioRunRequest& request, NSc::TValue& meta) {
    if (request.GetInput().HasVoice()) {
        const auto event = request.GetInput().GetVoice();
        if (event.HasBiometryClassification()) {
            const auto& biometryClassification = event.GetBiometryClassification();
            meta["biometry_classification"]["status"] = biometryClassification.GetStatus();
            if (biometryClassification.SimpleSize() > 0) {
                const auto& simple = biometryClassification.GetSimple();
                NSc::TArray& newSimpleArray  = meta["biometry_classification"]["simple"].GetArrayMutable();
                for (const auto& elem : simple) {
                    NSc::TValue newSimpleElement;
                    newSimpleElement["classname"] = elem.GetClassName();
                    newSimpleElement["tag"] = elem.GetTag();
                    newSimpleArray.push_back(newSimpleElement);
                }
            }
        }
    }
}

void GetEnvironmentStateForMeta(const NAlice::NScenarios::TScenarioRunRequest& request, NSc::TValue& meta) {
    if (request.GetDataSources().count(NAlice::EDataSourceType::ENVIRONMENT_STATE) == 0) {
        LOG(INFO) << "No Environment state" << Endl;
        return;
    }
    const NAlice::TTandemEnvironmentState sourceEnvironmentState = request.GetDataSources()
        .at(NAlice::EDataSourceType::ENVIRONMENT_STATE).GetTandemEnvironmentState();
        meta["environment_state"] = NSc::TValue::FromJson(NAlice::JsonFromProto(sourceEnvironmentState).GetStringRobust());
}

void GetTandemEnvironmentStateForMeta(const NAlice::NScenarios::TScenarioRunRequest& request, NSc::TValue& meta) {
    if (request.GetDataSources().count(NAlice::EDataSourceType::TANDEM_ENVIRONMENT_STATE) == 0) {
        LOG(INFO) << "No Tandem Environment state" << Endl;
        return;
    }
    const NAlice::TTandemEnvironmentState sourceTandemEnvironmentState = request.GetDataSources()
        .at(NAlice::EDataSourceType::TANDEM_ENVIRONMENT_STATE).GetTandemEnvironmentState();
        meta["tandem_environment_state"] = NSc::TValue::FromJson(NAlice::JsonFromProto(sourceTandemEnvironmentState).GetStringRobust());
}

TResultValue GetMetaFromRunRequest(const NAlice::NScenarios::TScenarioRunRequest& request, NSc::TValue& meta) {
    if (const auto err = ConstructMetaFromBaseRequest(request.GetBaseRequest(), meta); err.Defined()) {
        return *err;
    }
    CheckUtteranceForMeta(request, meta);
    FillBiometryForMeta(request, meta);
    GetEnvironmentStateForMeta(request, meta);
    GetTandemEnvironmentStateForMeta(request, meta);
    return ResultSuccess();
}

TResultValue GetMetaFromApplyRequest(const NAlice::NScenarios::TScenarioApplyRequest& request, NSc::TValue& meta) {
    if (const auto err = ConstructMetaFromBaseRequest(request.GetBaseRequest(), meta); err.Defined()) {
        return *err;
    }
    return ResultSuccess();
}

// TVideoScenario
NAlice::TClientFeatures CreateClientFeatures(const TScenarioRunRequest& request) {
    THashMap<TString, TMaybe<TString>> experiments;
    for (const auto& [key, value] : request.GetBaseRequest().GetExperiments().fields()) {
        experiments[key] = value.string_value();
    }

    return NAlice::TClientFeatures(request.GetBaseRequest().GetClientInfo(), experiments);
}

TMaybe<TString> GetSearchText(const TScenarioRunRequest& request, const TStringBuf intentType) {
    if (intentType == SEARCH_VIDEO) {
        const auto* slots = TryGetIntentFrame(request, SEARCH_VIDEO);
        if (!slots) {
            return Nothing();
        }
        const auto* textSlot = TryGetSlotFromFrame(*slots, SLOT_SEARCH_TEXT);
        if (!textSlot) {
            return Nothing();
        }
        return textSlot->GetValue();
    }
    if (intentType == QUASAR_SELECT_VIDEO_FROM_GALLERY) {
        auto selectFormName = TQuasarSelectVideoFromGalleryIntent::TryGetFormName(request);
        if (!selectFormName) {
            return Nothing();
        }
        const auto* slots = TryGetIntentFrame(request, *selectFormName);
        if (!slots) {
            return Nothing();
        }
        const auto* textSlot = TryGetSlotFromFrame(*slots, SLOT_VIDEO_TEXT);
        if (!textSlot) {
            return Nothing();
        }
        return textSlot->GetValue();
    }
    return Nothing();
}

class TVideoScenario {
public:
    explicit TVideoScenario() = default;

    bool IsEnabled(const NAlice::TClientInfo& clientInfo) const {
        if (clientInfo.IsSmartSpeaker()) {
            return true;
        }
        if (clientInfo.IsTvDevice()) {
            return true;
        }
        if (clientInfo.IsLegatus()) {
            return true;
        }
        return false;
    }

    virtual TStringBuf ChooseVideoIntent(const TScenarioRunRequest& request,
                                         const NAlice::TClientFeatures& clientFeatures) const {
        TStringBuf chosenIntent = NVideoProtocol::ChooseIntent(request, clientFeatures);
        LOG(DEBUG) << "Intent " << chosenIntent << " was chosen." << Endl;
        return chosenIntent;
    }

    TResultValue CreateRunRequest(const TScenarioRunRequest& request, NSc::TValue& resultRequest,
                                  NAlice::NVideoCommon::TVideoFeatures& features,
                                  TStringBuf& intentType, TMaybe<TString>& searchText) const {
        NAlice::TClientFeatures clientFeatures = CreateClientFeatures(request);

        if (!IsEnabled(clientFeatures) && !HasIntentFrame(request, VIDEO_COMMAND_CHANGE_TRACK_HARDCODED)) {
            TStringBuilder errMsg;
            errMsg << "Bass video protocol scenario is disabled";
            return TError{TError::EType::PROTOCOL_IRRELEVANT, errMsg};
        }

        if (request.GetInput().GetEventCase() == NScenarios::TInput::kCallback) {
            const auto& payload = request.GetInput().GetCallback().GetPayload();
            TMordoviaJsCallbackPayload jsCallbackPayload(payload);
            intentType = jsCallbackPayload.Command();
        } else {
            intentType = ChooseVideoIntent(request, clientFeatures);
        }

        if (const auto chosenIntent = NVideoProtocol::CreateIntent(intentType)) {
            if (const auto err = GetMetaFromRunRequest(request, resultRequest["meta"]); err.Defined()) {
                return *err;
            }
            if (const auto err = chosenIntent->MakeRunRequest(request, resultRequest); err.Defined()) {
                return *err;
            }
            searchText = GetSearchText(request, intentType);
            CalculateFeaturesAtStart(
                intentType,
                searchText,
                features,
                request.GetBaseRequest().GetDeviceState(),
                SelectByName(request).Confidence,
                SelectByNumber(request).Confidence,
                TBassLogAdapter{}
            );
            return ResultSuccess();
        }
        return TError{TError::EType::PROTOCOL_IRRELEVANT, TStringBuf("Unable to choose video intent")};
    }

    TResultValue CreateApplyRequest(const TScenarioApplyRequest& request,
                                    NSc::TValue& resultRequest) const {
        resultRequest = NSc::TValue::FromJson(
            NAlice::JsonFromProto(request.GetArguments())["value"].GetStringRobust());
        if (const auto err = GetMetaFromApplyRequest(request, resultRequest["Meta"]); err.Defined()) {
            return *err;
        }
        return ResultSuccess();
    }
};

} // namespace

TResultValue NVideoProtocol::CreateBassRunVideoRequest(const TScenarioRunRequest& request,
                                                       NSc::TValue& bassRunRequest,
                                                       NAlice::NVideoCommon::TVideoFeatures& features,
                                                       TStringBuf& intentType, TMaybe<TString>& searchText) {
    const TVideoScenario scenario;
    if (const auto err = scenario.CreateRunRequest(request, bassRunRequest, features,
                                                   intentType, searchText); err.Defined()) {
        return *err;
    }
    return ResultSuccess();
}

TResultValue NVideoProtocol::CreateBassApplyVideoRequest(const TScenarioApplyRequest& request,
                                                         NSc::TValue& bassApplyRequest) {
    const TVideoScenario scenario;
    if (const auto err = scenario.CreateApplyRequest(request, bassApplyRequest); err.Defined()) {
        return *err;
    }
    return ResultSuccess();
}
