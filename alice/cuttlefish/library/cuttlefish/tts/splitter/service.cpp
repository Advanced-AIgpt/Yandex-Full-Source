#include "service.h"
#include "utils.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/stream_servant_base/base.h>
#include <alice/cuttlefish/library/cuttlefish/tts/utils/utils.h>

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/aws/apphost_http_request_signer.h>
#include <alice/cuttlefish/library/proto_censor/tts.h>

#include <voicetech/library/settings_manager/proto/settings.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

// TODO(VOICESERV-4176) Remove this
// Now it's only for migration
static const TString S3_HOST_DOMAIN = ".s3.mds.yandex.net";

TString S3HostByBucket(const TString& bucket) {
    return TStringBuilder{} << bucket << S3_HOST_DOMAIN;
}

// Parse tts requests and generate requests for tts cache, s3 audio and tts backends
class TTtsSplitter : public TStreamServantBase {
public:
    TTtsSplitter(
        const NAliceCuttlefishConfig::TConfig& config,
        NAppHost::TServiceContextPtr ctx,
        TLogContext logContext
    )
        : TStreamServantBase(
            ctx,
            logContext,
            TTtsSplitter::SOURCE_NAME
        )
        , Config_(config)
        , FinalRequestProcessed_(false)
    {}

protected:
    bool ProcessFirstChunk() override {
        const auto requestContextItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_REQUEST_CONTEXT, NAppHost::EContextItemSelection::Input);
        if (!requestContextItemRefs.empty()) {
            try {
                ParseProtobufItem(*requestContextItemRefs.begin(), RequestContext_);
            } catch (...) {
                Metrics_.SetError("badrequestcontext");
                OnError(TStringBuilder() << "Failed to parse request context: " << CurrentExceptionMessage(), /* isCritical = */ true);
                return false;
            }
        } else {
            Metrics_.SetError("norequestcontext");
            OnError("Request context not found in first chunk", /* isCritical = */ true);
            return false;
        }

        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostRequestContext>(RequestContext_.ShortUtf8DebugString());

        const auto sessionContextItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_SESSION_CONTEXT, NAppHost::EContextItemSelection::Input);
        if (!sessionContextItemRefs.empty()) {
            try {
                ParseProtobufItem(*sessionContextItemRefs.begin(), SessionContext_);
            } catch (...) {
                Metrics_.SetError("badsessioncontext");
                OnError(TStringBuilder() << "Failed to parse session context: " << CurrentExceptionMessage(), /* isCritical = */ false);
            }
        } else {
            Metrics_.SetError("nosessioncontext");
        }

        return true;
    }

    bool ProcessInput() override {
        if (!ProcessFinalTtsRequests()) {
            return false;
        }

        ProcessPartialTtsRequests();

        return true;
    }

    bool IsCompleted() override {
        return FinalRequestProcessed_;
    }

    TString GetErrorForIncompleteInput() override {
        return "";
    }

    void OnIncompleteInput() override {
        // It's not an error because it can have happened on contractivation for example
        Metrics_.PushRate("cancel", "cancel", "final");
        LogContext_.LogEventInfoCombo<NEvClass::TInfoMessage>("Apphost stream finished without final request");

        OnCompleted();
    }

private:
    bool ProcessFinalTtsRequests() {
        if (FinalRequestProcessed_) {
            // Must be unreachable
            // Just sanity check
            return true;
        }

        const auto finalItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_TTS_REQUEST, NAppHost::EContextItemSelection::Input);
        if (!finalItemRefs.empty()) {
            if (finalItemRefs.size() >= 2u) {
                LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>(TStringBuilder() << "Got " << finalItemRefs.size() << " final messages");
            }

            NTts::TRequest ttsRequest;
            try {
                ParseProtobufItem(*finalItemRefs.begin(), ttsRequest);
            } catch (...) {
                Metrics_.PushRate("parse", "error", "final");
                Metrics_.SetError("badfinal");
                OnError(TStringBuilder() << "Failed to parse final tts request: " << CurrentExceptionMessage(), /* isCritical = */ true);
                return false;
            }

            try {
                OnFinalTtsRequest(ttsRequest);
                Metrics_.PushRate("handle", "ok", "final");
            } catch (...) {
                Metrics_.PushRate("handle", "error", "final");
                Metrics_.SetError("badfinal");
                OnError(TStringBuilder() << "OnFinalTtsRequeset failed: " << CurrentExceptionMessage(), /* isCritical = */ true);
                return false;
            }

            AhContext_->IntermediateFlush();
            FinalRequestProcessed_ = true;
        }

        return true;
    }

    void ProcessPartialTtsRequests() {
        // We already got final request
        // so there is no more sense to warm up the cache
        if (FinalRequestProcessed_) {
            return;
        }

        bool needFlush = false;
        const auto partialItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_TTS_PARTIAL_REQUEST, NAppHost::EContextItemSelection::Input);
        for (const auto& partialItemRef : partialItemRefs) {
            NTts::TRequest ttsRequest;
            try {
                ParseProtobufItem(partialItemRef, ttsRequest);
            } catch (const yexception& e) {
                Metrics_.PushRate("parse", "error", "partial");
                OnError(TStringBuilder() << "Failed to parse partial tts request: " << CurrentExceptionMessage(), /* isCritical = */ false);
                continue;
            }

            try {
                needFlush |= OnPartialTtsRequest(ttsRequest);
                Metrics_.PushRate("handle", "ok", "partial");
            } catch (...) {
                Metrics_.PushRate("handle", "error", "partial");
                OnError(TStringBuilder() << "OnPartialTtsRequeset failed: " << CurrentExceptionMessage(), /* isCritical = */ false);
            }
        }

        if (needFlush) {
            // Flush all partial requests
            AhContext_->IntermediateFlush();
        }
    }

    bool AllowWhisper() {
        if (RequestContext_.HasSettingsFromManager() &&
            RequestContext_.GetSettingsFromManager().HasTtsAllowWhisper() &&
            RequestContext_.GetSettingsFromManager().GetTtsAllowWhisper()
        ) {
            return true;
        }
        return false;
    }

    void OnFinalTtsRequest(
        const NTts::TRequest& ttsRequest
    ) {
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostTtsRequest>(GetCensoredTtsRequestStr(ttsRequest), true);

        if (ttsRequest.GetNeedTtsBackendTimings()) {
            Metrics_.PushRate("needtimings", "ok", "final");
        }

        if (ttsRequest.GetEnableTtsBackendTimings()) {
            Metrics_.PushRate("timings", "enabled", "final");
        } else {
            Metrics_.PushRate("timings", "disabled", "final");
        }

        if (ttsRequest.GetEnableGetFromCache()) {
            Metrics_.PushRate("cacheget", "enabled", "final");
        } else {
            Metrics_.PushRate("cacheget", "disabled", "final");
        }

        if (ttsRequest.GetEnableSaveToCache()) {
            if (!ttsRequest.GetDoNotLogTexts()) {
                Metrics_.PushRate("cachesave", "enabled", "final");
            } else {
                Metrics_.PushRate("cachesave", "disabledbycensor", "final");
            }
        } else {
            Metrics_.PushRate("cachesave", "disabled", "final");
        }

        // User want something strange
        if (ttsRequest.GetNeedTtsBackendTimings() && !ttsRequest.GetEnableTtsBackendTimings()) {
            Metrics_.PushRate("needbutnotenabledtimings", "error", "final");
            LogContext_.LogEventInfoCombo<NEvClass::TWarningMessage>("Request with NeedTtsBackendTimings but without EnableTtsBackendTimings");
        }

        // Can throw
        // We handle this exception in ProcessFinalTtsRequest
        const auto splitBySpeakersTagsResult = SplitTextBySpeakerTags(
            ttsRequest,
            RequestContext_.GetAudioOptions(),
            RequestContext_.GetVoiceOptions(),
            Config_.tts().cache_key_prefix(),
            AllowWhisper()
        );
        Metrics_.PushRate(splitBySpeakersTagsResult.AudioParts_.size(), "parts", "ok", "final");
        // Background audio metrics in other place (near to s3 http request add)

        TString targetFormat = NTtsUtils::FormatToMime(
            RequestContext_.GetAudioOptions().GetFormat(),
            RequestContext_.GetVoiceOptions().GetQuality()
        );

        NTts::TAggregatorRequest aggregatorRequest;
        aggregatorRequest.SetMime(targetFormat);
        aggregatorRequest.SetNeedTtsBackendTimings(ttsRequest.GetNeedTtsBackendTimings());
        if (ttsRequest.HasNeedRtsTimings()) {
            aggregatorRequest.SetNeedRtsTimings(ttsRequest.GetNeedRtsTimings());
        }
        if (ttsRequest.HasRtsBufferSeconds()) {
            aggregatorRequest.SetRtsBufferSeconds(ttsRequest.GetRtsBufferSeconds());
        }
        aggregatorRequest.SetEnableSaveToCache(
            ttsRequest.GetEnableSaveToCache() &&
            // We do not want to save something to cache when texts logging is disabled
            !ttsRequest.GetDoNotLogTexts()
        );
        aggregatorRequest.SetDoNotLogTexts(ttsRequest.GetDoNotLogTexts());

        // All request to tts backend must be sent via request sender
        // even it's just simple forward with "always send" send condition
        // We want to have exactly one place that do that
        NTts::TRequestSenderRequest requestSenderRequest;
        requestSenderRequest.SetDoNotLogTexts(ttsRequest.GetDoNotLogTexts());

        // Separate counters for s3 audio and tts backend requests to compress requestId
        // For example we have <tts_backend> <s3_audio> <tts_backend> <s3_audio> <tts_backend>
        // result will be tts_backend_request_0, s3_audio_http_request_0, tts_backend_request_1, s3_audio_http_request_1, tts_backend_request_2
        // Aggregattor will recive correct order in aggregator request
        ui32 s3AudioRequestId = 0;
        ui32 ttsBackendRequestId = 0;
        for (const auto& audioPartToGenerate : splitBySpeakersTagsResult.AudioParts_) {
            auto logRecord = GetCensoredTtsAudioPartToGenerateStr(audioPartToGenerate, ttsRequest.GetDoNotLogTexts());
            LogContext_.LogEventInfoCombo<NEvClass::TtsAudioPartToGenerate>(logRecord);
            if (audioPartToGenerate.HasAudio()) {
                AddS3AudioHttpRequestForAudioPart(
                    audioPartToGenerate,
                    aggregatorRequest,
                    s3AudioRequestId++,
                    targetFormat
                );
            }

            if (
                !audioPartToGenerate.HasAudio()
                // Legacy logic https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/tts.py?rev=r7947151#L534-536
                // Aggregator will handle this correctly
                || (audioPartToGenerate.HasAudio() && !audioPartToGenerate.GetText().empty())
            ) {
                AddTtsBackendRequest(
                    ttsRequest,
                    audioPartToGenerate,
                    aggregatorRequest,
                    requestSenderRequest,
                    ttsBackendRequestId++
                );
            }
        }

        if (splitBySpeakersTagsResult.BackgroundAudioPathForS3_.Defined()) {
            if (RequestContext_.GetSettingsFromManager().GetTtsBackgroundAudio()) {
                AddS3AudioHttpRequestForBackground(*splitBySpeakersTagsResult.BackgroundAudioPathForS3_, aggregatorRequest);
                Metrics_.PushRate("background", "ok", "final");
            } else {
                Metrics_.PushRate("background", "skipped", "final");
                LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>(TStringBuilder() << "Background audio generation skipped by request manager settings");
            }
        }

        auto reportLogAndMetrics = [this](
            ui32 requestId,
            ui32 limit,
            const TString& name
        ) {
            if (requestId > limit) {
                Metrics_.PushRate(requestId - limit, name, "skipped", "final");
                LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Too many " << name << " items: got " << requestId << ", limit " << limit);
            }

            Metrics_.PushRate(Min(requestId, limit), name, "ok", "final");
        };

        reportLogAndMetrics(
            s3AudioRequestId,
            Config_.tts().s3_config().max_audio_items(),
            "s3audio"
        );
        reportLogAndMetrics(
            ttsBackendRequestId,
            Config_.tts().max_tts_backend_items(),
            "ttsbackend"
        );

        AhContext_->AddProtobufItem(aggregatorRequest, ITEM_TYPE_TTS_AGGREGATOR_REQUEST);
        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostTtsAggregatorRequest>(GetCensoredTtsRequestStr(aggregatorRequest));

        if (!requestSenderRequest.GetAudioPartGenerateRequests().empty()) {
            AhContext_->AddProtobufItem(requestSenderRequest, ITEM_TYPE_TTS_REQUEST_SENDER_REQUEST);
            Metrics_.PushRate("requestsender", "ok", "final");
            LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostTtsRequestSenderRequest>(GetCensoredTtsRequestStr(requestSenderRequest));
        } else {
            Metrics_.PushRate("requestsender", "skipped", "final");
            LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Request sender request is empty");
        }
    }

    bool OnPartialTtsRequest(const NTts::TRequest& ttsRequest) {
        bool needFlush = false;
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostTtsRequest>(GetCensoredTtsRequestStr(ttsRequest), false);

        // Can throw
        // We handle this exception in ProcessPartialTtsRequest
        const auto splitBySpeakersTagsResult = SplitTextBySpeakerTags(
            ttsRequest,
            RequestContext_.GetAudioOptions(),
            RequestContext_.GetVoiceOptions(),
            Config_.tts().cache_key_prefix(),
            AllowWhisper()
        );
        Metrics_.PushRate(splitBySpeakersTagsResult.AudioParts_.size(), "parts", "ok", "partial");
        if (splitBySpeakersTagsResult.BackgroundAudioPathForS3_.Defined()) {
            Metrics_.PushRate("background", "ok", "partial");
        }

        // Even if cache warm up is disabled we want calculate metrics
        ui32 totalWarmUp = 0;
        ui32 skippedByWarmUpItemsLimit = 0;
        ui32 alreadyWarmedUp = 0;
        for (const auto& audioPartToGenerate : splitBySpeakersTagsResult.AudioParts_) {
            if (
                !audioPartToGenerate.HasAudio()
                // Legacy logic https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/tts.py?rev=r7947151#L534-536
                || (audioPartToGenerate.HasAudio() && !audioPartToGenerate.GetText().empty())
            ) {
                ++totalWarmUp;
                if (ttsRequest.GetEnableCacheWarmUp()) {
                    if (AlreadyWarmedUp_.size() >= Config_.tts().max_cache_warm_up_items()) {
                        if (!AlreadyWarmedUp_.contains(audioPartToGenerate.GetCacheKey())) {
                            LogContext_.LogEventInfoCombo<NEvClass::ErrorMessage>(
                                TStringBuilder()
                                    << "Warm up of audio part with cache key '"
                                    << audioPartToGenerate.GetCacheKey()
                                    << "' skipped due to exceeding warm up items limit: "
                                    << Config_.tts().max_cache_warm_up_items()
                            );
                            ++skippedByWarmUpItemsLimit;
                        } else {
                            LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(
                                TStringBuilder()
                                    << "Audio part with cache key '"
                                    << audioPartToGenerate.GetCacheKey()
                                    << "' already warmed up"
                            );
                            ++alreadyWarmedUp;
                        }
                    } else if (AlreadyWarmedUp_.insert(audioPartToGenerate.GetCacheKey()).second) {
                        NTts::TCacheWarmUpRequest cacheWarmUpRequest;
                        cacheWarmUpRequest.SetKey(audioPartToGenerate.GetCacheKey());

                        AhContext_->AddProtobufItem(cacheWarmUpRequest, ITEM_TYPE_TTS_CACHE_WARM_UP_REQUEST);
                        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostTtsCacheWarmUpRequest>(cacheWarmUpRequest.ShortUtf8DebugString());

                        needFlush = true;
                    } else {
                        LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(
                            TStringBuilder()
                                << "Audio part with cache key '"
                                << audioPartToGenerate.GetCacheKey()
                                << "' already warmed up"
                        );
                        ++alreadyWarmedUp;
                    }
                } else {
                    LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(
                        TStringBuilder()
                            << "Got audio part with cache key '"
                            << audioPartToGenerate.GetCacheKey()
                            << "' in partial request, but cache warm up is disabled"
                    );
                }
            }
        }

        if (ttsRequest.GetEnableCacheWarmUp()) {
            Metrics_.PushRate(totalWarmUp - alreadyWarmedUp - skippedByWarmUpItemsLimit, "cachewarmup", "ok", "partial");
            Metrics_.PushRate(skippedByWarmUpItemsLimit, "cachewarmup", "skipped", "partial");
            Metrics_.PushRate(alreadyWarmedUp, "alreadywarmedup", "ok", "partial");
        } else {
            Metrics_.PushRate(totalWarmUp, "cachewarmup", "skipped", "partial");
        }

        return needFlush;
    }

    NAppHostHttp::THttpRequest GetHttpRequestForS3(
        const TString& audioName,
        const TString& s3AudioBucket,
        const TString& backendForMetrics
    ) {
        NAppHostHttp::THttpRequest req;
        req.SetPath(TStringBuilder() << "/" << audioName);
        req.SetMethod(NAppHostHttp::THttpRequest::Get);

        {
            auto hostHeader = req.AddHeaders();
            hostHeader->SetName("host");

            if (!s3AudioBucket.empty()) {
                if (s3AudioBucket == Config_.tts().s3_config().default_bucket() ||
                    std::find(
                        Config_.tts().s3_config().allowed_buckets().begin(),
                        Config_.tts().s3_config().allowed_buckets().end(),
                        s3AudioBucket
                    ) != Config_.tts().s3_config().allowed_buckets().end()
                ) {

                    hostHeader->SetValue(S3HostByBucket(s3AudioBucket));
                } else {
                    ythrow yexception() << "S3 bucket '" << s3AudioBucket << "' is not allowed";
                }
            } else {
                hostHeader->SetValue(S3HostByBucket(Config_.tts().s3_config().default_bucket()));
            }

        }

        // historically, requests to default bucket don't need to be signed
        if (!s3AudioBucket.empty() && s3AudioBucket != Config_.tts().s3_config().default_bucket()) {
            try {
                NAws::GetDefaultS3RequestSignerInstance().SignRequest(
                    S3HostByBucket(s3AudioBucket),
                    req
                );

                Metrics_.PushRate("s3requestsign", "ok", backendForMetrics);
            } catch (...) {
                const auto currentExceptionMessage = CurrentExceptionMessage();

                Metrics_.PushRate("s3requestsign", "error", backendForMetrics);
                LogContext_.LogEventErrorCombo<NEvClass::S3RequestSingError>(
                    req,
                    currentExceptionMessage
                );

                ythrow yexception() << "Failed to sign s3 request " << currentExceptionMessage;
            }
        } else {
            Metrics_.PushRate("s3requestsign", "not_needed", backendForMetrics);
        }

        return req;
    }

    void AddS3AudioHttpRequestForBackground(
        const TString& backgroundAudioPathForS3,
        NTts::TAggregatorRequest& aggregatorRequest
    ) {
        // TODO(chegoryu): check audio format (?)

        // Send http request to s3
        {
            // it's impossible for override s3 bucket for background so use default_bucket for now
            auto req = GetHttpRequestForS3(
                backgroundAudioPathForS3,
                Config_.tts().s3_config().default_bucket(),
                /* backendForMetrics =  */ "final"
            );

            static const TString requestItemType = TStringBuilder() << ITEM_TYPE_PREFIX_S3_AUDIO_HTTP_REQUEST << "background";
            AhContext_->AddProtobufItem(req, requestItemType);
            LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostHttpRequestItem>(req.ShortUtf8DebugString(), requestItemType);
        }

        // Patch aggregator request
        {
            static const TString responseItemType = TStringBuilder() << ITEM_TYPE_PREFIX_S3_AUDIO_HTTP_RESPONSE << "background";
            aggregatorRequest.MutableBackgroundAudio()->MutableHttpResponse()->SetItemType(responseItemType);
        }
    }

    void AddS3AudioHttpRequestForAudioPart(
        const NTts::TAudioPartToGenerate& audioPartToGenerate,
        NTts::TAggregatorRequest& aggregatorRequest,
        ui32 requestId,
        const TString& targetFormat
    ) {
        const TString& audioName = audioPartToGenerate.GetAudio();

        if (audioName.EndsWith(".wav")) {
            static constexpr TStringBuf sourceFormat = "audio/x-pcm;bit=16;rate=48000";
            if (sourceFormat != targetFormat) {
                ythrow yexception() << "Format '" << targetFormat << "' is not supported";
            }
        } else if (audioName.EndsWith(".opus")) {
            static constexpr TStringBuf sourceFormat = "audio/opus";
            if (sourceFormat != targetFormat) {
                ythrow yexception() << "Format '" << targetFormat << "' is not supported";
            }
        } else {
            ythrow yexception() << "'" << audioName << "' is not supported audio file type";
        }

        // Check this only after validation
        if (requestId >= Config_.tts().s3_config().max_audio_items()) {
            // There's nothing we can do here :(
            // Error logging and metrics in other place
            return;
        }

        // Send http request to s3
        {
            auto req = GetHttpRequestForS3(
                audioName,
                audioPartToGenerate.GetS3AudioBucket(),
                /* backendForMetrics = */ "final"
            );

            const TString requestItemType = TStringBuilder() << ITEM_TYPE_PREFIX_S3_AUDIO_HTTP_REQUEST << ToString(requestId);
            AhContext_->AddProtobufItem(req, requestItemType);
            LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostHttpRequestItem>(req.ShortUtf8DebugString(), requestItemType);
        }

        // Patch aggregator request
        {
            auto& currentAggregatorAudioPart = *aggregatorRequest.AddAudioParts();
            // Add fake cache key to s3 audio part because we do not save s3 audio to tts cache
            // Probably will be changed in the future
            // https://st.yandex-team.ru/VOICESERV-3890#609e379d063e3a6d76b9519e
            currentAggregatorAudioPart.SetCacheKey("fake_for_s3_audio");

            const TString responseItemType = TStringBuilder() << ITEM_TYPE_PREFIX_S3_AUDIO_HTTP_RESPONSE << ToString(requestId);
            currentAggregatorAudioPart.AddAudioSources()->MutableHttpResponse()->SetItemType(responseItemType);
        }
    }

    void AddTtsBackendRequest(
        const NTts::TRequest& ttsRequest,
        const NTts::TAudioPartToGenerate& audioPartToGenerate,
        NTts::TAggregatorRequest& aggregatorRequest,
        NTts::TRequestSenderRequest& requestSenderRequest,
        ui32 requestId
    ) {
        TTtsBackendRequestInfo ttsBackendRequestInfo = CreateTtsBackendRequestInfo(
            audioPartToGenerate,
            RequestContext_.GetHeader().GetSessionId(),
            ttsRequest.GetRequestId(),
            ttsRequest.GetEnableTtsBackendTimings(),
            ttsRequest.GetDoNotLogTexts(),
            requestId,
            SessionContext_.GetSurface()
        );

        // Check this only after validation in CreateTtsBackendRequestInfo
        if (requestId >= Config_.tts().max_tts_backend_items()) {
            // There's nothing we can do here :(
            // Error logging and metrics in other place
            return;
        }

        // Patch request sender request
        {
            auto& currentAudioPartGenerateRequest = *requestSenderRequest.AddAudioPartGenerateRequests();

            currentAudioPartGenerateRequest.MutableRequest()->SetItemType(ttsBackendRequestInfo.ItemType_);
            currentAudioPartGenerateRequest.MutableRequest()->MutableBackendRequest()->CopyFrom(ttsBackendRequestInfo.Request_);

            if (ttsRequest.GetEnableGetFromCache()) {
                currentAudioPartGenerateRequest.MutableSendCondition()->MutableCacheMissOrError()->SetKey(audioPartToGenerate.GetCacheKey());
            } else {
                // Cache is disabled
                // Just forward message
                currentAudioPartGenerateRequest.MutableSendCondition()->MutableAlwaysSend();
            }
        }

        // Send cache get request if needed
        if (ttsRequest.GetEnableGetFromCache()) {
            if (AlreadyGottenFromCache_.insert(audioPartToGenerate.GetCacheKey()).second) {
                NTts::TCacheGetRequest cacheGetRequest;
                cacheGetRequest.SetKey(audioPartToGenerate.GetCacheKey());

                AhContext_->AddProtobufItem(cacheGetRequest, ITEM_TYPE_TTS_CACHE_GET_REQUEST);
                Metrics_.PushRate("cacheget", "ok", "final");
                LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostTtsCacheGetRequest>(cacheGetRequest.ShortUtf8DebugString());
            } else {
                LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Audio part with cache key '" << audioPartToGenerate.GetCacheKey() << "' already gotten");
                Metrics_.PushRate("samecacheget", "ok", "final");
            }
        }

        // Patch aggregator request
        {
            auto& currentAggregatorAudioPart = *aggregatorRequest.AddAudioParts();

            // Always add cache key to aggregator audio part
            // because saving to cache can be enabled even if getting from cache is disabled
            currentAggregatorAudioPart.SetCacheKey(audioPartToGenerate.GetCacheKey());

            // Always add audio response source
            currentAggregatorAudioPart.AddAudioSources()->MutableAudio()->SetReqSeqNo(ttsBackendRequestInfo.Request_.GetReqSeqNo());

            // Add tts cache source if getting from cache is enabled
            if (ttsRequest.GetEnableGetFromCache()) {
                currentAggregatorAudioPart.AddAudioSources()->MutableCacheGetResponse();
            }
        }
    }

private:
    static constexpr TStringBuf SOURCE_NAME = "tts_splitter";

    const NAliceCuttlefishConfig::TConfig& Config_;

    NAliceProtocol::TRequestContext RequestContext_;
    NAliceProtocol::TSessionContext SessionContext_;
    THashSet<TString> AlreadyWarmedUp_;
    THashSet<TString> AlreadyGottenFromCache_;

    bool FinalRequestProcessed_ = false;
};

} // namespace

NThreading::TPromise<void> TtsSplitter(const NAliceCuttlefishConfig::TConfig& config, NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    TIntrusivePtr<TTtsSplitter> stream(new TTtsSplitter(config, ctx, std::move(logContext)));
    stream->OnNextInput();
    return stream->GetFinishPromise();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
