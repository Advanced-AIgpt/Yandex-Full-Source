#include "client.h"

#include <alice/megamind/api/response/constructor.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/json/json.h>

using namespace NAlice::NCuttlefish;

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

constexpr TStringBuf RUN_PHASE_METRICS_NAME = "megamind_client.run";
constexpr TStringBuf APPLY_PHASE_METRICS_NAME = "megamind_client.apply";

constexpr int BAD_MEGAMIND_ANSWER_HTTP_CODE = 512; // the answer is bad, but we can show it to the user

}  // anonymous namespace


TMegamindClient::TMegamindClient(
    const ERequestPhase phase,
    const NAliceCuttlefishConfig::TConfig& config,
    const NAliceProtocol::TSessionContext& sessionCtx,
    TLogContext logContext
): Phase(phase)
    , Config(config)
    , Metrics(sessionCtx, phase == ERequestPhase::RUN ? RUN_PHASE_METRICS_NAME : APPLY_PHASE_METRICS_NAME)
    , LogContext(logContext)
{
}

NThreading::TFuture<NAliceProtocol::TMegamindResponse> TMegamindClient::SendRequest(
    NNeh::TMessage&& msg,
    int asrPartialNumber,
    TAtomicSharedPtr<NAlice::NCuttlefish::TRTLogActivation> rtLogChild
) {
    auto promise = NThreading::NewPromise<NAliceProtocol::TMegamindResponse>();
    auto result = promise.GetFuture();

    try {
        //ALARM: we log here request/additional_options/oauth_token as is, so we MUST NOT send this event to RTLog (without censoring)
        LogContext.LogEvent(NEvClass::DebugMessage(TStringBuilder() << "Request: addr='" << msg.Addr << "' data='" << msg.Data << "'"));

        SendHttpRequest(
            std::move(msg),
            Config.megamind().timeout(),
            Config.megamind().additional_attempts(),
            [self = TIntrusivePtr<TMegamindClient>(this), promise, rtLogChild](NNeh::TResponseRef resp) mutable {
                if (!resp) {
                    self->Metrics.PushRate("response", "timeout", "megamind");
                    self->LogContext.LogEvent(NEvClass::ErrorMessage("TMegamindClient timeouted"));
                    promise.SetException(std::make_exception_ptr(TErrorWithCode("timeout", "Timeout")));
                    rtLogChild->Finish(false, "TMegamindClient timeouted");
                    return;
                }

                self->LogContext.LogEvent(
                    NEvClass::InfoMessage(
                        TStringBuilder()
                            << "TMegamindClient (phase " << self->Phase << ") textual response ("
                            << "duration=" << resp->Duration << " "
                            << "firstLine=" << resp->FirstLine << " "
                            << "dataLength=" << resp->Data.size() << "): " << resp->Data
                    )
                );

                static const TString noError("http.200");
                TString errorCode = noError;
                TString errorText;
                if (resp->IsError()) {
                    if (resp->GetErrorType() == NNeh::TError::Cancelled) {
                        static const TString canceled("canceled");
                        errorCode = canceled;
                    } else if (resp->GetErrorType() == NNeh::TError::ProtocolSpecific) {
                        errorCode = TStringBuilder() << TStringBuf("http.") << resp->GetErrorCode();
                        errorText = resp->GetErrorText();
                    } else if (const auto code = resp->GetSystemErrorCode()) {
                        errorCode = TStringBuilder() << TStringBuf("sys.") << resp->GetErrorCode();
                        errorText = resp->GetErrorText();
                    } else {
                        errorCode = TStringBuilder() << TStringBuf("neh.") << resp->GetErrorCode();
                        errorText = resp->GetErrorText();
                    }
                }
                self->Metrics.PushRate("response", errorCode, "megamind");
                if (
                    const THttpInputHeader* vinsOkHeader = resp->Headers.FindHeader("x-yandex-vins-ok");
                    resp->IsError() &&
                    !(
                        // See VA-2463, VOICESERV-1617, DIALOG-637
                        resp->GetErrorCode() == BAD_MEGAMIND_ANSWER_HTTP_CODE &&
                        (vinsOkHeader && vinsOkHeader->Value() == "true"sv)
                    )
                ) {
                    promise.SetException(std::make_exception_ptr(TErrorWithCode(errorCode, errorText)));
                    rtLogChild->Finish(false, errorText);
                } else {
                    TString jsonResponseBody;
                    try {
                        const THttpInputHeader* contentType = resp->Headers.FindHeader("content-type"sv);
                        if (contentType && contentType->Value() == "application/protobuf"sv) {
                            NAliceProtocol::TMegamindResponse mmResponse;
                            {
                                if (!mmResponse.MutableProtoResponse()->ParseFromString(resp->Data)) {
                                    self->Metrics.PushRate("response", "invalid_protobuf");
                                    self->LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>("MM response=invalid_protobuf (ignore problem&continue)");
                                }

                                auto respConstructor = NAlice::NMegamindApi::TResponseConstructor();
                                respConstructor.PushSpeechKitProto(mmResponse.GetProtoResponse());
                                NJson::TJsonValue respJson = std::move(respConstructor).MakeResponse();
                                mmResponse.SetRawJsonResponse(NJson::WriteJson(respJson, /* formatOutput */ false));
                            }

                            rtLogChild->Finish();  // SUCCESS
                            promise.SetValue(std::move(mmResponse));
                        } else {
                            jsonResponseBody = resp->Data;
                            const NJson::TJsonValue json = NAlice::JsonFromString(resp->Data);
                            auto proto = NAlice::JsonToProto<NAlice::TSpeechKitResponseProto>(json, /* validateUtf8 = */ false, /* ignoreUnknownFields = */ true);

                            NAliceProtocol::TMegamindResponse mmResponse;
                            mmResponse.SetRawJsonResponse(resp->Data);
                            mmResponse.MutableProtoResponse()->Swap(&proto);

                            rtLogChild->Finish();  // SUCCESS
                            promise.SetValue(std::move(mmResponse));
                        }
                    } catch (...) {
                        static const TString failParseResponse{"fail_parse_response"};
                        promise.SetException(std::make_exception_ptr(TErrorWithCode(failParseResponse, CurrentExceptionMessage())));
                        rtLogChild->Finish(false, CurrentExceptionMessage());
                        if (jsonResponseBody) {
                            self->LogContext.LogEvent(NEvClass::ErrorMessage(TStringBuilder() << "BAD_MM_RAW_RESPONSE:"sv << jsonResponseBody));
                        }
                    }
                }
            },
            LogContext,
            asrPartialNumber
        );
        Metrics.PushRate("request", "sent");
    } catch (...) {
        static const TString failSendRequest{"fail_send_request"};
        Metrics.PushRate("request", "failed");
        const TErrorWithCode ec{
                failSendRequest,
                TStringBuilder{} << "Failed to send request " << CurrentExceptionMessage()
        };
        LogContext.LogEvent(NEvClass::ErrorMessage(ec.Text()));
        promise.SetException(std::make_exception_ptr(ec));
    }
    return result;
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
