#include "callbacks_handler.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

using namespace NAlice::NTtsCache;

void TCallbacksHandler::OnCacheSetRequestCompleted(const TString& key, const TMaybe<TString>& error) {
    if (Closed_) {
        return;
    }

    Y_UNUSED(key);
    Y_UNUSED(error);
}

void TCallbacksHandler::OnCacheWarmUpRequestCompleted(const TString& key, const TMaybe<TString>& error) {
    if (Closed_) {
        return;
    }

    Y_UNUSED(key);
    Y_UNUSED(error);
}

void TCallbacksHandler::OnCacheGetResponse(const NProtobuf::TCacheGetResponse& cacheGetResponse) {
    if (Closed_) {
        return;
    }

    AddCacheGetResponseAndFlush(cacheGetResponse);
}

void TCallbacksHandler::OnClosed() {
    if (Closed_) {
        return;
    }

    FlushAppHostContext(/* isFinalFlush = */ true);
}

void TCallbacksHandler::OnAnyError(const TString& error) {
    if (Closed_) {
        return;
    }

    Y_UNUSED(error);
}

void TCallbacksHandler::Finish() {
    RequestHandler_->Finish();
}

void TCallbacksHandler::AddCacheGetResponseAndFlush(const NProtobuf::TCacheGetResponse& cacheGetResponse) {
    if (Closed_) {
        return;
    }

    RequestHandler_->Context().AddProtobufItem(cacheGetResponse, NAlice::NCuttlefish::ITEM_TYPE_TTS_CACHE_GET_RESPONSE);
    {
        NProtobuf::TCacheGetResponseStatus cacheGetResponseStatus;
        cacheGetResponseStatus.SetKey(cacheGetResponse.GetKey());
        cacheGetResponseStatus.SetStatus(cacheGetResponse.GetStatus());
        cacheGetResponseStatus.SetErrorMessage(cacheGetResponse.GetErrorMessage());

        RequestHandler_->Context().AddProtobufItem(cacheGetResponseStatus, NAlice::NCuttlefish::ITEM_TYPE_TTS_CACHE_GET_RESPONSE_STATUS);
    }

    FlushAppHostContext(/* isFinalFlush = */ false);
}

void TCallbacksHandler::FlushAppHostContext(bool isFinalFlush) {
    if (isFinalFlush) {
        RequestHandler_->Context().Flush();
        Closed_ = true;
    } else {
        RequestHandler_->Context().IntermediateFlush();
    }
}
