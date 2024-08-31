#pragma once

#include "protobuf.h"
#include <util/generic/ptr.h>

namespace NAlice::NYabio {
    class TInterface : public TThrRefBase {
    public:
        class TCallbacks : public TThrRefBase {
        public:
            virtual void OnInitResponse(const NProtobuf::TResponse&) = 0;
            virtual void OnAddDataResponse(const NProtobuf::TResponse&) = 0;
            virtual void OnClosed() = 0;
            virtual void OnAnyError(NProtobuf::EResponseCode, const TString& error, bool fastError=false) = 0;
            virtual void Finish() {}
        };

        TInterface(TIntrusivePtr<TCallbacks>& callbacks)
            : Callbacks_(callbacks)
        {}
        virtual bool ProcessInitRequest(NProtobuf::TInitRequest&) = 0;
        virtual bool ProcessAddData(NProtobuf::TAddData&) = 0;
        // sometimes we need use impl. thread for call TCallbacks::OnAnyError, so use this method for this
        virtual void CauseError(NProtobuf::EResponseCode, const TString& error) = 0;
        virtual void Close() = 0;
        virtual void SoftClose() = 0;

    protected:
        TIntrusivePtr<TCallbacks> Callbacks_;
    };
}
