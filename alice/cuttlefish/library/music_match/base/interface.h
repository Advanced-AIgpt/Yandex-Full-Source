#pragma once

#include "protobuf.h"

#include <library/cpp/threading/future/future.h>

#include <util/generic/ptr.h>
#include <util/generic/maybe.h>

namespace NAlice::NMusicMatch {
    class TInterface : public TThrRefBase {
    public:
        class TCallbacks : public TThrRefBase {
        public:
            virtual void OnInitResponse(const NProtobuf::TInitResponse& initResponse) = 0;
            virtual void OnStreamResponse(const NProtobuf::TStreamResponse& streamResponse) = 0;
            virtual void OnClosed() = 0;
            virtual void OnAnyError(const TString& error) = 0;
        };

        TInterface(TIntrusivePtr<TCallbacks>& callbacks)
            : Callbacks_(callbacks)
        {}

        // Be careful
        // You can use this if and only if you know that you can synchronously send response to apphost context (no asio thead is started now)
        TIntrusivePtr<TCallbacks> GetCallbacks() const {
            return Callbacks_;
        }

        virtual void ProcessContextLoadResponse(const NProtobuf::TContextLoadResponse& contextLoadResponse) = 0;
        virtual void ProcessSessionContext(const NProtobuf::TSessionContext& sessionContext) = 0;
        virtual void ProcessTvmServiceTicket(const TString& tvmServiceTicket) = 0;
        virtual void ProcessInitRequest(const NProtobuf::TInitRequest& initRequest) = 0;
        virtual void ProcessStreamRequest(const NProtobuf::TStreamRequest& streamRequest) = 0;

        virtual void CauseError(const TString& error) = 0;
        virtual void Close() = 0;

        virtual void SetFinishPromise(NThreading::TPromise<void>& promise) = 0;

    protected:
        TIntrusivePtr<TCallbacks> Callbacks_;
    };
}
