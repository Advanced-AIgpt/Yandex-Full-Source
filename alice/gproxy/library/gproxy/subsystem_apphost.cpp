#include "subsystem_apphost.h"

#include <apphost/api/client/stream_options.h>
#include <apphost/api/client/yp_grpc_backend.h>
#include <apphost/api/client/fixed_grpc_backend.h>

#include <voicetech/library/itags/itags.h>


namespace {
    TString GetGeo() {
        const TString& geo = TInstanceTags::Get().Geo;
        if (geo.empty()) {
            return "sas";
        }
        return geo;
    }

    TString GetEndpointSet(const NGProxy::TVerticalConfig& vertical) {
        const TStringBuf ctype = TInstanceTags::Get().CtypeStr;
        TString defaultEndpointSet;

        for (size_t i = 0; i < vertical.EndpointSetsSize(); ++i) {
            const NGProxy::TEndpointSetConfig& cfg = vertical.GetEndpointSets(i);
            if (cfg.GetCtype() == ctype) {
                return cfg.GetEndpointSet();
            }
            if (cfg.GetCtype() == "default") {
                defaultEndpointSet = cfg.GetEndpointSet();
            }
        }

        if (defaultEndpointSet.empty()) {
            Cerr << "No endpoint set found for current ctype=" << ctype << Endl;
            exit(1);
        }

        return defaultEndpointSet;
    }

}  // anonymous namespace


namespace NGProxy {


    TAppHostSubsystem::TAppHostSubsystem(const TAppHostClientConfig& config, TLoggingSubsystem&, TMetricsSubsystem&)
        : Config_(config)
    {
    }

    void TAppHostSubsystem::Init() {
        for (size_t i = 0; i < Config_.VerticalsSize(); ++i) {
            AddBackendForVertical(Config_.GetVerticals(i));
        }
    }

    void TAppHostSubsystem::Wait() {
    }

    void TAppHostSubsystem::Stop() {
    }

    NAppHost::NClient::TStream TAppHostSubsystem::CreateStream(
        TStringBuf vertical,
        TStringBuf graph,
        int64_t timeout
    ) {
        NAppHost::NClient::TStreamOptions opts;
        opts.Path = TString(graph);
        opts.MaxAttempts = 1;
        opts.Timeout = TDuration::MilliSeconds(timeout);
        return Client.CreateStream(TString(vertical), opts);
    }

    void TAppHostSubsystem::AddBackendForVertical(const TVerticalConfig& vertical) {
        if (vertical.HasFixedEndpoint() && vertical.GetFixedEndpoint().HasHost()) {
            NAppHost::NClient::TFixedGrpcBackend backend(
                vertical.GetFixedEndpoint().GetHost(),
                vertical.GetFixedEndpoint().GetPort()
            );
            Client.AddOrUpdateBackend(vertical.GetVertical(), backend);
        } else {
            NAppHost::NClient::TYpGrpcBackend backend(GetGeo(), GetEndpointSet(vertical));
            Client.AddOrUpdateBackend(vertical.GetVertical(), backend);
        }
    }


}   // namespace NGProxy
