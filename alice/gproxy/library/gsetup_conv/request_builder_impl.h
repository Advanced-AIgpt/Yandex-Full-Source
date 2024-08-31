#pragma once

#include <library/cpp/json/json_value.h>

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/gproxy/library/protos/gsetup.pb.h>
#include <alice/gproxy/library/protos/metadata.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>


namespace NGProxy {


class TRequestBuilderImpl {
public:
    TRequestBuilderImpl();

    void SetSession(const NAliceProtocol::TSessionContext& session);

    void SetContext(const NAliceProtocol::TContextLoadResponse& context);

    void SetGrpcData(const NGProxy::TMetadata& meta, const NGProxy::GSetupRequestInfo& info, const NGProxy::GSetupRequest& req);


    NAppHostHttp::THttpRequest Build() const;

protected:
    virtual void SetGrpcRequest(NJson::TJsonValue& output, const NGProxy::TMetadata& meta, const NGProxy::GSetupRequestInfo& info, const NGProxy::GSetupRequest& req) = 0;

private: /* methods */
    void InitRequest();

private: /* data */
    NGProxy::TMetadata              Metadata;
    NAliceProtocol::TSessionContext Session;
    NJson::TJsonValue               Request;
    TString                         RequestBytes;
};


}   // namespace NGProxy
