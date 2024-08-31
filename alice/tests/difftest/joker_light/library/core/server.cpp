#include "server.h"

#include "client.h"

#include <alice/joker/library/log/log.h>

namespace NAlice::NJokerLight {

namespace {

THttpServer::TOptions ConstructHttpServerOptions(const TConfig& config) {
    THttpServer::TOptions options{config.Port()};
    options.SetThreads(config.Threads());
    options.EnableKeepAlive(true);
    options.EnableCompression(true);
    return options;
}

} // namespace

TServer::TServer(TContext& context)
    : THttpServer{static_cast<THttpServer::ICallBack*>(this), ConstructHttpServerOptions(context.Config())}
    , Context_{context}
{
}

void TServer::OnFailRequest(int failState) {
    LOG(ERROR) << "Server failed request: " << failState << Endl;
}

TClientRequest* TServer::CreateClient() {
    return new TClient{Context_};
}

} // namespace NAlice::NJokerLight
