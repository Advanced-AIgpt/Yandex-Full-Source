#include "callbacks_handler.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <apphost/api/service/cpp/service_exceptions.h>

using namespace NAlice::NYabio;
using namespace NAlice::NCuttlefish;

void TCallbacksHandler::OnInitResponse(const NProtobuf::TResponse& response) {
    HasInitResponse_ = true;
    if (Closed_) {
        return;
    }

    const auto& initResponse = response.GetInitResponse();
    bool isFinalResponse = initResponse.GetresponseCode() != NProtobuf::RESPONSE_CODE_OK;
    AddAndFlush(response, isFinalResponse);
}

void TCallbacksHandler::OnAddDataResponse(const NProtobuf::TResponse& response) {
    if (Closed_) {
        return;
    }

    NProtobuf::TAddDataResponse addDataResponse{response.GetAddDataResponse()};
    if (addDataResponse.has_context()) {
        NAliceProtocol::TBioContextSaveNewEnrolling newEnrolling;
        newEnrolling.MutableYabioEnrolling()->Swap(addDataResponse.mutable_context()->mutable_enrolling());
        //?or for (auto& e : addDataResponse.context().enrolling()) {
        //    newEnrolling.AddYabioEnrolling(e);
        //}
        //TODO: enreach enrollings here or in save service?
        //            for e in res.context.enrolling:
        //                e.request_id = self.message_id
        //                result["request_id"] = e.request_id
        //                e.timestamp = int(time.time())
        //                e.device_model = self._system.device_model or 'unknown'
        //                e.device_id = get_by_path(self.params, 'request', 'device_state', 'device_id') or 'unknown'
        //                e.device_manufacturer = get_by_path(
        //                    self.params, 'vins', 'application', 'device_manufacturer') or 'unknown'
        newEnrolling.MutableSupportedTags()->Swap(addDataResponse.mutable_supported_tags());
        //? or for (auto& st : addDataResponse.supported_tags()) {
        //    newEnrolling.AddSupportedTags(st);
        //}
        AddNewEnrolling(newEnrolling);
        // addDataResponse.clear_context();
    }
    bool isFinalResponse = addDataResponse.GetresponseCode() != NProtobuf::RESPONSE_CODE_OK;
    AddAndFlush(response, isFinalResponse);
}

void TCallbacksHandler::OnClosed() {
    if (Closed_) {
        return;
    }

    Finish();
    Closed_ = true;
}

void TCallbacksHandler::OnAnyError(NProtobuf::EResponseCode responseCode, const TString& error, bool fastError) {
    (void)error;  // current yabio protocol not has field for error text
    if (Closed_) {
        return;
    }

    // send fastError to AppHostContext: https://st.yandex-team.ru/APPHOST-3439#5fd219a92cd7b5352a65f6c0
    if (fastError) {
        RequestHandler_->SetException(std::make_exception_ptr(NAppHost::NService::TFastError()));
        Closed_ = true;
        return;
    }

    NProtobuf::TResponse response;
    if (HasInitResponse_) {
        auto addDataResponse2 = response.MutableAddDataResponse();
        addDataResponse2->SetresponseCode(responseCode);
    } else {
        auto& initResponse2 = *response.MutableInitResponse();
        NProtobuf::FillRequiredDefaults(initResponse2);
        initResponse2.SetresponseCode(responseCode);
    }
    AddAndFlush(response, true);
}

void TCallbacksHandler::AddNewEnrolling(const NAliceProtocol::TBioContextSaveNewEnrolling& newEnrolling) {
    if (Closed_) {
        return;
    }

    RequestHandler_->Context().AddProtobufItem(newEnrolling, ITEM_TYPE_YABIO_NEW_ENROLLING);
}

void TCallbacksHandler::AddAndFlush(const NProtobuf::TResponse& response, bool isFinalResponse) {
    if (Closed_) {
        return;
    }

    RequestHandler_->Context().AddProtobufItem(response, ITEM_TYPE_YABIO_PROTO_RESPONSE);
    if (isFinalResponse) {
        RequestHandler_->Context().Flush();
        Closed_ = true;
    } else {
        RequestHandler_->Context().IntermediateFlush();
    }
}
