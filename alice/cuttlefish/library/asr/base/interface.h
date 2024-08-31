#pragma once

#include "protobuf.h"

#include <alice/cuttlefish/library/protos/uniproxy2.pb.h>
#include <util/generic/ptr.h>
#include <util/generic/maybe.h>

namespace NAlice::NAsr {
    class TInterface : public TThrRefBase {
    public:
        class TCallbacks : public TThrRefBase {
        public:
            virtual void OnInitResponse(const NProtobuf::TResponse&) = 0;
            virtual void OnSpotterValidation(bool valid) = 0;
            virtual void OnAddDataResponse(const NProtobuf::TResponse&) = 0;
            virtual void OnSessionLog(const NAliceProtocol::TSessionLogRecord&) {}
            virtual void OnClosed() = 0;
            virtual void OnAnyError(const TString& error, bool fastError=false) = 0;
            virtual void Finish() = 0;
        };

        TInterface(TIntrusivePtr<TCallbacks>& callbacks)
            : Callbacks_(callbacks)
        {}
        ~TInterface() {
            if (NeedFinish_) {
                Callbacks_->Finish();
            }
        }
        virtual bool ProcessAsrRequest(const NProtobuf::TRequest&) = 0;
        // sometimes we need use impl. thread for call TCallbacks::OnAnyError, so use this method for this
        virtual void CauseError(const TString& error) = 0;
        virtual void Close() = 0;

        virtual void SetNeedFinish() {
            NeedFinish_ = true;
        }

    protected:
        TIntrusivePtr<TCallbacks> Callbacks_;
        bool NeedFinish_ = false;  // continue executing request after receive EOS from apphost ()
    };
}
