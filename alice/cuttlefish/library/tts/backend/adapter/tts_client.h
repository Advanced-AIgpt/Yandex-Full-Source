#pragma once

#include <alice/cuttlefish/library/tts/backend/base/interface.h>

#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <voicetech/library/protobuf_handler/protobuf_handler.h>
#include <voicetech/library/ws_server/http_client.h>


namespace NAlice::NTtsAdapter {
    class TTtsClient {
    public:
        struct TConfig {
            TString TtsUrl_;
            ui32 ParallelRequestExecutionLimit_;
        };

    public:
        TTtsClient(
            const TConfig& config,
            TIntrusivePtr<NTts::TInterface::TCallbacks> callbacks,
            NVoicetech::THttpClient& httpClient,
            NRTLog::TRequestLoggerPtr rtLogger
        );
        ~TTtsClient();

        void AddBackendRequest(const NTts::NProtobuf::TBackendRequest& backendRequest);
        void CancelAll();

    private:
        class TImpl : public TThrRefBase {
        private:
            class TTtsHandler;
            friend TTtsHandler;

        public:
            TImpl(
                const TConfig& config,
                TIntrusivePtr<NTts::TInterface::TCallbacks> callbacks,
                NVoicetech::THttpClient& httpClient,
                NRTLog::TRequestLoggerPtr rtLogger
            );
            ~TImpl();

            void AddBackendRequest(const NTts::NProtobuf::TBackendRequest& backendRequest);
            void CancelAll();

        private:
            // WARNING: all this methods must be executed only in IOService
            void AddBackendRequestImpl(const NTts::NProtobuf::TBackendRequest& backendRequest);
            void CancelAllImpl();

            void TryStartNotStartedRequestsProcessing();
            void TryStartRequest(ui32 requestIndex);

            void RegisterRequest(NVoicetech::TProtobufHandler* handler, ui32 reqSeqNo);
            void OnRequestCompleted(ui32 reqSeqNo, bool isSuccess);

        private:
            struct TStartedRequestData {
                NVoicetech::TProtobufHandler* Handler_;
                ui32 ReqSeqNo_;
            };

        private:
            const TConfig& Config_;
            TIntrusivePtr<NTts::TInterface::TCallbacks> Callbacks_;
            NVoicetech::THttpClient& HttpClient_;
            NRTLog::TRequestLoggerPtr RtLogger_;

            bool Canceled_ = false;
            TVector<NTts::NProtobuf::TBackendRequest> NotStartedRequests_;
            TVector<TStartedRequestData> StartedRequests_;
            // There are 3 requests in q99.99
            // So TSet is more optimal than THashSet here
            TSet<ui32> AddedReqSeqNo_;
        };

    private:
        TIntrusivePtr<TImpl> Impl_;
    };
}
