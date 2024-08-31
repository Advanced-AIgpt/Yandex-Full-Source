#include <alice/paskills/granet_server/library/handlers.h>

#include "server.h"

namespace NGranetServer {

TGranetServer::TGranetServer(const TGranetServerConfig& config)
    : NServer::TServer{config.GetHttpServer()}
    , Config(config) {
}

TClientRequest* TGranetServer::CreateClient() {
    return new TRequestHandler(*this, GetGranetServerConfig());
}

const TGranetServerConfig& TGranetServer::GetGranetServerConfig() const {
    return Config;
}

} // NGranetServer
