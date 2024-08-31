#pragma once

#include <alice/megamind/library/analytics/megamind_analytics_info.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/scenarios/registry/interface/registry.h>
#include <alice/megamind/library/session/session.h>
#include <alice/megamind/library/util/guid.h>
#include <alice/megamind/library/util/http_response.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/typed_frame/typed_frames.h>

namespace NAlice {

class TDirectiveListResponse {
public:
    TDirectiveListResponse(
        const ::google::protobuf::RepeatedPtrField<TUntypedDirective>& directives,
        const TSpeechKitRequest& request,
        const NMegamind::IGuidGenerator& guidGenerator
    );

    TDirectiveListResponse(
        const ::google::protobuf::RepeatedPtrField<NScenarios::TDirective>& directives,
        const TSpeechKitRequest& request,
        const TMaybe<TIoTUserInfo>& iotUserInfo,
        const ISession& session,
        TRTLogger& logger,
        const NMegamind::IGuidGenerator& guidGenerator
    );

    const TSpeechKitResponseProto& GetProto() const {
        return Response;
    }

private:
    TDirectiveListResponse(
        const TVector<NSpeechKit::TDirective>& directives,
        const TSpeechKitRequest& request,
        const NMegamind::IGuidGenerator& guidGenerator
    );

private:
    TSpeechKitResponseProto Response;
};

using TActionResponse = std::variant<
    TDirectiveListResponse,
    NScenarios::TCallbackDirective,
    NScenarios::TParsedUtterance,
    TSemanticFrame,
    TVector<TSemanticFrameRequestData>
>;

TMaybe<TActionResponse>
TryGetActionResponse(const IContext& ctx, const TMaybe<TIoTUserInfo>& iotUserInfo,
                     NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                     const NMegamind::IGuidGenerator& guidGenerator = NMegamind::TGuidGenerator{});

TMaybe<TActionResponse> TryGetScenarioActionResponse(const IContext& ctx,
                                                     const TMaybe<TIoTUserInfo>& iotUserInfo,
                                                     NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                                     const NMegamind::IGuidGenerator& guidGenerator);

TMaybe<TActionResponse> TryGetDeviceActionResponse(const IContext& ctx,
                                                   NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                                   const NMegamind::IGuidGenerator& guidGenerator);

bool ProcessConditionalAction(const TTypedSemanticFrame& activationFrame, const TVector<TTypedSemanticFrame>& requestFrames);

TMaybe<TSemanticFrameRequestData>
TryGetConditionalEffect(const IContext& ctx,
                        NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder);

IScenarioRegistry::TFramesToScenarios AddDynamicAcceptedFrames(
    IScenarioRegistry::TFramesToScenarios framesToScenarios,
    const ISession* session
);

} // namespace NAlice
