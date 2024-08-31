#pragma once

#include "interface.h"

#include <util/generic/vector.h>

namespace NAlice::NYabio {
    class TFake : public TInterface {
    public:
        TFake(TIntrusivePtr<TCallbacks>& callbacks)
            : TInterface(callbacks)
        {
        }

        void CauseError(NProtobuf::EResponseCode, const TString&) override;
        void Close() override {
            Closed_ = true;
        }
        void SoftClose() override {
            Closed_ = true;
        }

        bool ProcessInitRequest(NProtobuf::TInitRequest&) override;
        bool ProcessAddData(NProtobuf::TAddData&) override;

    private:
        void ProcessCloseConnection();
        void FillAddDataResponse(const TVector<TString>& words, NProtobuf::TAddDataResponse&, size_t messages=1);

        bool Classify_ = false;
        TVector<TString> ClassificationTags_;
        NProtobuf::TContext Context_;
        bool ContextHasNewEnrolling_ = false;
        size_t RecvBytes_ = 0;
        bool Closed_ = false;
        bool SendedInitResponse_ = false;
        size_t DurationProcessedAudio_ = 0;
    };
}
