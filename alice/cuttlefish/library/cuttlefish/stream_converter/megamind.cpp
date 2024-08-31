#include "megamind.h"

#include "support_functions.h"

#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/utils.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/library/client/protos/client_info.pb.h>
#include <alice/library/json/json.h>
#include <alice/megamind/api/request/constructor.h>
#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <google/protobuf/struct.pb.h>
#include <library/cpp/protobuf/json/json2proto.h>

#undef DLOG
#define DLOG(args)

using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NJson;

void NAlice::NCuttlefish::NAppHostServices::MessageToMegamindRequest(
    NAliceProtocol::TMegamindRequest& mmRequest,
    NAliceProtocol::TRequestContext& requestContext,
    const NVoicetech::NUniproxy2::TMessage& message,
    const NAliceProtocol::TSessionContext& sessionContext,
    TLogContext* logContext
) {
    (void)logContext;
    auto& mmRequestBase = *mmRequest.MutableRequestBase();
    mmRequestBase = sessionContext.GetRequestBase();  // fill mmRequest using 'session_data' (protobuf surrogate)
    /* ^^^ TODO: remove in future RequestBase from SessionContex - use instead native SessionContext fields - see sample code below:
    // fill RequestBase.Application (json "payload/application" branch)
    if (sessionContext.HasAppId()) {
        mmRequestBase.MutableApplication()->SetAppId(sessionContext.GetAppId());
    }
    if (sessionContext.HasAppVersion()) {
        mmRequestBase.MutableApplication()->SetAppVersion(sessionContext.GetAppVersion());
    }
    if (sessionContext.HasAppLang()) {
        mmRequestBase.MutableApplication()->SetLang(sessionContext.GetAppLang());
    }
    if (sessionContext.HasDeviceInfo()) {
        auto& deviceInfo = sessionContext.GetDeviceInfo();
        if (deviceInfo.HasDeviceManufacturer()) {
            mmRequestBase.MutableApplication()->SetDeviceManufacturer(deviceInfo.GetDeviceManufacturer());
        }
        if (deviceInfo.HasDeviceModel()) {
            mmRequestBase.MutableApplication()->SetDeviceModel(deviceInfo.GetDeviceModel());
        }
        if (deviceInfo.HasDeviceId()) {
            mmRequestBase.MutableApplication()->SetDeviceId(deviceInfo.GetDeviceId());
        }
        if (deviceInfo.HasPlatform()) {
            mmRequestBase.MutableApplication()->SetPlatform(deviceInfo.GetPlatform());
        }
        if (deviceInfo.HasOsVersion()) {
            mmRequestBase.MutableApplication()->SetOsVersion(deviceInfo.GetOsVersion());
        }
        if (deviceInfo.HasDeviceColor()) {
            mmRequestBase.MutableApplication()->SetDeviceColor(deviceInfo.GetDeviceColor());
        }
        //deviceInfo.NetworkType?
        //deviceInfo.SupportedFeatures?
        //deviceInfo.WifiNetworks?
        //deviceInfo.Device?
        //deviceInfo.DeviceModication?
    }
    if (sessionContext.HasUserInfo()) {
        auto& userInfo = sessionContext.GetUserInfo();
        if (userInfo.HasUuid()) {
            mmRequestBase.MutableApplication()->SetUuid(userInfo.GetUuid());
        }
    }
    //requestBase.MutableApplication().ClientTime - get from request
    //requestBase.MutableApplication().Timezone - get from request

    //requestBase.MutableHeader()->RequestId - get from request
    //requestBase.MutableHeader()->PrevReqId - get from request
    //requestBase.MutableHeader()->SequenceNumber - get from request
    //requestBase.MutableHeader()->DialogId - get from request
    //requestBase.MutableHeader()->RefMessageId - fill in MM_ADAPTER_*
    //requestBase.MutableHeader()->RandomSeed?
    */

    // update mmRequest using current 'vins' request
    UpdateSpeechKitRequest(mmRequestBase, sessionContext, message.Json);
    // set fields not received from user (or placed in unexpected fields)
    if (sessionContext.HasSessionId()) {
        mmRequestBase.MutableHeader()->SetSessionId(sessionContext.GetSessionId());
    }

    if (const auto* request = message.Json.GetValueByPath("event.payload.request")) {
        static const NProtobufJson::TJson2ProtoConfig useJsonNames{NProtobufJson::TJson2ProtoConfig().SetUseJsonName(true)};

        if (const auto* predefinedBioScoringResult = request->GetValueByPath("predefined_bio_scoring_result")) {
            NProtobufJson::Json2Proto(
                *predefinedBioScoringResult,
                *mmRequestBase.MutableRequest()->MutableEvent()->MutableBiometryScoring(),
                useJsonNames
            );
            requestContext.MutablePredefinedResults()->SetBioScoring(true);
        }
        if (const auto* predefinedBioClassifyResult = request->GetValueByPath("predefined_bio_classify_result")) {
            NProtobufJson::Json2Proto(
                *predefinedBioClassifyResult,
                *mmRequestBase.MutableRequest()->MutableEvent()->MutableBiometryClassification(),
                useJsonNames
            );
            requestContext.MutablePredefinedResults()->SetBioClassify(true);
        }
        if (const auto* predefinedAsrResult = request->GetValueByPath("predefined_asr_result")) {
            if (const auto* recognition = predefinedAsrResult->GetValueByPath("recognition")) {
                if (recognition->IsArray()) {
                    for (auto& it : recognition->GetArray()) {
                        auto& asrResult = *mmRequestBase.MutableRequest()->MutableEvent()->AddAsrResult();
                        NProtobufJson::Json2Proto(it, asrResult, useJsonNames);
                    }
                }
            }
            if (const auto* eou = predefinedAsrResult->GetValueByPath("endOfUtt")) {
                mmRequestBase.MutableRequest()->MutableEvent()->SetEndOfUtterance(eou->GetBooleanSafe());
            }
            if (const auto* partNum = predefinedAsrResult->GetValueByPath("asr_partial_number")) {
                mmRequestBase.MutableRequest()->MutableEvent()->SetHypothesisNumber(partNum->GetIntegerSafe());
            }
            requestContext.MutablePredefinedResults()->SetAsr(true);
        }
        if (const auto* enrollmentHeadersJson = request->GetValueByPath("enrollment_headers"); enrollmentHeadersJson != nullptr) {
            NAlice::TEnrollmentHeaders& enrollmentHeadersContainer = *mmRequestBase.MutableEnrollmentHeaders();
            enrollmentHeadersContainer = JsonToProto<NAlice::TEnrollmentHeaders>(*enrollmentHeadersJson);

            // TODO (@aradzevich): REMOVE AFTER 2022	
            // Empty personId means, that user hasn't got acquainted with smart column using the latest acquaintance scenario.	
            for (NAlice::TEnrollmentHeader& enrollmentHeader : *enrollmentHeadersContainer.MutableHeaders()) {	
                if (enrollmentHeader.GetUserType() == NAlice::OWNER && enrollmentHeader.GetPersonId().empty()) {	
                    // PersonId is usually generated by the scenario itself, but if user hasn't passed it, he doesn't have this id	
                    enrollmentHeader.SetPersonId(TStringBuilder() << "PersId-" << CreateGuidAsString());	

                    // Special user type to let client now, that it's an initial pushing of the voiceprint to the client.	
                    enrollmentHeader.SetUserType(NAlice::__SYSTEM_OWNER_DO_NOT_USE_AFTER_2021);	
                }
            }
        }
        // TODO @aradzevich (VOICESERV-4454): Get GuestUserOptions only from additional_options.guest_user_options when moved into additional_options
        if (const auto* guestUserOptionsJson = request->GetValueByPath("guest_user_options");
            guestUserOptionsJson != nullptr && !guestUserOptionsJson->GetMap().empty()
        ) {
            NAlice::TGuestOptions& guestUserOptions = *mmRequestBase.MutableRequest()->MutableAdditionalOptions()->MutableGuestUserOptions();
            guestUserOptions = JsonToProto<NAlice::TGuestOptions>(*guestUserOptionsJson);
        }
        if (const auto* guestUserOptionsJson = request->GetValueByPath("additional_options.guest_user_options");
            guestUserOptionsJson != nullptr && !guestUserOptionsJson->GetMap().empty()
        ) {
            NAlice::TGuestOptions& guestUserOptions = *mmRequestBase.MutableRequest()->MutableAdditionalOptions()->MutableGuestUserOptions();
            guestUserOptions = JsonToProto<NAlice::TGuestOptions>(*guestUserOptionsJson);
        }
        // for testing purpose can receive iot config from user (in request)
        if (sessionContext.GetUserInfo().GetUuidKind() != NAliceProtocol::TUserInfo::USER) {
            if (const auto* iotConfig = request->GetValueByPath("iot_config")) {
                const TString s = iotConfig->GetString();
                if (s) {
                    mmRequestBase.SetIoTUserInfoData(s);
                    mmRequestBase.MutableRequest()->ClearSmartHomeInfo();
                    requestContext.MutablePredefinedResults()->SetIotConfig(true);
                }
            }
        }

        // for testing purpose we can receive mm-session from user (in request)
        // VOICESERV-1937 DIALOG-5443 VOICESERV-4189
        if (NAlice::NCuttlefish::NExpFlags::ConductingExperiment(requestContext, "stateless_uniproxy_session")) {
            if (const NJson::TJsonValue* session = request->GetValueByPath("session")) {
                requestContext.MutablePredefinedResults()->SetMegamindSession(true);
                // fail test if json-field session has invalid type.
                mmRequestBase.SetSession(session->GetStringSafe());
            }
        }
    }

    if (!mmRequestBase.GetRequest().GetAdditionalOptions().HasServerTimeMs()
        || !NAlice::NCuttlefish::NExpFlags::ConductingExperiment(requestContext, "uniproxy_use_server_time_from_client"))
    {
        // MEGAMIND-3031 VOICESERV-4132
        mmRequestBase.MutableRequest()->MutableAdditionalOptions()->SetServerTimeMs(TInstant::Now().MilliSeconds());
    }
    {
        // MEGAMIND-3033
        auto& connInfo = sessionContext.GetConnectionInfo();
        if (connInfo.HasPredefinedIpAddress()) {
            mmRequestBase.MutableRequest()->MutableAdditionalOptions()->MutableBassOptions()->SetClientIP(connInfo.GetPredefinedIpAddress());
        } else if (connInfo.HasIpAddress()) {
            mmRequestBase.MutableRequest()->MutableAdditionalOptions()->MutableBassOptions()->SetClientIP(connInfo.GetIpAddress());
        }
    }
    if (sessionContext.HasUserInfo()) {
        auto& userInfo = sessionContext.GetUserInfo();
        if (userInfo.HasAuthToken()) {
            mmRequestBase.MutableRequest()->MutableAdditionalOptions()->SetOAuthToken(userInfo.GetAuthToken());
        }

        // TODO (paxakor): remove
        if (userInfo.HasLaasRegion()) {
            google::protobuf::util::JsonStringToMessage(userInfo.GetLaasRegion(), mmRequestBase.MutableRequest()->MutableLaasRegion());
        }
    }
}

void NAlice::NCuttlefish::NAppHostServices::UpdateSpeechKitRequest(
    NAlice::TSpeechKitRequestProto& skRequest,
    const NAliceProtocol::TSessionContext& sessionContext,
    const NAliceProtocol::TWsEvent& wsEvent
) {
    if (!wsEvent.HasText()) {
        return;
    }

    NJson::TJsonValue messageJson;
    NJson::ReadJsonTree(wsEvent.GetText(), &messageJson, /* trowOnError */ true);
    UpdateSpeechKitRequest(skRequest, sessionContext, messageJson);
}

void NAlice::NCuttlefish::NAppHostServices::UpdateSpeechKitRequest(
    NAlice::TSpeechKitRequestProto& skRequest,
    const NAliceProtocol::TSessionContext& sessionContext,
    const NJson::TJsonValue& messageJson
) {
    (void)sessionContext;
    if (const auto* payload = messageJson.GetValueByPath("event.payload")) {
        DLOG("Original JSON: " << TJsonAsDense{*payload});

        NAlice::NMegamindApi::TRequestConstructor constructor{};
        const auto status = constructor.PushSpeechKitJson(*payload);
        if (!status.Ok()) {
            ythrow TMegamindRequestConstructorError() << status.GetMessage();
        }

        NAlice::TSpeechKitRequestProto& requestBase = skRequest;
        requestBase.MergeFrom(std::move(constructor).MakeRequest());
        if (const auto* applicationNode = payload->GetValueByPath("vins.application")) {
            NAlice::TClientInfoProto tmp;
            NAlice::JsonToProto(*applicationNode, tmp, /* validateUtf8 = */ false, /* ignoreUnknownFields = */ true);
            requestBase.MutableApplication()->MergeFrom(tmp);
            DLOG("Result application info: " << *requestBase.MutableApplication());
        }
    }
}
