#pragma once

#include <alice/cuttlefish/library/metrics/metrics.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <apphost/api/service/cpp/service_context.h>


namespace NAlice::NCuttlefish {

    void InitMetrics();


    class TSourceMetrics : public NVoice::NMetrics::TSourceMetrics {
    public:
        explicit TSourceMetrics(TStringBuf sourceName);
        TSourceMetrics(NAppHost::IServiceContext& ctx, TStringBuf sourceName);
        TSourceMetrics(const NAliceProtocol::TSessionContext& sessionCtx, TStringBuf sourceName);

    private:
        static NVoice::NMetrics::TClientInfo MakeClientInfo(const NAliceProtocol::TSessionContext& session);
        static NVoice::NMetrics::TClientInfo MakeClientInfo(NAppHost::IServiceContext& ctx);
    };

}   // namespace NAlice::NCuttlefish
