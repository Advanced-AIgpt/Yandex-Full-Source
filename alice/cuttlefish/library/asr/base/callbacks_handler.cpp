#include "callbacks_handler.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/protos/asr.pb.h>
#include <apphost/api/service/cpp/service_exceptions.h>

using namespace NAlice::NAsr;

void TCallbacksHandler::OnInitResponse(const NProtobuf::TResponse& response) {
    HasInitResponse_ = true;
    if (Closed_) {
        return;
    }

    const auto& initResponse = response.GetInitResponse();
    bool isFinalResponse = initResponse.HasIsOk() && !initResponse.GetIsOk();
    AddAndFlush(response, isFinalResponse);
}

void TCallbacksHandler::OnSpotterValidation(bool valid) {
    if (Closed_) {
        return;
    }

    NAliceProtocol::TSpotterValidation spotterValidation;
    spotterValidation.SetValid(valid);
    RequestHandler_->Context().AddProtobufItem(spotterValidation, NCuttlefish::ITEM_TYPE_ASR_SPOTTER_VALIDATION);
}

void TCallbacksHandler::OnAddDataResponse(const NProtobuf::TResponse& response) {
    if (Closed_) {
        return;
    }

    if (!response.HasAddDataResponse()) {
        // handle hearbeat & other messages
        AddAndFlush(response, /*final response=*/false);
        return;
    }

    const auto& addDataResponse = response.GetAddDataResponse();
    bool isFinalResponse = addDataResponse.HasIsOk() && !addDataResponse.GetIsOk();
    NProtobuf::TResponse responseWithNumber(response);
    responseWithNumber.MutableAddDataResponse()->SetNumber(AddDataResponseNumber_++);
    AddAndFlush(responseWithNumber, isFinalResponse);
}

void TCallbacksHandler::OnSessionLog(const NAliceProtocol::TSessionLogRecord& sessionLog) {
    if (Closed_) {
        return;
    }

    NAliceProtocol::TUniproxyDirective directive;
    *directive.MutableSessionLog() = sessionLog;
    RequestHandler_->Context().AddProtobufItem(directive, NCuttlefish::ITEM_TYPE_UNIPROXY2_DIRECTIVE);
    RequestHandler_->Context().IntermediateFlush();
}

void TCallbacksHandler::OnClosed() {
    if (Closed_) {
        return;
    }

    AddAsrFinished(NAliceProtocol::TAsrFinished());
    RequestHandler_->Context().Flush();
    Closed_ = true;
}

void TCallbacksHandler::OnAnyError(const TString& error, bool fastError) {
    if (Closed_) {
        return;
    }

    // send fastError to AppHostContext: https://st.yandex-team.ru/APPHOSTSUPPORT-287#603f3def2f11735d78c21dbc
    if (fastError) {
        RequestHandler_->SetException(std::make_exception_ptr(NAppHost::NService::TFastError()));
        Closed_ = true;
        return;
    }

    NProtobuf::TResponse response;
    if (HasInitResponse_) {
        auto& addDataResponse2 = *response.MutableAddDataResponse();
        NProtobuf::FillRequiredDefaults(addDataResponse2);
        addDataResponse2.SetIsOk(false);
        addDataResponse2.SetErrMsg(error);
        addDataResponse2.SetNumber(AddDataResponseNumber_++);
    } else {
        auto& initResponse2 = *response.MutableInitResponse();
        NProtobuf::FillRequiredDefaults(initResponse2);
        initResponse2.SetIsOk(false);
        initResponse2.SetErrMsg(error);
    }

    AddAndFlush(response, true);
}

void TCallbacksHandler::AddAsrFinished(const NAliceProtocol::TAsrFinished& asrFinished) {
    RequestHandler_->Context().AddProtobufItem(asrFinished, NCuttlefish::ITEM_TYPE_ASR_FINISHED);
}

void TCallbacksHandler::AddAndFlush(const NProtobuf::TResponse& response, bool isFinalResponse) {
    if (Closed_) {
        return;
    }

    RequestHandler_->Context().AddProtobufItem(response, NCuttlefish::ITEM_TYPE_ASR_PROTO_RESPONSE);
    if (isFinalResponse) {
        AddAsrFinished(NAliceProtocol::TAsrFinished());
        RequestHandler_->Context().Flush();
        Closed_ = true;
    } else {
        RequestHandler_->Context().IntermediateFlush();
    }
}
