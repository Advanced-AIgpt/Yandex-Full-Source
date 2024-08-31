#pragma once

#include "interface.h"

namespace NAlice::NMusicMatch {
    class TFake : public TInterface {
    public:
        TFake(TIntrusivePtr<TCallbacks>& callbacks)
            : TInterface(callbacks)
        {
        }
        ~TFake();

        void ProcessContextLoadResponse(const NProtobuf::TContextLoadResponse& contextLoadResponse) override;
        void ProcessSessionContext(const NProtobuf::TSessionContext& sessionContext) override;
        void ProcessTvmServiceTicket(const TString& tvmServiceTicket) override;
        void ProcessInitRequest(const NProtobuf::TInitRequest& initRequest) override;
        void ProcessStreamRequest(const NProtobuf::TStreamRequest& streamRequest) override;

        void CauseError(const TString& error) override;
        void Close() override;

        void SetFinishPromise(NThreading::TPromise<void>& promise) override;

    private:
        void ProcessAddData(const NProtobuf::TAddData& addData);

        TString RecvData_ = "";

        bool Closed_ = false;

        bool HasSessionContext_ = false;
        bool HasContextLoadResponse_ = false;
        bool HasTvmServiceTicket_ = false;
        bool HasInitRequest_ = false;

        TMaybe<NThreading::TPromise<void>> FinishPromise_ = Nothing();
    };
}
