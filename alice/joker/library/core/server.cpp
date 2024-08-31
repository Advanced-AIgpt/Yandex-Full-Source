#include "server.h"
#include "client.h"
#include "config.h"

#include <alice/joker/library/log/log.h>

namespace NAlice::NJoker {
namespace {

THttpServer::TOptions ConstructHttpServerOptions(const TConfig& config) {
    THttpServer::TOptions options{config.Server().HttpPort()};
    options.SetThreads(config.Server().HttpThreads());
    options.EnableKeepAlive(true);
    options.EnableCompression(true);
    return options;
}

} // namespace

TServer::TServer(TGlobalContext& globalCtx)
    : THttpServer{static_cast<THttpServer::ICallBack*>(this), ConstructHttpServerOptions(globalCtx.Config())}
    , GlobalCtx_{globalCtx}
{
}

TClientRequest* TServer::CreateClient() {
    return new TClient{GlobalCtx_};
}

void TServer::OnListenStart() {
}

void TServer::OnException() {
}

void TServer::OnWait() {
}

void TServer::OnFailRequest(int failState) {
    LOG(ERROR) << "Server failed request: " << failState << Endl;
}

void TServer::OnMaxConn() {
}

} // namespace NJoker
