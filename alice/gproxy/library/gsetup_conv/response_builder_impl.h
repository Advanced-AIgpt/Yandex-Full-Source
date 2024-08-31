#pragma once

#include <library/cpp/json/json_value.h>

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/gproxy/library/protos/gsetup.pb.h>
#include <alice/gproxy/library/protos/metadata.pb.h>
#include <alice/cuttlefish/library/logging/event_log.h>


namespace NGProxy {

using TLogFramePtr = NAlice::NCuttlefish::TLogFramePtr;

class TResponseBuilderImpl {
public:
    TResponseBuilderImpl();

    /**
     *  @brief initialize response builder with raw megamind response
     */
    void SetResponse(NJson::TJsonValue content);

    void SetRequestInfo(const NGProxy::TMetadata& meta, const NGProxy::GSetupRequestInfo& info);

    NGProxy::GSetupResponse Build() const;

protected:
    TLogFramePtr LogFrame;
    virtual void SetGrpcResponse(const NJson::TJsonValue& output, const NGProxy::TMetadata& meta, const NGProxy::GSetupRequestInfo& info, NGProxy::GSetupResponse& resp) const = 0;

private: /* data */
    NJson::TJsonValue Response;
    NGProxy::TMetadata Metadata;
    NGProxy::GSetupRequestInfo RequestInfo;
};


}   // namespace NGProxy
