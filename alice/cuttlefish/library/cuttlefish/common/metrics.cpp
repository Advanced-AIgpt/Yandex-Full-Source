#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/metrics/solomon.h>


namespace NAlice::NCuttlefish {

    void InitMetrics() {
        NVoice::NMetrics::TMetrics& metrics = NVoice::NMetrics::TMetrics::Instance();

        NVoice::NMetrics::TAggregationRules rules;

        metrics.SetBackend(
            NVoice::NMetrics::EMetricsBackend::Solomon,
            MakeHolder<NVoice::NMetrics::TSolomonBackend>(
                rules,
                NVoice::NMetrics::MakeMillisBuckets(),
                "cuttlefish",
                /* maskHost = */ false
            )
        );
    }


    TSourceMetrics::TSourceMetrics(TStringBuf sourceName)
        : NVoice::NMetrics::TSourceMetrics(
            sourceName,
            MakeEmptyClientInfo(),
            NVoice::NMetrics::EMetricsBackend::Solomon
        )
    {}

    TSourceMetrics::TSourceMetrics(NAppHost::IServiceContext& ctx, TStringBuf sourceName)
        : NVoice::NMetrics::TSourceMetrics(
            sourceName,
            MakeClientInfo(ctx),
            NVoice::NMetrics::EMetricsBackend::Solomon
        )
    {}

    TSourceMetrics::TSourceMetrics(const NAliceProtocol::TSessionContext& sessionCtx, TStringBuf sourceName)
        : NVoice::NMetrics::TSourceMetrics(
            sourceName,
            MakeClientInfo(sessionCtx),
            NVoice::NMetrics::EMetricsBackend::Solomon
        )
    {}

    NVoice::NMetrics::TClientInfo TSourceMetrics::MakeClientInfo(const NAliceProtocol::TSessionContext& session) {
        try {
            NVoice::NMetrics::TClientInfo info;
            if (session.GetUserInfo().GetUuidKind() == NAliceProtocol::TUserInfo::USER) {
                info.ClientType = NVoice::NMetrics::EClientType::User;
            } else {
                info.ClientType = NVoice::NMetrics::EClientType::Robot;
            }
            info.GroupName = session.GetSurface();
            info.AppId = session.GetAppId();

            if (info.GroupName == "quasar") {
                info.DeviceName = session.GetDeviceInfo().GetDeviceModel();
            } else {
                info.DeviceName = "other";
            }

            switch (session.GetSurfaceType()) {
                case NAliceProtocol::TSessionContext::ESurfaceType::TSessionContext_ESurfaceType_T_PROD:
                case NAliceProtocol::TSessionContext::ESurfaceType::TSessionContext_ESurfaceType_T_PUBLIC:
                    info.SubgroupName = "prod";
                    break;
                case NAliceProtocol::TSessionContext::ESurfaceType::TSessionContext_ESurfaceType_T_BETA:
                    info.SubgroupName = "beta";
                    break;
                default:
                    info.SubgroupName = "all";
                    break;
            }

            return info;
        } catch (...) {
            // do nothing and ignore exception
        }

        return MakeEmptyClientInfo();
    }

    NVoice::NMetrics::TClientInfo TSourceMetrics::MakeClientInfo(NAppHost::IServiceContext& ctx) {
        auto items = ctx.GetProtobufItemRefs(ITEM_TYPE_SESSION_CONTEXT, NAppHost::EContextItemSelection::Input);
        if (items.empty()) {
            return MakeEmptyClientInfo();
        }

        try {
            NAliceProtocol::TSessionContext session;
            ParseProtobufItem(items.front(), session);
            return MakeClientInfo(session);
        } catch (...) {
            // do nothing and ignore exception
        }

        return MakeEmptyClientInfo();
    }

}   // namespace NAlice::NCuttlefish
