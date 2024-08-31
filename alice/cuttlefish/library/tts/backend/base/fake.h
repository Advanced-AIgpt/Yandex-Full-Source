#pragma once

#include "interface.h"

#include <library/cpp/neh/asio/asio.h>
#include <library/cpp/threading/atomic/bool.h>

#include <util/generic/hash_set.h>

namespace NAlice::NTts {
    class TFake : public TInterface {
    public:
        struct TFakeChunksConfiguration {
            // We need this constructor
            // Otherwise we get "default member initializer for 'RepeatContent_' needed within definition of enclosing class 'TFake' outside of member functions"
            TFakeChunksConfiguration()
            {}

            // If false, stop sending even if not reach FakeChunksNum_
            bool RepeatContent_ = true;
            TString Content_ = "fake_audio";
            size_t ChunksNum_ = 10;
            size_t ChunkSize_ = 10;
            TDuration ChunkDuration_ = TDuration::MilliSeconds(100);
        };

    public:
        explicit TFake(
            TIntrusivePtr<TCallbacks>& callbacks,
            const TFakeChunksConfiguration& fakeChunksConfiguration = {}
        );

        void ProcessBackendRequest(const NProtobuf::TBackendRequest& backendRequest) override;
        void Cancel() override;

    private:
        class TTtsClient {
        public:
            TTtsClient(
                TIntrusivePtr<TCallbacks>& callbacks,
                const TFakeChunksConfiguration& fakeChunksConfiguration
            );

            void AddBackendRequest(const NProtobuf::TBackendRequest& backendRequest);
            void CancelAll();

        private:
            class TTtsHandler : public TThrRefBase {
            public:
                TTtsHandler(
                    TIntrusivePtr<TCallbacks>& callbacks,
                    const NProtobuf::TBackendRequest& backendRequest,
                    const TFakeChunksConfiguration& fakeChunksConfiguration,
                    NAsio::TIOService& ioService
                );

                void Start();
                void Cancel();

            private:
                bool SendNextChunkUnsafe();
                void SendErrorUnsafe(const TString& error);
                void ScheduleTimerUnsafe();
                void OnTimerCallback(const NAsio::TErrorCode& err);

            private:
                TIntrusivePtr<TCallbacks> Callbacks_;
                NProtobuf::TBackendRequest BackendRequest_;

                NAtomic::TBool Closed_ = false;

                TFakeChunksConfiguration FakeChunksConfiguration_;
                size_t ContentConsumed_ = 0;

                // Chunked mode options
                THolder<NAsio::TDeadlineTimer> Timer_;
                NAsio::TIOService& IOService_;

                bool CompletedSended_ = false;
                TString Error_;
            };

        private:
            TIntrusivePtr<TCallbacks> Callbacks_;
            const TFakeChunksConfiguration FakeChunksConfiguration_;
            NAsio::TIOService& IOService_;

            THashSet<ui32> AddedReqSeqNo_;
            TVector<TIntrusivePtr<TTtsHandler>> TtsHandlers_;
        };

    private:
        TTtsClient TtsClient_;
    };
}
