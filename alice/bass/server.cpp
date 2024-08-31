#include "server.h"

#include "http_request.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/logging_v2/logger.h>

namespace {

THttpServer::TOptions ConstructHttpServerOptions(NBASS::IGlobalContext& globalCtx) {
    THttpServer::TOptions options(globalCtx.Config().GetHttpPort());
    options.SetThreads(globalCtx.Config().HttpThreads());
    options.SetMaxConnections(globalCtx.Config().HttpConnections());
    options.EnableKeepAlive(true);
    options.EnableCompression(true);
    return options;
}

} // namespace

TBassServer::TBassServer(NBASS::TGlobalContextPtr globalCtx)
    : THttpServer(static_cast<THttpServer::ICallBack*>(this), ConstructHttpServerOptions(*globalCtx))
    , HttpHandlers(globalCtx)
    , GlobalCtx(globalCtx)
{
}

TBassServer::~TBassServer() = default;

TClientRequest* TBassServer::CreateClient() {
    return new THttpReplier(GlobalCtx, HttpHandlers);
}

void TBassServer::OnListenStart() {
    Y_STATS_INC_COUNTER("bass_server_start");
}

void TBassServer::OnException() {
    Y_STATS_INC_COUNTER("bass_server_exception");
}

void TBassServer::OnWait() {
    Y_STATS_INC_COUNTER("bass_server_wait");
}

void TBassServer::OnFailRequest(int failState) {
    LOG(ERR) << "Server failed request: " << failState << Endl;
    Y_STATS_INC_COUNTER("bass_server_wait");
}

void TBassServer::OnMaxConn() {
    Y_STATS_INC_COUNTER("bass_server_maxconn");
}
