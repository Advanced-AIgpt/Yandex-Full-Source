#pragma once

#include <alice/cuttlefish/library/music_match/base/callbacks_handler.h>

#include <alice/cuttlefish/library/logging/event_log.h>

namespace NAlice::NMusicMatchAdapter {
    // add logging to callbacks usage
    class TMusicMatchCallbacksWithEventlog: public NMusicMatch::TCallbacksHandler {
    public:
        TMusicMatchCallbacksWithEventlog(NAppHost::TServiceContextPtr& ahContext, TAtomicBase requestNumber);

        void OnAnyError(const TString& error) override;

        void AddInitResponseAndFlush(const NMusicMatch::NProtobuf::TInitResponse& initResponse, bool isFinalResponse) override;
        void AddStreamResponseAndFlush(const NMusicMatch::NProtobuf::TStreamResponse& streamResponse, bool isFinalResponse) override;

        const NCuttlefish::TLogFramePtr& LogFrame() noexcept {
            return LogFrame_;
        }

    protected:
        void FlushAppHostContext(bool isFinalFlush) override;

    private:
        NCuttlefish::TLogFramePtr LogFrame_;
        const TString RequestId_;
        const TAtomicBase RequestNumber_;
    };

}
