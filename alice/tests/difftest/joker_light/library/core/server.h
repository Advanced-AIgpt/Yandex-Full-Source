#pragma once

#include <library/cpp/http/server/http.h>

#include "context.h"

namespace NAlice::NJokerLight {

class TServer : public THttpServer, public THttpServer::ICallBack {
public:
    TServer(TContext& context);

private:
    void OnFailRequest(int failState) override;
    TClientRequest* CreateClient() override;

private:
    TContext& Context_;
};

} // namespace NAlice::NJokerLight
