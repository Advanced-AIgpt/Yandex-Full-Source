#pragma once

#include <grpcpp/server.h>
#include <grpcpp/completion_queue.h>


namespace NGProxy {

using TGrpcServerPtr = std::unique_ptr<grpc::Server>;

using TCompletionQueuePtr = std::unique_ptr<grpc::ServerCompletionQueue>;

}   // namespace NGProxy
