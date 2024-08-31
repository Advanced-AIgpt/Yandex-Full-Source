#pragma once

#include <alice/paskills/granet_server/config/proto/config.pb.h>

#include <kernel/server/server.h>
#include <kernel/server/protos/serverconf.pb.h>

#include "handlers.h"

namespace NGranetServer {

class TGranetServer : public NServer::TServer {
public:
    explicit TGranetServer(const TGranetServerConfig& config);

    TClientRequest* CreateClient() override;

    const TGranetServerConfig& GetGranetServerConfig() const;

private:
    const TGranetServerConfig Config;
};

} // NGranetServer

