#include "callbacks_handler.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

using namespace NAlice::NMusicMatch;

void TCallbacksHandler::OnInitResponse(const NProtobuf::TInitResponse& initResponse) {
    HasInitResponse_ = true;
    if (!AppHostContext_) {
        return;
    }

    bool isFinalResponse = initResponse.HasIsOk() && !initResponse.GetIsOk();
    AddInitResponseAndFlush(initResponse, isFinalResponse);
}

void TCallbacksHandler::OnStreamResponse(const NProtobuf::TStreamResponse& streamResponse) {
    if (!AppHostContext_) {
        return;
    }

    const auto& musicResult = streamResponse.GetMusicResult();
    bool isFinalResult = (!musicResult.GetIsOk() || musicResult.GetIsFinish());
    AddStreamResponseAndFlush(streamResponse, isFinalResult);
}

void TCallbacksHandler::OnClosed() {
    if (!AppHostContext_) {
        return;
    }

    FlushAppHostContext(true);
}

void TCallbacksHandler::OnAnyError(const TString& error) {
    if (!AppHostContext_) {
        return;
    }

    if (HasInitResponse_) {
        NProtobuf::TStreamResponse streamResponse;
        auto musicResult = streamResponse.MutableMusicResult();
        musicResult->SetIsOk(false);
        musicResult->SetErrorMessage(error);

        AddStreamResponseAndFlush(streamResponse, true);
    } else {
        NProtobuf::TInitResponse initResponse;
        initResponse.SetIsOk(false);
        initResponse.SetErrorMessage(error);

        AddInitResponseAndFlush(initResponse, true);
    }
}

void TCallbacksHandler::AddInitResponseAndFlush(const NProtobuf::TInitResponse& initResponse, bool isFinalResponse) {
    AppHostContext_->AddProtobufItem(initResponse, NCuttlefish::ITEM_TYPE_MUSIC_MATCH_INIT_RESPONSE);
    FlushAppHostContext(isFinalResponse);
}

void TCallbacksHandler::AddStreamResponseAndFlush(const NProtobuf::TStreamResponse& streamResponse, bool isFinalResponse) {
    AppHostContext_->AddProtobufItem(streamResponse, NCuttlefish::ITEM_TYPE_MUSIC_MATCH_STREAM_RESPONSE);
    FlushAppHostContext(isFinalResponse);
}

void TCallbacksHandler::FlushAppHostContext(bool isFinalFlush) {
    if (isFinalFlush) {
        AppHostContext_->Flush();
        AppHostContext_.Reset();
    } else {
        AppHostContext_->IntermediateFlush();
    }
}
