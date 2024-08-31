#pragma once

#include "interface.h"

namespace NAlice::NAsr {
    class TFake : public TInterface {
    public:
        TFake(TIntrusivePtr<TCallbacks>& callbacks)
            : TInterface(callbacks)
        {
        }

        bool ProcessAsrRequest(const NProtobuf::TRequest&) override;
        void CauseError(const TString&) override;
        void Close() override {
            Closed_ = true;
        }

    private:
        bool ProcessInitRequest(const NProtobuf::TInitRequest&);
        bool ProcessAddData(const NProtobuf::TAddData&);
        void ProcessEndOfStream();
        void ProcessCloseConnection();
        void FillAddDataResponse(const TVector<TString>& words, NProtobuf::TAddDataResponse&, size_t messages=1);

        bool SpotterValidation_ = false;
        size_t RecvBytes_ = 0;
        bool Closed_ = false;
        bool SendedInitResponse_ = false;
        size_t DurationProcessedAudio_ = 0;
    };
}
