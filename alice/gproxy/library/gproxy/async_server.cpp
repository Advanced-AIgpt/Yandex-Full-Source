#include <alice/gproxy/library/gproxy/async_server.h>

#include <grpcpp/server_builder.h>


namespace NGProxy {

TAsyncServer::TAsyncServer(const TGrpcServerConfig& config, TLoggingSubsystem& logging, TMetricsSubsystem& metrics, TAppHostSubsystem& apphost)
    : Config_(config)
    , Logging_(logging)
    , Metrics_(metrics)
    , AppHost_(apphost)
    , Service_(logging, metrics, apphost)
{ }


void TAsyncServer::BuildAndStart() {
    grpc::ServerBuilder Builder;

    Builder.AddListeningPort("*:" + ToString(Config_.GetPort()), grpc::InsecureServerCredentials());
    // set max metadata size to 1 MB
    Builder.AddChannelArgument("GRPC_ARG_MAX_METADATA_SIZE", 1*1024*1024);

    Builder.RegisterService(Service_.Get());
    Service_.SetQueue(Builder.AddCompletionQueue());
    Service_.SetAllowSrcrwr(Config_.GetAllowSrcrwr());
    Service_.SetAllowDumpRequestsResponses(Config_.GetAllowDumpRequestsResponses());

    Server_ = Builder.BuildAndStart();
}


void TAsyncServer::Wait() {
    // if there are more than one service, organize waiting loop here
    Service_.DoRpcs();
}

}   // namespace NGProxy
