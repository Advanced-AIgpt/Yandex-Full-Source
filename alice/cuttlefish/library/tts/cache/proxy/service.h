#pragma once

#include "cachalot_client.h"
#include "tts_cache_callbacks_with_eventlog.h"

#include <alice/cuttlefish/library/logging/apphost_log.h>
#include <alice/cuttlefish/library/tts/cache/base/service.h>
#include <alice/cuttlefish/library/proto_configs/tts_cache_proxy.cfgproto.pb.h>

#include <voicetech/library/ws_server/http_client.h>

#include <apphost/api/service/cpp/service.h>

#include <library/cpp/neh/asio/executor.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/string/builder.h>

namespace NAlice::NTtsCacheProxy {
    class TService : public NTtsCache::TService {
    public:
        typedef NAliceTtsCacheProxyConfig::Config TConfig;
        static const TString DefaultConfigResource;

        explicit TService(const TConfig& config);

        const NAliceTtsCacheProxyConfig::TConfig& Config() noexcept {
            return Config_;
        }

        // Expand base tts_cache request processor with log & tts_cache's impl
        class TRequestProcessor: public NTtsCache::TService::TRequestProcessor {
        public:
            explicit TRequestProcessor(TService& service, NAlice::NCuttlefish::TLogContext&& logContext)
                : NTtsCache::TService::TRequestProcessor(service)
                , Service_(service)
                , LogContext_(std::move(logContext))
                , Number_(NextProcNumber_.Inc())
            {
                LogContext_.LogEventInfoCombo<NEvClass::AppHostProcessor>(Number_);
            }

            void OnCacheSetRequest(const NTtsCache::NProtobuf::TCacheSetRequest& cacheSetRequest) override {
                {
                    NTtsCache::NProtobuf::TCacheSetRequest cacheSetRequestCopy = cacheSetRequest;
                    // Do not log audio
                    cacheSetRequestCopy.MutableCacheEntry()->ClearAudio();
                    LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostTtsCacheSetRequest>(
                        cacheSetRequestCopy.ShortUtf8DebugString(),
                        cacheSetRequest.GetCacheEntry().GetAudio().size()
                    );
                }
                NTtsCache::TService::TRequestProcessor::OnCacheSetRequest(cacheSetRequest);
            }

            void OnCacheGetRequest(const NTtsCache::NProtobuf::TCacheGetRequest& cacheGetRequest) override {
                LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostTtsCacheGetRequest>(cacheGetRequest.ShortUtf8DebugString());
                NTtsCache::TService::TRequestProcessor::OnCacheGetRequest(cacheGetRequest);
            }

            void OnCacheWarmUpRequest(const NTtsCache::NProtobuf::TCacheWarmUpRequest& cacheWarmUpRequest) override {
                LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostTtsCacheWarmUpRequest>(cacheWarmUpRequest.ShortUtf8DebugString());
                NTtsCache::TService::TRequestProcessor::OnCacheWarmUpRequest(cacheWarmUpRequest);
            }

            void InitializeTtsCache(TIntrusivePtr<NTtsCache::TInterface::TCallbacks> callbacks) override;
            TIntrusivePtr<NTtsCache::TInterface::TCallbacks> CreateTtsCacheCallbacks() override;

            void OnAppHostEmptyInput() override {
                LogContext_.LogEventInfoCombo<NEvClass::AppHostEmptyInput>();
                NTtsCache::TService::TRequestProcessor::OnAppHostEmptyInput();
            }
            void OnAppHostClose() override {
                LogContext_.LogEventInfoCombo<NEvClass::ProcessCloseProcessor>();
                NTtsCache::TService::TRequestProcessor::OnAppHostClose();
            }
            void OnUnknownItemType(const TString& tag, const TString& type) override {
                LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>(TStringBuilder() << TStringBuf("unknown item tag=") << tag << TStringBuf(", type=") << type);
                NTtsCache::TService::TRequestProcessor::OnUnknownItemType(tag, type);
            }
            void OnWarning(const TString& warning) override {
                LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>(warning);
                NTtsCache::TService::TRequestProcessor::OnWarning(warning);
            }
            void OnError(const TString& error) override {
                LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(error);
                NTtsCache::TService::TRequestProcessor::OnError(error);
            }

        private:
            TService& Service_;
            NCuttlefish::TLogContext LogContext_;
            TAtomicBase Number_ = 0;
            static TAtomicCounter NextProcNumber_;
        };

    public:
        TIntrusivePtr<NTtsCache::TService::TRequestProcessor> CreateProcessor(NAppHost::IServiceContext& ctx, NAlice::NCuttlefish::TRtLogClient* rtLogClient) override {
            return new TRequestProcessor(*this, NAlice::NCuttlefish::LogContextFor(ctx, rtLogClient));
        }

        const TCachalotClient::TConfig& GetCachalotMainClientConfig() const {
            return CachalotMainClientConfig_;
        }

        NVoicetech::THttpClient& GetHttpClient() {
            return *HttpClients_[ClientRequestNum_.Inc() % HttpClients_.size()];
        }

    private:
        NAliceTtsCacheProxyConfig::TConfig Config_;
        TCachalotClient::TConfig CachalotMainClientConfig_;
        NAsio::TExecutorsPool ExecutorsPool_;
        // Use for suspend asio executors (wait destroy all cachalot clients)
        NVoicetech::TAsioClients ClientsCount_;
        TAtomicCounter ClientRequestNum_;
        TVector<THolder<NVoicetech::THttpClient>> HttpClients_;

    };
}
