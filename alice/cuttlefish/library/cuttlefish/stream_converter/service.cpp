#include "service.h"
#include "proto_to_ws_stream.h"
#include "ws_stream_to_proto.h"
#include <alice/cuttlefish/library/cuttlefish/common/utils.h>
#include <alice/cuttlefish/library/experiments/experiments.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/json/json_reader.h>


using namespace NAlice::NCuttlefish::NAppHostServices;


namespace {

    const NVoice::NExperiments::TExperiments& GetExperiments()
    {
        static const NVoice::NExperiments::TExperiments globalExperiments(
            ReadJson(NResource::Find("/experiments/experiments.json")),
            ReadJson(NResource::Find("/experiments/macros.json"))
        );

        return globalExperiments;
    }

}  // anonymous namespace


NThreading::TPromise<void> NAlice::NCuttlefish::NAppHostServices::StreamRawToProtobuf(NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    TIntrusivePtr<TWsStreamToProtobuf> stream(new TWsStreamToProtobuf(ctx, GetExperiments(), std::move(logContext)));
    stream->OnNextInput();
    return stream->Promise;
}

NThreading::TPromise<void> NAlice::NCuttlefish::NAppHostServices::StreamProtobufToRaw(NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    TIntrusivePtr<TProtobufToWsStream> stream(new TProtobufToWsStream(ctx, std::move(logContext)));
    stream->OnNextInput();
    return stream->Promise;
}
