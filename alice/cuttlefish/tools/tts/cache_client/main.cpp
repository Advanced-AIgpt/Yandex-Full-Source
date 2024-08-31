#include <alice/cuttlefish/library/tts/cache/client/tts_cache_client.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/variant.h>
#include <util/system/env.h>
#include <util/system/event.h>
#include <util/system/hostname.h>

namespace NDebug {
    static bool Enable = false;
}

namespace {

void DebugOut(const TString& s) {
    if (NDebug::Enable) {
        Cout << TInstant::Now() << " " << s << Endl;
    }
}

} // namespace

using namespace NAlice;
using namespace NAlice::NTtsCache;

using TTtsCacheRequestProto = std::variant<
    NProtobuf::TCacheSetRequest,
    NProtobuf::TCacheGetRequest,
    NProtobuf::TCacheWarmUpRequest
>;

class TTtsCacheRequest: public TTtsCacheClient::TRequest {
public:
    TTtsCacheRequest(
        TTtsCacheClient& ttsCacheClient,
        NAsio::TIOService& ioService,
        TManualEvent& event
    )
        : TTtsCacheClient::TRequest(ttsCacheClient, ioService)
        , Event_(event)
    {
    }
    ~TTtsCacheRequest() override {
        DebugOut("~TTtsCacheRequest");
        Event_.Signal();
    }

    void SendRequests(
        TVector<TTtsCacheRequestProto>&& ttsCacheRequests,
        TDuration durationBetweenRequests
    ) {
        TtsCacheRequestPtr_ = 0;
        TtsCacheRequests_ = ttsCacheRequests;
        DurationBetweenRequests_ = durationBetweenRequests;

        SendNextRequest();
        NextAsyncRead();
    }

private:
    void OnCacheGetResponse(const NProtobuf::TCacheGetResponse& cacheGetResponse) override {
        {
            NProtobuf::TCacheGetResponse cacheGetResponseCopy = cacheGetResponse;
            cacheGetResponseCopy.MutableCacheEntry()->SetAudio(TStringBuilder() << "Audio size = " << cacheGetResponse.GetCacheEntry().GetAudio().size());
            DebugOut(TStringBuilder() << "Got CacheGetResponse: " << cacheGetResponseCopy.ShortUtf8DebugString());
        }
    }

    bool NextAsyncRead() override {
        if (TTtsCacheClient::TRequest::NextAsyncRead()) {
            DebugOut("NextAsyncRead");
            return true;
        }
        return false;
    }
    void OnEndNextResponseProcessing() override {
        TTtsCacheClient::TRequest::OnEndNextResponseProcessing();
    }

    void OnTimeout() override {
        StopSendingNextRequest();
        TTtsCacheClient::TRequest::OnTimeout();
    }

    void OnError(const TString& s) override {
        TTtsCacheClient::TRequest::OnError(s);
    }

    void SendNextRequest() {
        DebugOut("SendNextRequest");
        if (TtsCacheRequestPtr_ == TtsCacheRequests_.size()) {
            if (TtsCacheRequests_.empty()) {
                DebugOut("Cancel (no requests)");
                Cancel();
            } else {
                DebugOut("SendEndOfStream");
                SendEndOfStream();
            }
        } else {
            auto&& currentRequest = TtsCacheRequests_[TtsCacheRequestPtr_++];

            if (std::holds_alternative<NProtobuf::TCacheSetRequest>(currentRequest)) {
                DebugOut("SendCacheSetRequest");
                SendCacheSetRequest(std::get<NProtobuf::TCacheSetRequest>(std::move(currentRequest)));
            } else if (std::holds_alternative<NProtobuf::TCacheGetRequest>(currentRequest)) {
                DebugOut("SendCacheGetRequest");
                SendCacheGetRequest(std::get<NProtobuf::TCacheGetRequest>(std::move(currentRequest)));
            } else if (std::holds_alternative<NProtobuf::TCacheWarmUpRequest>(currentRequest)) {
                DebugOut("SendCacheWarmUpRequest");
                SendCacheWarmUpRequest(std::get<NProtobuf::TCacheWarmUpRequest>(std::move(currentRequest)));
            } else {
                DebugOut(TStringBuilder() << "Skip unknown request: " << currentRequest.index());
            }

            SetNextRequestTimer();
        }
    }

    void SetNextRequestTimer() {
        TAtomicSharedPtr<NAsio::TDeadlineTimer> timer(new NAsio::TDeadlineTimer(IOService_));
        TIntrusivePtr<TTtsCacheRequest> self(this);
        timer->AsyncWaitExpireAt(DurationBetweenRequests_, [timer, self](const NAsio::TErrorCode& err, NAsio::IHandlingContext&){
            if (!err.Value()) {
                DebugOut("OnNextRequest timer");
                self->SendNextRequest();
            } else {
                DebugOut(TStringBuilder() << "OnNextRequest timer error: " << err.Text());
            }
        });
        NextRequestTimer_ = timer;
    }

    void StopSendingNextRequest() {
        DebugOut("StopSendingNextRequest");
        if (NextRequestTimer_) {
            NextRequestTimer_->Cancel();
            NextRequestTimer_.Reset();
        }
    }

private:
    TManualEvent& Event_;

    size_t TtsCacheRequestPtr_ = 0;
    TVector<TTtsCacheRequestProto> TtsCacheRequests_;
    TDuration DurationBetweenRequests_ = TDuration::Zero();
    TAtomicSharedPtr<NAsio::TDeadlineTimer> NextRequestTimer_;
};

int main(int argc, char *argv[]) {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    opts.AddLongOption("debug").StoreResult(&NDebug::Enable).DefaultValue(true);

    TTtsCacheClient::TConfig config;
    {
        opts.AddLongOption("host").StoreResult(&config.Host).DefaultValue("localhost");
        opts.AddLongOption("path").StoreResult(&config.Path).DefaultValue("/tts_cache");
        opts.AddLongOption("port").StoreResult(&config.Port).DefaultValue(12001);
        config.Timeout = TDuration::Seconds(100);
    }

    TVector<TTtsCacheRequestProto> ttsCacheRequests;

    opts
        .AddLongOption("add-set-request", "(repeatable) add set request with (key, value)")
        .Handler1T<TString>([&ttsCacheRequests](const TString& data) {
            TVector<TString> parts = SplitString(data, " ");
            Y_ENSURE(parts.size() == 2u, "expected (key, value) but found '" << data << "'");

            NProtobuf::TCacheSetRequest cacheSetRequest;

            cacheSetRequest.SetKey(parts[0]);
            cacheSetRequest.MutableCacheEntry()->SetAudio(parts[1]);

            ttsCacheRequests.push_back(std::move(cacheSetRequest));
        })
    ;
    opts
        .AddLongOption("add-get-request", "(repeatable) add get request with key")
        .Handler1T<TString>([&ttsCacheRequests](const TString& key) {
            NProtobuf::TCacheGetRequest cacheGetRequest;
            cacheGetRequest.SetKey(key);
            ttsCacheRequests.push_back(std::move(cacheGetRequest));
        })
    ;
    opts
        .AddLongOption("add-warm-up-request", "(repeatable) add warm up request with key")
        .Handler1T<TString>([&ttsCacheRequests](const TString& key) {
            NProtobuf::TCacheWarmUpRequest cacheWarmUpRequest;
            cacheWarmUpRequest.SetKey(key);
            ttsCacheRequests.push_back(std::move(cacheWarmUpRequest));
        })
    ;

    TDuration durationBetweenRequests;
    opts.AddLongOption("duration-between-requests").StoreResult(&durationBetweenRequests).DefaultValue("200ms");

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    NAsio::TExecutorsPool pool(1);
    try {
        TManualEvent eventFinish;
        TTtsCacheClient ttsCacheClient(config);
        {
            TTtsCacheRequest::TPtr ttsCacheRequest(new TTtsCacheRequest(ttsCacheClient, pool.GetExecutor().GetIOService(), eventFinish));
            dynamic_cast<TTtsCacheRequest*>(ttsCacheRequest.Get())->SendRequests(
                std::move(ttsCacheRequests),
                durationBetweenRequests
            );
        }
        eventFinish.WaitI();
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
