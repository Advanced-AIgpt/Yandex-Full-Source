#include "biometry.h"
#include "support_functions.h"

#include <alice/cuttlefish/library/experiments/flags_json.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <library/cpp/json/writer/json.h>
#include <util/stream/mem.h>
#include <util/stream/tokenizer.h>
#include <util/string/builder.h>
#include <util/system/hostname.h>
#include <voicetech/library/idl/log/events.ev.pb.h>
#include <voicetech/library/uniproxy2/unistat.h>

using namespace NAlice::NYabio;
using namespace NJson;
using namespace NVoicetech::NUniproxy2;

TString NAlice::NCuttlefish::NAppHostServices::GetGroupId(const NVoicetech::NUniproxy2::TMessage& message, const TString& defautGroupId) {
    const TJsonValue& payload = NSupport::GetJsonValueByPathOrThrow(message.Json, TStringBuf("event.payload"));
    if (!payload.IsMap()) {
        return defautGroupId;
    }

    const TJsonValue* advancedOptions = nullptr;
    if (!advancedOptions) {
        if (!payload.GetValuePointer(TStringBuf("advancedASROptions"), &advancedOptions)) {
            if (!payload.GetValuePointer(TStringBuf("advanced_options"), &advancedOptions)) {
                payload.GetValuePointer(TStringBuf("advancedOptions"), &advancedOptions);
            }
        }
    }
    if (advancedOptions && !advancedOptions->IsMap()) {
        ythrow yexception() << "advanced asr options MUST be dict type";
    }

    TString groupId;
    if (!GetString(payload, TStringBuf("biometry_group"), &groupId)) {
        if (advancedOptions && GetString(*advancedOptions, TStringBuf("biometry_group"), &groupId)) {
            // done
        } else {
            if (defautGroupId) {
                groupId = defautGroupId;
            } else {
                GetString(payload, TStringBuf("uuid"), &groupId);  // crutch/fallback for biometry_group
            }
        }
    }
    return groupId;
}

namespace {
    using namespace NAlice::NCuttlefish::NAppHostServices;
    struct TIsComma {
        inline bool operator()(const char ch) const noexcept {
            return ',' == ch;
        }
    };

    bool TryFillTags(const TJsonValue& tags,  NProtobuf::TInitRequest& initRequest) {
        if (tags.IsString()) {
            // split by ','
            auto&& input = TMemoryInput{tags.GetString()};
            auto&& tokenizer = TStreamTokenizer<TIsComma>{&input};
            for (auto it = tokenizer.begin(); tokenizer.end() != it; ++it) {
                if (it->Length()) {
                    initRequest.Addclassification_tags(it->Data(), it->Length());
                }
            }
            return true;
        } else if (tags.IsArray()) {
            for (auto& val : tags.GetArray()) {
                if (val.IsString()) {
                    initRequest.Addclassification_tags(val.GetString());
                }
            }
            return true;
        } else {
            return false;
        }
    }

    void MessageToYabioInitRequest(
        const TMessage& message,
        NProtobuf::TInitRequest& initRequest,
        const NAliceProtocol::TSessionContext& sessionContext,
        const NAliceProtocol::TRequestContext& requestContext,
        NAlice::NCuttlefish::TLogContext* logContext
    ) {
        (void)requestContext;
        const TMessage::THeader& header = NSupport::GetHeaderOrThrow(message);
        const TJsonValue& payload = NSupport::GetJsonValueByPathOrThrow(message.Json, TStringBuf("event.payload"));
        if (!payload.IsMap()) {
            return;
        }

        // ignore version: optional int32 protocolVersion = 1 [default = 1];
        {
            TString mime;
            GetString(payload, TStringBuf("mime"), &mime);
            if (!mime) {
                GetString(payload, TStringBuf("format"), &mime);
            }
            initRequest.Setmime(mime);
        }
        initRequest.SetsessionId(sessionContext.GetSessionId());
        {
            TString uuid;
            if (sessionContext.HasUserInfo()) {
                auto& userInfo = sessionContext.GetUserInfo();
                if (userInfo.HasUuid()) {
                    uuid = userInfo.GetUuid();
                }
            }
            initRequest.set_uuid(uuid);
        }
        const TJsonValue* advancedOptions = nullptr;
        if (!advancedOptions) {
            if (!payload.GetValuePointer(TStringBuf("advancedASROptions"), &advancedOptions)) {
                if (!payload.GetValuePointer(TStringBuf("advanced_options"), &advancedOptions)) {
                    payload.GetValuePointer(TStringBuf("advancedOptions"), &advancedOptions);
                }
            }
        }
        if (advancedOptions && !advancedOptions->IsMap()) {
            ythrow yexception() << "advanced asr options MUST be dict type";
        }
        {
            TString groupId = requestContext.GetBiometryOptions().GetGroup();
            initRequest.set_group_id(GetGroupId(message, groupId));  // REQUIRED
        }
        {
            TString userId;
            if (GetString(payload, TStringBuf("user_id"), &userId)) {
                initRequest.set_user_id(userId);
            }
        }
        {
            bool hasSpotter = false;
            if (!GetBoolean(payload, TStringBuf("enable_spotter_validation"), &hasSpotter)) {
                if (advancedOptions) {
                    GetBoolean(*advancedOptions, TStringBuf("spotter_validation"), &hasSpotter);
                }
            }
            initRequest.set_spotter(hasSpotter);
        }
        {
            static const TString defaultHostname = "unknown";
            TString hostname = defaultHostname;
            try {
                hostname = FQDNHostName();
            } catch (...) {
            }
            if (hostname) {
                initRequest.SetclientHostname(hostname);
            }
            GetString(payload, TStringBuf("hostName"), &hostname);
            if (hostname) {
                initRequest.SethostName(hostname);
            }
        }
        {
            const bool isMultiaccountModeEnabled = payload.GetValueByPath("request.enrollment_headers") != nullptr;
            initRequest.mutable_advanced_options()->set_is_multiaccount_mode_enabled(isMultiaccountModeEnabled);
        }
        try {
            // see /uniproxy/library/uaas/__init__.py get_ab_config('BIO') usage
            using namespace NVoice::NExperiments;
            if (auto flagsInfoRef = MakeFlagsConstRefFromSessionContextProto(sessionContext)) {
                TMaybe<TString> bioFlags = flagsInfoRef->GetBioAbFlagsSerializedJson();
                if (bioFlags.Defined()) {
                    initRequest.set_experiments(std::move(bioFlags.GetRef()));
                }
            }
        } catch (...) {
            // invalid UAAS response format can cause exception, - log this (if can)
            DLOG("fail get AB experiment" << CurrentExceptionMessage());
            if (logContext) {
                logContext->LogEvent(NEvClass::WarningMessage(
                    TStringBuilder() << "fail get flags.json experiments for BIO: " << CurrentExceptionMessage()
                ));
            }
        }
        {
            initRequest.SetMessageId(header.MessageId);
            // hack with using test id for enrolling
            const TJsonValue* enrollingId = nullptr;
            if (payload.GetValuePointer(TStringBuf("_test_request_ids"), &enrollingId)) {
                if (enrollingId->IsArray() && enrollingId->GetArray().size()) {
                    initRequest.SetTestRequestId((*enrollingId)[0].GetString());
                }
            }
        }
    }
}

void NAlice::NCuttlefish::NAppHostServices::MessageToYabioInitRequestClassify(
    const TMessage& message,
    NProtobuf::TInitRequest& initRequest,
    const NAliceProtocol::TSessionContext& sessionContext,
    const NAliceProtocol::TRequestContext& requestContext,
    TLogContext* logContext
) {
    MessageToYabioInitRequest(message, initRequest, sessionContext, requestContext, logContext);
    initRequest.set_method(YabioProtobuf::Method::Classify);  // REQUIRED
    const TJsonValue& payload = NSupport::GetJsonValueByPathOrThrow(message.Json, TStringBuf("event.payload"));
    if (requestContext.GetBiometryOptions().HasClassify()) {
        // split by ','
        auto&& input = TMemoryInput{requestContext.GetBiometryOptions().GetClassify()};
        auto&& tokenizer = TStreamTokenizer<TIsComma>{&input};
        for (auto it = tokenizer.begin(); tokenizer.end() != it; ++it) {
            if (it->Length()) {
                initRequest.Addclassification_tags(it->Data(), it->Length());
            }
        }
    } else {
        if (!TryFillTags(payload[TStringBuf("classification_tags")], initRequest)) {
            TryFillTags(payload[TStringBuf("biometry_classify")], initRequest);
        }
    }
}

void NAlice::NCuttlefish::NAppHostServices::MessageToYabioInitRequestScore(
    const TMessage& message,
    NProtobuf::TInitRequest& initRequest,
    const NAliceProtocol::TSessionContext& sessionContext,
    const NAliceProtocol::TRequestContext& requestContext,
    TLogContext* logContext
) {
    MessageToYabioInitRequest(message, initRequest, sessionContext, requestContext, logContext);
    initRequest.set_method(YabioProtobuf::Method::Score);  // REQUIRED
    // TODO:? repeated string requests_ids = 10;
}

void NAlice::NCuttlefish::NAppHostServices::BiometryAddDataResponseToJson(
    const NAlice::NYabio::NProtobuf::TAddDataResponse& addDataResponse,
    NJson::TJsonValue& payload,
    NAlice::NYabio::NProtobuf::EMethod method
) {
    payload["status"] = "ok";
    payload["messagesCount"] = addDataResponse.GetmessagesCount();
    if (method == NAlice::NYabio::NProtobuf::METHOD_SCORE) {
        // {"status":"ok","scores_with_mode":[{"mode":"default",
        // "scores":[{"user_id":"uniclient_test_user_id","score":0.9845389127731323}]},{"mode":"high_tnr","scores":[{"user_id":"uni
        // client_test_user_id","score":0.9845389127731323}]},{"mode":"high_tpr","scores":[{"user_id":"uniclient_test_user_id","score":0.9845389127731323}]},
        // {"mode":"max_accuracy","scores":[{"user_id":"uniclient_test_user_id","score":0.9845389127731323}]}],"group_id":"uniproxy_test","request_id":"request_1"}}}
        payload["group_id"] = addDataResponse.context().group_id();
        if (addDataResponse.has_request_id()) {
            payload["request_id"] = addDataResponse.request_id();
        }
        payload["scores_with_mode"] = NJson::JSON_ARRAY;  // we now has at least empty array, so emulate this
        for (auto& sm : addDataResponse.scores_with_mode()) {
            TJsonValue jSM;
            jSM["mode"] = sm.mode();
            for (auto& userScore : sm.scores()) {
                jSM["scores"]["user_id"] = userScore.user_id();
                jSM["scores"]["score"] = userScore.score();
            }
            payload["scores_with_mode"].AppendValue(std::move(jSM));
        }
    } else {
        for (auto& result : addDataResponse.Getclassification()) {
            TJsonValue jRes;
            jRes["classname"] = result.classname();
            jRes["confidence"] = result.confidence();
            jRes["tag"] = result.tag();
            payload["bioResult"].AppendValue(std::move(jRes));
        }
        for (auto& result : addDataResponse.GetclassificationResults()) {
            TJsonValue jRes;
            jRes["tag"] = result.tag();
            jRes["classname"] = result.classname();
            payload["classification_results"].AppendValue(std::move(jRes));
        }
    }
}
