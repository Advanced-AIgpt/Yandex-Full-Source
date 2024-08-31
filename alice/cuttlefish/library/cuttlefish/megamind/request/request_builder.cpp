#include "request_builder.h"

#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/context.h>

#include <alice/cuttlefish/library/cuttlefish/common/datasync_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/experiments/flags_json.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/common/smart_home.pb.h>
#include <alice/megamind/protos/guest/guest_data.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/megamind/protos/scenarios/notification_state.pb.h>

#include <alice/protos/data/contacts.pb.h>

#include <alice/library/blackbox/blackbox.h>
#include <alice/library/blackbox/proto/blackbox.pb.h>
#include <alice/library/experiments/utils.h>
#include <alice/library/json/json.h>

#include <voicetech/library/settings_manager/proto/settings.pb.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/hash_set.h>
#include <util/string/builder.h>
#include <util/string/type.h>
#include <util/system/env.h>
#include <util/system/hostname.h>

#include "hinter.h"

using namespace NAlice::NCuttlefish;

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

const TString CHILDREN_BIOMETRY_HANDLE = "/v1/personality/profile/alisa/kv/alice_children_biometry";

void ParseContact(const NJson::TJsonValue &jContact, NAlice::NData::TContactsList::TContact &pContact) {
    if (jContact.Has("account_name")) {
        pContact.SetAccountName(jContact["account_name"].GetString());
    }
    if (jContact.Has("account_type")) {
        pContact.SetAccountType(jContact["account_type"].GetString());
    }
    if (jContact.Has("display_name")) {
        pContact.SetDisplayName(jContact["display_name"].GetString());
    }
    if (jContact.Has("first_name")) {
        pContact.SetFirstName(jContact["first_name"].GetString());
    }
    if (jContact.Has("middle_name")) {
        pContact.SetMiddleName(jContact["middle_name"].GetString());
    }
    if (jContact.Has("second_name")) {
        pContact.SetSecondName(jContact["second_name"].GetString());
    }
    if (jContact.Has("contact_id")) {
        pContact.SetContactId(jContact["contact_id"].GetInteger());
    }
    if (jContact.Has("_id")) {
        pContact.SetId(jContact["_id"].GetInteger());
    }
    if (jContact.Has("lookup_key")) {
        pContact.SetLookupKey(jContact["lookup_key"].GetString());
    }
    if (jContact.Has("last_time_contacted")) {
        pContact.SetLastTimeContacted(jContact["last_time_contacted"].GetInteger());
    }
    if (jContact.Has("times_contacted")) {
        pContact.SetTimesContacted(jContact["times_contacted"].GetInteger());
    }
    if (jContact.Has("lookup_key_index")) {
        pContact.SetLookupKeyIndex(jContact["lookup_key_index"].GetInteger());
    }
}

void ParsePhone(const NJson::TJsonValue &jPhone, NAlice::NData::TContactsList::TPhone &pPhone) {
    if (jPhone.Has("_id")) {
        pPhone.SetId(jPhone["_id"].GetInteger());
    }
    if (jPhone.Has("account_type")) {
        pPhone.SetAccountType(jPhone["account_type"].GetString());
    }
    if (jPhone.Has("lookup_key")) {
        pPhone.SetLookupKey(jPhone["lookup_key"].GetString());
    }
    if (jPhone.Has("phone")) {
        pPhone.SetPhone(jPhone["phone"].GetString());
    }
    if (jPhone.Has("type")) {
        pPhone.SetType(jPhone["type"].GetString());
    }
    if (jPhone.Has("_id_string")) {
        pPhone.SetIdString(jPhone["_id_string"].GetString());
    }
    if (jPhone.Has("lookup_key_index")) {
        pPhone.SetLookupKeyIndex(jPhone["lookup_key_index"].GetInteger());
    }
}

}

void ParseContactsResponseJson(const NAppHostHttp::THttpResponse &httpResp, NAlice::TSpeechKitRequestProto::TContacts &proto, const TLogContext& logContext) {
    if (httpResp.GetStatusCode() / 100 != 2) {
        logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Contacts responded with a non-ok status code: " << httpResp.GetStatusCode());
        return;
    }
    const NJson::TJsonValue json = NAlice::JsonFromString(httpResp.GetContent());
    if (json.Has("status")) {
        proto.SetStatus(json["status"].GetString());
    }
    if (json.Has("data")) {
        const NJson::TJsonValue& jData = json["data"];
        NAlice::NData::TContactsList& pData = *proto.MutableData();
        if (jData.Has("is_known_uuid")) {
            pData.SetIsKnownUuid(jData["is_known_uuid"].GetBoolean());
        }
        if (jData.Has("contacts")) {
            const auto& jContacts = jData["contacts"].GetArraySafe();
            for (auto it = jContacts.begin(); it != jContacts.end(); ++it) {
                ParseContact(*it, *pData.AddContacts());
            }
        }
        if (jData.Has("phones")) {
            const auto& jPhones = jData["phones"].GetArraySafe();
            for (auto it = jPhones.begin(); it != jPhones.end(); ++it) {
                ParsePhone(*it, *pData.AddPhones());
            }
        }
        if (jData.Has("lookup_key_map_serialized")) {
            pData.SetLookupKeyMapSerialized(Base64Decode(jData["lookup_key_map_serialized"].GetString()));
        }
    }
}


namespace {

const THashSet<TStringBuf> VINS_PRODUCTION_HOSTS = {
    "vins-int.voicetech.yandex.net",
    "vins.alice.yandex.net",
};


void EnrichRequestWithContextLoadResponse(
    const NAliceProtocol::TRequestContext& requestCtx,
    const bool shouldSendProtobufContent,
    const ERequestPhase phase,
    NAlice::TSpeechKitRequestProto& proto,
    const NAliceProtocol::TContextLoadResponse& contextLoadResponse,
    const TLogContext& logContext
) {
    // Memento response
    if (contextLoadResponse.HasMementoResponse()) {
        try {
            logContext.LogEvent(NEvClass::InfoMessage("Merging Memento to request base"));
            const auto& content = contextLoadResponse.GetMementoResponse().GetContent();
            proto.SetMementoData(Base64Encode(content));
        } catch (...) {
            logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Merging Memento failed: " << CurrentExceptionMessage());
        }
    }

    // Datasync response
    try {
        TDatasyncResponseParser parser;
        if (contextLoadResponse.HasDatasyncResponse()) {
            parser.ParseDatasyncResponse(contextLoadResponse.GetDatasyncResponse());
        }
        if (contextLoadResponse.HasDatasyncDeviceIdResponse()) {
            parser.ParseDatasyncResponse(contextLoadResponse.GetDatasyncDeviceIdResponse());
        }
        if (contextLoadResponse.HasDatasyncUuidResponse()) {
            parser.ParseDatasyncResponse(contextLoadResponse.GetDatasyncUuidResponse());
        }

        if (!parser.PersonalData.GetMap().empty()) {
            proto.MutableRequest()->SetRawPersonalData(NAlice::JsonToString(parser.PersonalData));
        }
        if (parser.DoNotUseUserLogs.Defined()) {
            proto.MutableRequest()->MutableAdditionalOptions()->SetDoNotUseUserLogs(parser.DoNotUseUserLogs.GetRef());
        }

        if (parser.PersonalData.Has(CHILDREN_BIOMETRY_HANDLE) &&
            proto.HasRequest() &&
            proto.GetRequest().HasEvent() &&
            proto.GetRequest().GetEvent().HasBiometryClassification()
        ) {
            const TString& childrenBiometryHandleValue = parser.PersonalData[CHILDREN_BIOMETRY_HANDLE].GetString();

            if (childrenBiometryHandleValue == "disabled") {
                auto bioClassesSimple = proto.MutableRequest()->MutableEvent()->MutableBiometryClassification()->MutableSimple();

                ::google::protobuf::RepeatedPtrField<NAlice::TBiometryClassification_TClassificationSimple> filteredBio;
                for (auto& bioClassSimple : *bioClassesSimple) {
                   if (!bioClassSimple.HasTag() || bioClassSimple.GetTag() != "children") {
                        filteredBio.Add(std::move(bioClassSimple));
                   }
                }

                *bioClassesSimple = std::move(filteredBio);
            } else if (childrenBiometryHandleValue != "enabled") {
                logContext.LogEventErrorCombo<NEvClass::IncorrectValueOfChildrenBiometryError>(childrenBiometryHandleValue);
            }
        }
    } catch (...) {
        logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Merging Datasync failed: " << CurrentExceptionMessage());
    }

    // Contacts response
    try {
        if (contextLoadResponse.HasContactsProto()) {
            // Fill TContacts field only in protobuf mode
            // https://a.yandex-team.ru/review/2073518/details#comment-2783598

            if (shouldSendProtobufContent) {
                proto.MutableContacts()->MutableData()->CopyFrom(contextLoadResponse.GetContactsProto());
                proto.MutableContacts()->SetStatus("ok");
            } else {
                NAlice::TSpeechKitRequestProto::TContacts contacts;
                contacts.SetStatus("ok");
                *contacts.MutableData() = contextLoadResponse.GetContactsProto();
                proto.SetContactsProto(Base64Encode(contacts.SerializeAsString()));
            }
        } else if (contextLoadResponse.HasContactsResponse()) {
            // TODO: Kill this branch once everything uses proto.
            ParseContactsResponseJson(contextLoadResponse.GetContactsResponse(), *proto.MutableContacts(), logContext);
        }
    } catch (...) {
        logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Merging Contacts failed: " << CurrentExceptionMessage());
    }

    // Quasar IoT response
    if (proto.HasIoTUserInfoData()) {
        logContext.LogEvent(NEvClass::InfoMessage("Already has IoTUserInfoData in request (ignore QuasarIotResponse even if has)"));
    } else if (contextLoadResponse.HasIoTUserInfo()) {
        try {
            logContext.LogEvent(NEvClass::InfoMessage("Merging IoTUserInfo to request base"));

            // TODO (@paxakor) do we need copy here?
            NAlice::TIoTUserInfo info = contextLoadResponse.GetIoTUserInfo();

            const TString& rawData = info.GetRawUserInfo();
            NAlice::JsonToProto(NAlice::JsonFromString(rawData), *proto.MutableRequest()->MutableSmartHomeInfo(), /* validateUtf8 = */ false, /* ignoreUnknownFields = */ true);

            info.ClearRawUserInfo(); // delete RawUserInfo
            proto.SetIoTUserInfoData(Base64Encode(info.SerializeAsString()));
        } catch (...) {
            logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Merging IoTUserInfo failed: " << CurrentExceptionMessage());
        }
    } else if (contextLoadResponse.HasQuasarIotResponse()) {
        try {
            logContext.LogEvent(NEvClass::InfoMessage("Merging QuasarIoT to request base"));

            const auto& content = contextLoadResponse.GetQuasarIotResponse().GetContent();
            NAlice::TIoTUserInfo info;
            Y_PROTOBUF_SUPPRESS_NODISCARD info.ParseFromString(content);

            const TString& rawData = info.GetRawUserInfo();
            NAlice::JsonToProto(NAlice::JsonFromString(rawData), *proto.MutableRequest()->MutableSmartHomeInfo(), /* validateUtf8 = */ false, /* ignoreUnknownFields = */ true);

            info.SetRawUserInfo(""); // delete RawUserInfo
            proto.SetIoTUserInfoData(Base64Encode(info.SerializeAsString()));
        } catch (...) {
            logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Merging QuasarIoT failed: " << CurrentExceptionMessage());
        }
    }

    // Notificator response
    if (contextLoadResponse.HasNotificatorResponse()) {
        try {
            logContext.LogEvent(NEvClass::InfoMessage("Merging Notificator to request base"));
            const auto& content = contextLoadResponse.GetNotificatorResponse().GetContent();
            Y_PROTOBUF_SUPPRESS_NODISCARD proto.MutableRequest()->MutableNotificationState()->ParseFromString(content);
        } catch (...) {
            logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Merging Notificator failed: " << CurrentExceptionMessage());
        }
    }

    // Megamind session (if not apply)
    // Megamind session can be already set in WS_ADAPTER_IN in case of ue2e tests.
    if (
        !requestCtx.GetPredefinedResults().GetMegamindSession() &&
        contextLoadResponse.HasMegamindSessionResponse() &&
        phase != ERequestPhase::APPLY
    ) {
        try {
            const auto& sessionResp = contextLoadResponse.GetMegamindSessionResponse();
            if (sessionResp.HasMegamindSessionLoadResp()) {
                logContext.LogEvent(NEvClass::InfoMessage("Merging MM Session to request base"));
                const auto& loadResp = sessionResp.GetMegamindSessionLoadResp();
                proto.SetSession(Base64Encode(loadResp.GetData()));
            }
        } catch (...) {
            logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Merging MM Session failed: " << CurrentExceptionMessage());
        }
    }

    // AB flags
    if (contextLoadResponse.HasFlagsInfo()) {
        const NAliceProtocol::TFlagsInfo& flagsInfoProto = contextLoadResponse.GetFlagsInfo();
        proto.MutableRequest()->MutableExperiments()->MergeFrom(flagsInfoProto.GetVoiceFlags());

        for (const TString& testIdFromFlagsJson : flagsInfoProto.GetAllTestIds()) {
            // Write -1 if string is not integer.
            int64_t intTestId = -1;
            if (!TryFromString(testIdFromFlagsJson, intTestId)) {
                logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(
                    TStringBuilder() << "Invalid test-id from flags.json: " << testIdFromFlagsJson
                );
            }
            proto.MutableRequest()->AddTestIDs(intTestId);
        }

        NVoice::NExperiments::TFlagsJsonFlagsConstRef flagsJsonFlags(&flagsInfoProto);

        // VOICESERV-4259
        if (const TMaybe<TString> langExpValue = flagsJsonFlags.GetValueFromName("force_language_mm=")) {
            proto.MutableApplication()->SetLang(langExpValue.GetRef());
        }
    }

    // laas
    if (contextLoadResponse.HasLaasResponse() && contextLoadResponse.GetLaasResponse().GetStatusCode() == 200) {
        try {
            google::protobuf::util::JsonStringToMessage(
                contextLoadResponse.GetLaasResponse().GetContent(),
                proto.MutableRequest()->MutableLaasRegion()
            );
        } catch (...) {
            logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Merging Laas failed: " << CurrentExceptionMessage());
        }
    }
}

void EnrichRequestWithGuestData(
    NAlice::TSpeechKitRequestProto& proto,
    const TAtomicSharedPtr<TSpeakerContext> speakerContextPtr,
    const NAliceProtocol::TContextLoadResponse& contextLoadResponse
) {
    // VoiceInput
    if (speakerContextPtr) {
        auto& guestDataProto = *proto.MutableGuestUserData();
        guestDataProto.MergeFrom(speakerContextPtr->GuestUserData);

        auto& guestOptionsProto = *proto.MutableRequest()->MutableAdditionalOptions()->MutableGuestUserOptions();
        guestOptionsProto.MergeFrom(speakerContextPtr->GuestUserOptions);

    // TextInput
    } else if (proto.GetRequest().GetAdditionalOptions().HasGuestUserOptions()) {
        TRefSpeakerContext speakerContext(
            *proto.MutableRequest()->MutableAdditionalOptions()->MutableGuestUserOptions(),
            *proto.MutableGuestUserData());

        if (contextLoadResponse.HasGuest()) {
            const NAliceProtocol::TGuestContextLoadResponse& guestContext = contextLoadResponse.GetGuest();

            if (guestContext.HasBlackboxResponse()) {
                EnrichFromBlackboxResponse(speakerContext, guestContext.GetBlackboxResponse());
            }
            if (guestContext.HasDatasyncResponse()) {
                EnrichFromDatasyncResponse(speakerContext, guestContext.GetDatasyncResponse());
            }
        }
    }
}

void EnrichRequestAdditionalOptions(
    NAlice::TSpeechKitRequestProto& proto,
    const NAliceProtocol::TSessionContext& sessionCtx,
    const NAliceProtocol::TContextLoadResponse& contextLoadResponse
) {
    auto& additionalOptions = *proto.MutableRequest()->MutableAdditionalOptions();

    // python source: https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/vins/vinsrequest.py?rev=r8039889#L552
    if (sessionCtx.GetConnectionInfo().HasIpAddress()) {
        auto& bassOptions = *additionalOptions.MutableBassOptions();
        bassOptions.SetClientIP(sessionCtx.GetConnectionInfo().GetIpAddress());
    }

    if (contextLoadResponse.HasFlagsInfo()) {
        using namespace NVoice::NExperiments;
        TFlagsJsonFlagsConstRef flagsInfoRef(&contextLoadResponse.GetFlagsInfo());
        TMaybe<TString> boxes = flagsInfoRef.GetExpBoxes();
        if (boxes.Defined() && boxes.GetRef().size() > 0) {
            additionalOptions.SetExpboxes(std::move(boxes.GetRef()));
        }
    }

    // python source: https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/vins/vinsrequest.py?rev=r8039889#L712-720
    if (sessionCtx.GetUserInfo().HasYuid()) {
        additionalOptions.SetYandexUID(sessionCtx.GetUserInfo().GetYuid());
    }
    if (sessionCtx.GetUserInfo().HasPuid()) {
        additionalOptions.SetPuid(sessionCtx.GetUserInfo().GetPuid());
    }
    if (sessionCtx.GetUserOptions().HasDoNotUseLogs()) {
        // TODO (paxakor): remove it. DoNotUseUserLogs is loaded from context_load.
        if (!additionalOptions.HasDoNotUseUserLogs()) {
            additionalOptions.SetDoNotUseUserLogs(sessionCtx.GetUserOptions().GetDoNotUseLogs());
        }
    }
    if (sessionCtx.GetUserInfo().HasICookie()) {
        additionalOptions.SetICookie(sessionCtx.GetUserInfo().GetICookie());
    }
}

}

TMegamindRequestBuilder::TMegamindRequestBuilder(
    const ERequestPhase requestPhase,
    const NAliceCuttlefishConfig::TConfig& config,
    TMaybe<NJson::TJsonValue> appHostParams,
    const NAliceProtocol::TSessionContext& sessionCtx,
    const NAliceProtocol::TRequestContext& requestCtx,
    const NAliceProtocol::TContextLoadResponse& contextLoadResponse,
    const IActiveSpeakerService& speakerService,
    const NAlice::TLoggerOptions& aliceLoggerOptions,
    TLogContext logContext
)
    : RequestPhase(requestPhase)
    , Config(config)
    , AppHostParams(std::move(appHostParams))
    , SessionCtx(sessionCtx)
    , RequestCtx(requestCtx)
    , ContextLoadResponse(contextLoadResponse)
    , SpeakerService(speakerService)
    , AliceLoggerOptions(aliceLoggerOptions)
    , LogContext(logContext)
{
}

NNeh::TMessage TMegamindRequestBuilder::Build(const NAlice::TSpeechKitRequestProto& base, TRTLogActivation& rtLogChild, NJson::TJsonValue& sessionLog, bool isFinal) const {
    auto skrProto = base;
    EnrichRequestWithContextLoadResponse(RequestCtx, ShouldSendProtobufContent(), RequestPhase, skrProto, ContextLoadResponse, LogContext);
    EnrichRequestAdditionalOptions(skrProto, SessionCtx, ContextLoadResponse);
    EnrichRequestWithGuestData(skrProto, SpeakerService.GetActiveSpeaker(), ContextLoadResponse);

    const TString url = BuildVinsUrl();
    sessionLog["EffectiveVinsUrl"] = url;
    const bool legacyJson = url.StartsWith("http://vins.alice.yandex.net/speechkit/app/naviapp");
    const TString content = BuildContent(skrProto, sessionLog["Body"], legacyJson);
    const TString headers = BuildHeaders(skrProto, rtLogChild, url, isFinal, legacyJson);

    NNeh::TMessage msg{url, {}};
    NNeh::NHttp::MakeFullRequest(msg, headers, content);
    return msg;
}

bool TMegamindRequestBuilder::ShouldSendProtobufContent() const {
    const auto& expFlags = RequestCtx.GetExpFlags();
    const auto sendProtoExpFlagIt = expFlags.find(NExpFlags::SEND_PROTOBUF_TO_MEGAMIND);
    return sendProtoExpFlagIt != expFlags.end() && IsTrue(sendProtoExpFlagIt->second);
}

TString TMegamindRequestBuilder::BuildVinsUrl() const {
    // python version - https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/vins/vinsrequest.py?rev=r7939667#L463-522
    TString envUrl = GetEnv("UNIPROXY_VINS_BACKEND");
    if (!envUrl.Empty()) {
        return envUrl;
    }

    TMaybe<TString> srcrwrUrl;
    if (AppHostParams) {
        if (const auto* url = AppHostParams->GetValueByPath("srcrwr.VINS"); url && url->IsString()) {
            srcrwrUrl = url->GetString();
        }
    }

    TMaybe<TString> uaasUrl;
    if (ContextLoadResponse.HasFlagsInfo()) {
        NVoice::NExperiments::TFlagsJsonFlagsConstRef flagsInfoRef(&ContextLoadResponse.GetFlagsInfo());
        if (const TMaybe<TString> flagValue = flagsInfoRef.GetValueFromName("UaasVinsUrl_"sv)) {
            try {
                uaasUrl = Base64Decode(*flagValue);
            } catch (...) {
                LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Can't decode base64 from " << *flagValue);
            }
        }
    }

    TMaybe<TString> payloadUrl;
    if (RequestCtx.HasVinsUrl()) {
        payloadUrl = RequestCtx.GetVinsUrl();
    }

    // Changing base url to real url
    NUri::TUri uri;
    if (payloadUrl) {
        NUri::TUri::EParsed parsed = uri.ParseAbsOrHttpUri(*payloadUrl, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeKnown);
        if (parsed != NUri::TUri::ParsedOK) {
            throw yexception() << "got invalid payloadUrl=" << *payloadUrl << " : " << NUri::ParsedStateToString(parsed);
        }
    } else {
        uri.ParseAbsOrHttpUri(TString::Join(Config.megamind().default_url(), Config.megamind().default_run_suffix()));
    }

    if (RequestPhase == ERequestPhase::APPLY) {
        NUri::TUriUpdate{uri}.Set(NUri::TField::EField::FieldPath, Config.megamind().default_apply_suffix());
    }

    if (uaasUrl && (!uri || VINS_PRODUCTION_HOSTS.contains(uri.GetHost()))) {
        TString prevPath = uri.IsNull() ? "" : TString{uri.GetField(NUri::TField::EField::FieldPath)};
        uri.ParseAbsOrHttpUri(*uaasUrl);
        if (!prevPath.Empty()) {
            const TStringBuf curPath = uri.GetField(NUri::TField::EField::FieldPath);
            if (curPath == "/") {
                NUri::TUriUpdate{uri}.Set(NUri::TField::EField::FieldPath, prevPath);
            } else {
                TString newPath = TString::Join(curPath, prevPath);
                NUri::TUriUpdate{uri}.Set(NUri::TField::EField::FieldPath, newPath);
            }
        }
    }

    if (srcrwrUrl) {
        uri.ParseAbsOrHttpUri(*srcrwrUrl);
        if (RequestPhase == ERequestPhase::APPLY) {
            NUri::TUriUpdate{uri}.Set(NUri::TField::EField::FieldPath, Config.megamind().default_apply_suffix());
        }
    }

    if (AppHostParams && VINS_PRODUCTION_HOSTS.contains(uri.GetHost())) {
        if (const auto* host = AppHostParams->GetValueByPath("srcrwr.VINS_HOST"); host && host->IsString()) {
            NUri::TUriUpdate{uri}.Set(NUri::TField::EField::FieldHost, host->GetString());
        }
    }

    // TODO(sparkle): normal logic for SRCRWR

    return uri.PrintS();
}

TString TMegamindRequestBuilder::BuildContent(const NAlice::TSpeechKitRequestProto& skrProto, NJson::TJsonValue& jReq, bool legacyJson) const {
    NJson::TJsonValue skrJson = NAlice::JsonFromProto(skrProto);
    if (ShouldSendProtobufContent() && !legacyJson) {
        jReq = std::move(skrJson);
        return skrProto.SerializeAsString();
    } else {
        using namespace NJson;
        // send json content
        if (legacyJson) {  // VOICESERV-4167 - patch json to more legacy (python VINS) format
            const auto& requestProto = skrProto.GetRequest();
            auto& request = skrJson["request"];
            auto& header = skrJson["header"];
            if (requestProto.HasExperiments()) {
                NMegamind::TExpFlagsToJsonVisitor{request["experiments"]}.Visit(requestProto.GetExperiments());
                request.EraseValue("Experiments");
            }
            if (!header.Has("prev_req_id")) {
                header["prev_req_id"] = JSON_NULL;  // required field
            }
            if (TJsonValue* asrResult = skrJson.GetValueByPath("request.event.asr_result") ; asrResult && asrResult->IsArray()) {
                for (auto& hyp : asrResult->GetArraySafe()) {
                    if (!hyp.IsMap()) {
                        continue;  // ignore strange format
                    }

                    TStringStream utterance;
                    if (TJsonValue* words = hyp.GetValueByPath("words"); words && words->IsArray()) {
                        for (auto& word : words->GetArraySafe()) {
                            if (!word.IsMap()) {
                                continue;  // ignore strange format
                            }

                            word["confidence"sv] = 1;
                            TString s;
                            if (NJson::GetString(word, "value", &s) && s.size()) {
                                if (utterance.Str().size()) {
                                    utterance << ' ';
                                }
                                utterance << s;
                            }
                        }
                    }
                    hyp["utterance"sv] = utterance.Str();
                }
            }
        }
        const TString skrContent = NAlice::JsonToString(skrJson);
        jReq = std::move(skrJson);
        return skrContent;
    }
}

TString TMegamindRequestBuilder::BuildHeaders(const NAlice::TSpeechKitRequestProto& skrProto, TRTLogActivation& rtLogChild, TStringBuf targetUrl, bool isFinal, bool legacyJson) const {
    // python version - https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/vins/vinsrequest.py?rev=r7939667#L718
    TStringBuilder headers;
    if (ShouldSendProtobufContent() && !legacyJson) {
        headers << "Content-Type: application/protobuf\r\n";
    } else {
        headers << "Content-Type: application/json\r\n";
    }
    if (ContextLoadResponse.HasUserTicket()) {
        headers << "X-Ya-User-Ticket: " << ContextLoadResponse.GetUserTicket() << "\r\n";
    }
    if (skrProto.GetHeader().HasRequestId()) {
        headers << "x-alice-client-reqid: " << skrProto.GetHeader().GetRequestId() << "\r\n";
    }
    if (SessionCtx.HasAppType()) {
        headers << "X-Alice-AppType: " << SessionCtx.GetAppType() << "\r\n";
    }
    if (SessionCtx.HasAppId()) {
        headers << "X-Alice-AppId: " << SessionCtx.GetAppId() << "\r\n";
    }

    if (RequestCtx.GetSettingsFromManager().HasBalancingModeMegamind()) {
        TBalancingHintHolder::AddBalancingHint(RequestCtx.GetSettingsFromManager().GetBalancingModeMegamind(), headers);
    }

    // TODO(sparkle): add srcrwr - https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/vins/vinsrequest.py?rev=r7939667#L736
    //                             https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/utils/srcrwr.py?rev=r7943652#L6

    if (rtLogChild.Token()) {
        headers << "X-RTLog-Token: " << rtLogChild.Token() << "\r\n";
        headers << "X-Yandex-Req-Id: " << rtLogChild.Token() << "\r\n";
    }
    try {
        const TString &name = FQDNHostName();
        headers << "X-Ya-Servant-Hostname: " << name << "\r\n";
    } catch (...) {
        headers << "X-Ya-Servant-Hostname: unknown\r\n";
    }

    headers << "X-Yandex-Target-Megamind-Url: " << targetUrl << "\r\n";

    headers << "X-Yandex-Internal-Request: 1\r\n";

    if (const TString dump = AliceLoggerOptions.SerializeAsString(); !dump.empty()) {
        headers << "X-Alice-Logger-Options: " << Base64Encode(dump) << "\r\n";
    }

    // Header used by rate limiter on load balancer to truncate intermediate ASR predictions on significant load growth
    headers << "X-Alice-IsPartial: " << (isFinal ? 0 : 1) << "\r\n";

    return std::move(headers);
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
