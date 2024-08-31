#pragma once

#include "http_request.h"

#include <alice/bass/libs/globalctx/fwd.h>

#include <library/cpp/http/server/http.h>

class TBassServer: public THttpServer::ICallBack, public THttpServer {
public:
    explicit TBassServer(NBASS::TGlobalContextPtr globalCtx);
    ~TBassServer();

    // THttpServer::ICallBack overrides:
    void OnFailRequest(int failstate) override;
    void OnException() override;
    void OnMaxConn() override;
    void OnListenStart() override;
    void OnWait() override;
    TClientRequest* CreateClient() override;

private:
    THttpHandlersMap HttpHandlers;
    NBASS::TGlobalContextPtr GlobalCtx;
};
