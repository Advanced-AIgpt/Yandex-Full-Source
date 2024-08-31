#pragma once

#include "globalctx.h"

#include <library/cpp/http/server/http.h>

namespace NAlice::NJoker {

class TServer: public THttpServer::ICallBack, public THttpServer {
public:
    explicit TServer(TGlobalContext& globalCtx);

private:
    // THttpServer::ICallBack overrides:
    void OnFailRequest(int failstate) override;
    void OnException() override;
    void OnMaxConn() override;
    void OnListenStart() override;
    void OnWait() override;
    TClientRequest* CreateClient() override;

private:
    TGlobalContext& GlobalCtx_;
};

} // namespace NJoker
