#include "scenario_request_builder.h"

#include <alice/library/client/protos/client_info.pb.h>

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/request_composite/client/client.h>

namespace NAlice::NMegamind::NRpc {

namespace {

class TScenarioRpcRequestBuilder {
public:
    TScenarioRpcRequestBuilder() = default;

    TScenarioRpcRequestBuilder& SetRequestId(const TString& requestId) {
        Request.MutableBaseRequest()->SetRequestId(requestId);
        return *this;
    }

    TScenarioRpcRequestBuilder& SetServerTimeMs(ui64 serverTimeMs) {
        Request.MutableBaseRequest()->SetServerTimeMs(serverTimeMs);
        return *this;
    }

    TScenarioRpcRequestBuilder& SetRandomSeed(ui64 randomSeed) {
        Request.MutableBaseRequest()->SetRandomSeed(randomSeed);
        return *this;
    }

    TScenarioRpcRequestBuilder& SetClientInfo(const TClientInfoProto& clientInfo) {
        Request.MutableBaseRequest()->MutableClientInfo()->CopyFrom(clientInfo);
        return *this;
    }

    TScenarioRpcRequestBuilder& SetLocation(const TMaybe<TRequest::TLocation>& loc) {
        if (loc.Defined()) {
            auto& location = *Request.MutableBaseRequest()->MutableLocation();
            location.SetLat(loc->GetLatitude());
            location.SetLon(loc->GetLongitude());
            location.SetAccuracy(loc->GetAccuracy());
            location.SetRecency(loc->GetRecency());
            location.SetSpeed(loc->GetSpeed());
        }
        return *this;
    }

    TScenarioRpcRequestBuilder& SetLanguage(ELang language) {
        Request.MutableBaseRequest()->SetUserLanguage(language);
        return *this;
    }

    TScenarioRpcRequestBuilder& SetExperiments(const TExperimentsProto& exps) {
        TExpFlagsToStructVisitor{*Request.MutableBaseRequest()->MutableExperiments()}.Visit(exps);
        return *this;
    }

    TScenarioRpcRequestBuilder& SetRequest(const google::protobuf::Any& request) {
        Request.MutableRequest()->CopyFrom(request);
        return *this;
    }

    TScenarioRpcRequestBuilder& SetInterfaces(const TRequest::TInterfaces& interfaces) {
        Request.MutableBaseRequest()->MutableInterfaces()->CopyFrom(interfaces);
        return *this;
    }

    NScenarios::TScenarioRpcRequest Build() && {
        return std::move(Request);
    }

private:
    NScenarios::TScenarioRpcRequest Request;
};

} // namespace

NScenarios::TScenarioRpcRequest CreateScenarioRpcRequest(const NAlice::NRpc::TRpcRequestProto& request,
                                                         const TMaybe<TRequest::TLocation>& location,
                                                         TRTLogger& logger) {
    const auto userLanguage =
        IContext::ForceKnownLanguage(logger, LanguageByName(request.GetMeta().GetApplication().GetLang()),
                                     TExpFlagsConverter::Build(request.GetMeta().GetExperiments()));
    const auto interfaces = ParseInterfaces(request.GetMeta().GetExperiments(),
                                            request.GetMeta().GetSupportedFeatures(),
                                            request.GetMeta().GetUnsupportedFeatures(),
                                            request.GetMeta().GetApplication(),
                                            /* isTvPluggedIn= */ false, /* voiceSession= */ false);

    auto builder =
        TScenarioRpcRequestBuilder()
            .SetRequestId(request.GetMeta().GetRequestId())
            .SetServerTimeMs(request.GetMeta().GetServerTimeMs())
            .SetRandomSeed(request.GetMeta().GetRandomSeed())
            .SetLanguage(static_cast<ELang>(userLanguage))
            .SetExperiments(request.GetMeta().GetExperiments())
            .SetRequest(request.GetRequest())
            .SetLocation(location)
            .SetInterfaces(interfaces)
            .SetClientInfo(request.GetMeta().GetApplication());

    return std::move(builder).Build();
}

} // namespace NAlice::NMegamind
