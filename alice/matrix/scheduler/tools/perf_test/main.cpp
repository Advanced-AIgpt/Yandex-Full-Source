#include <alice/protos/api/matrix/action.pb.h>
#include <alice/protos/api/matrix/scheduled_action.pb.h>
#include <alice/protos/api/matrix/scheduler_api.pb.h>

#include <apphost/api/client/client.h>
#include <apphost/api/client/fixed_grpc_backend.h>
#include <apphost/api/client/stream_timeout.h>
#include <apphost/api/client/yp_grpc_backend.h>

#include <apphost/lib/grpc/client/grpc_client.h>
#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/protobuf/interop/cast.h>
#include <library/cpp/threading/future/async.h>

#include <google/protobuf/duration.pb.h>
#include <google/protobuf/timestamp.pb.h>


int main(int argc, char** argv) {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    // Scheduler location
    TString target;
    opts.AddLongOption("target").StoreResult(&target).DefaultValue("localhost:80");
    ui32 parallelRequestsCount;
    opts.AddLongOption("parallel-requests-count").StoreResult(&parallelRequestsCount).DefaultValue(100);
    bool verboseErrors;
    opts.AddLongOption("verbose-errors").StoreResult(&verboseErrors).DefaultValue(false);

    // Perf test params
    ui32 scheduledActionsCount;
    opts.AddLongOption("scheduled-actions-count").StoreResult(&scheduledActionsCount).DefaultValue(5);
    TString scheduledActionsIdPrefix;
    opts.AddLongOption("scheduled-actions-id-prefix").StoreResult(&scheduledActionsIdPrefix).DefaultValue("perf-test-");
    TDuration timeToStartAt;
    opts.AddLongOption("time-to-start-at").StoreResult(&timeToStartAt).DefaultValue(TDuration::Minutes(30));

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    NAppHost::NClient::TClientOptions clientOptions;
    {
        clientOptions.BalancerWorkerCount = 16;
        clientOptions.ExecutorsCount = 16;
    }

    NAppHost::NClient::TClient client;
    {
        static constexpr TStringBuf ypPrefix = "yp@";
        if (target.StartsWith(ypPrefix)) {
            TStringBuf cluster;
            TStringBuf endpointSetId;
            if (!TStringBuf(target).Skip(ypPrefix.size()).TrySplit("/", cluster, endpointSetId)) {
                Cout << "Bad target: " << target << Endl;
                return 1;
            }
            NAppHost::NClient::TYpGrpcBackend backend(ToString(cluster), ToString(endpointSetId));
            client.AddOrUpdateBackend("SCHEDULER", backend);
        } else {
            TStringBuf host;
            TStringBuf port;
            if (!TStringBuf(target).TrySplit(":", host, port)) {
                Cout << "Bad target: " << target << Endl;
                return 1;
            }
            NAppHost::NClient::TFixedGrpcBackend backend(ToString(host), FromString<ui16>(port));
            client.AddOrUpdateBackend("SCHEDULER", backend);
        }
    }

    NAppHost::NClient::TStreamOptions streamOptions;
    {
        streamOptions.Path = "/add_scheduled_action";
        streamOptions.MaxAttempts = 1;
        streamOptions.Timeout = TDuration::Seconds(5);
        streamOptions.ChunkTimeout = TDuration::Seconds(5);
    }

    TInstant startAt = TInstant::Now() + timeToStartAt;
    std::atomic<size_t> cntSuccess = 0;
    std::atomic<size_t> cntErrors = 0;
    auto doRequest = [&](ui32 requestId) -> NThreading::TFuture<void> {
        TAtomicSharedPtr<NAppHost::NClient::TStream> stream = MakeAtomicShared<NAppHost::NClient::TStream>(
            client.CreateStream("SCHEDULER", streamOptions)
        );

        NAppHost::NClient::TInputDataChunk dataChunk;
        {
            NMatrix::NScheduler::NApi::TAddScheduledActionRequest addScheduledActionRequest;

            {
                auto& scheduledActionMeta = *addScheduledActionRequest.MutableMeta();
                scheduledActionMeta.SetId(TString::Join(scheduledActionsIdPrefix, ToString(requestId)));
            }

            {
                auto& scheduledActionSpec = *addScheduledActionRequest.MutableSpec();

                scheduledActionSpec.MutableStartPolicy()->MutableStartAt()->CopyFrom(
                    NProtoInterop::CastToProto(startAt)
                );

                scheduledActionSpec.MutableSendPolicy()->MutableSendOncePolicy();
                scheduledActionSpec.MutableAction()->MutableMockAction()->SetName("perf-test");
            }

            addScheduledActionRequest.SetOverrideMode(NMatrix::NScheduler::NApi::TAddScheduledActionRequest::META_AND_SPEC_ONLY);

            dataChunk.AddItem("INIT", "add_scheduled_action_request", addScheduledActionRequest);
        }

        stream->Write(std::move(dataChunk), true);
        return stream->ReadAll().Apply([&, stream = stream](const auto& fut) {
            if (!fut.HasException()) {
                ++cntSuccess;
            } else {
                if (verboseErrors) {
                    try {
                        fut.GetValue();
                    } catch (...) {
                        Cerr << "Request error: " << CurrentExceptionMessage() << Endl;
                    }
                }
                ++cntErrors;
            }
        });
    };

    Cout << "Start at is " << ToString(startAt) << Endl;
    TThreadPool mtp;
    mtp.Start(parallelRequestsCount);
    TVector<NThreading::TFuture<void>> fts;
    for (ui32 i = 0; i < scheduledActionsCount; ++i) {
        fts.push_back(
            NThreading::Async(
                [&, requestId = i]() {
                    doRequest(requestId).GetValueSync();
                }
                , mtp
            )
        );
    }

    auto finishFuture = NThreading::WaitAll(fts);
    while (true) {
        if (finishFuture.HasValue()) {
            break;
        }
        Cout << "Success: " << cntSuccess.load() << ", Errors: " << cntErrors.load() << Endl;
        Sleep(TDuration::Seconds(5));
    }
    Cout << "Success: " << cntSuccess.load() << ", Errors: " << cntErrors.load() << Endl;

    return 0;
}
