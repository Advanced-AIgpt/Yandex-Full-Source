#include "fake.h"

#include <alice/cuttlefish/library/logging/dlog.h>

#include <util/string/builder.h>

using namespace NAlice::NMusicMatch;

namespace {
    // TODO(chegoryu) Write here something normal (md5 hash ?)
    constexpr TStringBuf MUSIC_DATA = "abcdef";

} // namespace

TFake::~TFake() {
    if (FinishPromise_.Defined()) {
        FinishPromise_->SetValue();
    }
}

void TFake::ProcessContextLoadResponse(const NProtobuf::TContextLoadResponse& contextLoadResponse) {
    Y_UNUSED(contextLoadResponse);

    if (Closed_) {
        return;
    }

    HasContextLoadResponse_ = true;
}

void TFake::ProcessSessionContext(const NProtobuf::TSessionContext& sessionContext) {
    Y_UNUSED(sessionContext);

    if (Closed_) {
        return;
    }

    HasSessionContext_ = true;
}

void TFake::ProcessTvmServiceTicket(const TString& tvmServiceTicket) {
    Y_UNUSED(tvmServiceTicket);

    if (Closed_) {
        return;
    }

    HasTvmServiceTicket_ = true;
}

void TFake::ProcessInitRequest(const NProtobuf::TInitRequest& initRequest) {
    Y_UNUSED(initRequest);

    if (Closed_) {
        return;
    }

    if (HasContextLoadResponse_ && HasSessionContext_ && HasTvmServiceTicket_) {
        NProtobuf::TInitResponse initResponse;
        initResponse.SetIsOk(true);
        Callbacks_->OnInitResponse(initResponse);
        HasInitRequest_ = true;
    } else {
        NProtobuf::TInitResponse initResponse;
        initResponse.SetIsOk(false);

        TStringBuilder error;
        {
            if (!HasContextLoadResponse_) {
                error << "No context load response was provided before init request sent;";
            }
            if (!HasSessionContext_) {
                error << "No session context was provided before init request sent;";
            }
            if (!HasTvmServiceTicket_) {
                error << "No tvm service ticket was provided before init request sent;";
            }
        }
        initResponse.SetErrorMessage(error);
        Callbacks_->OnInitResponse(initResponse);

        // Do nothing after incorrect init request
        Closed_ = true;
    }
}

void TFake::ProcessStreamRequest(const NProtobuf::TStreamRequest& streamRequest) {
    if (Closed_) {
        return;
    }

    if (streamRequest.HasAddData()) {
        ProcessAddData(streamRequest.GetAddData());
    } else {
        DLOG("Unexpected TStreamRequest type");
    }
}

void TFake::CauseError(const TString& error) {
    if (Closed_) {
        return;
    }

    if (HasInitRequest_) {
        NProtobuf::TStreamResponse streamResponse;
        auto musicResult = streamResponse.MutableMusicResult();
        musicResult->SetIsOk(false);
        musicResult->SetErrorMessage(error);
        Callbacks_->OnStreamResponse(streamResponse);
    } else {
        NProtobuf::TInitResponse initResponse;
        initResponse.SetIsOk(false);
        initResponse.SetErrorMessage(error);
        Callbacks_->OnInitResponse(initResponse);
    }

    Callbacks_->OnClosed();
    Closed_ = true;
}

void TFake::Close() {
    Callbacks_->OnClosed();
    Closed_ = true;
}

void TFake::SetFinishPromise(NThreading::TPromise<void>& promise) {
    FinishPromise_ = promise;
}

void TFake::ProcessAddData(const NProtobuf::TAddData& addData) {
    if (Closed_) {
        return;
    }

    if (addData.HasAudioData()) {
        RecvData_ += addData.GetAudioData();
    }

    if (RecvData_ == MUSIC_DATA) {
        NProtobuf::TStreamResponse streamResponse;
        auto musicResult = streamResponse.MutableMusicResult();
        musicResult->SetIsOk(true);
        musicResult->SetRawMusicResultJson("{\"result\":\"music\"}");
        musicResult->SetIsFinish(true);
        Callbacks_->OnStreamResponse(streamResponse);
    } else if (RecvData_.size() >= MUSIC_DATA.size()) {
        NProtobuf::TStreamResponse streamResponse;
        auto musicResult = streamResponse.MutableMusicResult();
        musicResult->SetIsOk(true);
        musicResult->SetRawMusicResultJson("{\"result\":\"not-music\"}");
        musicResult->SetIsFinish(true);
        Callbacks_->OnStreamResponse(streamResponse);
    } else {
        NProtobuf::TStreamResponse streamResponse;
        auto musicResult = streamResponse.MutableMusicResult();
        musicResult->SetIsOk(true);
        musicResult->SetRawMusicResultJson("{\"result\":\"Can't recognize yet\"}");
        Callbacks_->OnStreamResponse(streamResponse);
    }
}
