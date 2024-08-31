#pragma once

#include "interface.h"

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NMusicMatch {
    class TCallbacksHandler: public TInterface::TCallbacks {
    public:
        TCallbacksHandler(const NAppHost::TServiceContextPtr& ahContext)
            : AppHostContext_(ahContext)
        {}

        void OnInitResponse(const NProtobuf::TInitResponse& initResponse) override;
        void OnStreamResponse(const NProtobuf::TStreamResponse& streamResponse) override;
        void OnClosed() override;
        void OnAnyError(const TString& error) override;

        virtual void AddInitResponseAndFlush(const NProtobuf::TInitResponse& initResponse, bool isFinalResponse);
        virtual void AddStreamResponseAndFlush(const NProtobuf::TStreamResponse& streamResponse, bool isFinalResponse);

    protected:
        virtual void FlushAppHostContext(bool isFinalFlush);

    protected:
        NAppHost::TServiceContextPtr AppHostContext_;
        bool HasInitResponse_ = false;
    };
}
