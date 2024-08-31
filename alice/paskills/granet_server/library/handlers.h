#pragma once

#include <alice/paskills/granet_server/config/proto/config.pb.h>
#include <kernel/server/server.h>
#include <util/generic/yexception.h>


namespace NGranetServer {

class RequestValidationException: public yexception {
};

class TRequestHandler : public NServer::TRequest {
public:
    explicit TRequestHandler(NServer::TServer& server, const TGranetServerConfig& config);

    bool DoReply(const TString& script, THttpResponse& response) override;

private:

    bool HandleCompileRequest(THttpResponse& response);
    void WriteJsonResponse(THttpResponse& response, const NJson::TJsonValue& json);
    void WriteJsonError(THttpResponse& response, const HttpCodes httpCode, const TStringBuf error);

    const TGranetServerConfig Config;

};

} // NGranetServer
