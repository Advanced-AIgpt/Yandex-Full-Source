#include "frame_filler_handlers.h"
#include "frame_filler_utils.h"

#include <alice/library/proto/proto.h>
#include <alice/library/response/defs.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/util/slot.h>
#include <alice/megamind/protos/common/frame.pb.h>

#include <apphost/api/service/cpp/service_context.h>

#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/generic/yexception.h>

#include <functional>
#include <tuple>

namespace NAlice {
namespace NFrameFiller {

using TLayout = NScenarios::TLayout;
using TScenarioRunRequest = NScenarios::TScenarioRunRequest;
using TScenarioRunResponse = NScenarios::TScenarioRunResponse;

namespace {
    struct TSlotRequest {
        TString SlotName;
        TLayout Layout;
    };

    struct TFrameFillerRequestContext {
        TFrameFillerRequest Request;
    };

    struct TForwardClientResponseContext {
        TFrameFillerRequest Request;
    };

    using TClientScenarioRequest = std::variant<
        TScenarioRunRequest,
        TScenarioRunResponse,
        TFrameFillerRequestContext
    >;
    using TClientScenarioResponse = std::variant<
        TScenarioRunResponse,
        TForwardClientResponseContext,
        TFrameFillerRequestContext
    >;
    using TClientScenarioRequester = std::function<TClientScenarioResponse(const TClientScenarioRequest&)>;

    TMaybe<TError<TScenarioRunResponse>> ValidateRequest(const TFrameFillerRequest& ffRequest) {
        const auto logicError = [&](){ return TError<TScenarioRunResponse>{}; };

        if (!ffRequest.HasScenarioResponse()) {
            return logicError() << "No scenario response.";
        }
        if (!GetResponseBody(ffRequest).HasSemanticFrame()) {
            return logicError() << "No frame to fill.";
        }
        const TSemanticFrame& frame = GetResponseBody(ffRequest).GetSemanticFrame();

        const TString& frameName = frame.GetName();
        if (frameName.empty()) {
            return logicError() << "Frame to fill has no name.";
        }

        const auto frameError = [&](){ return logicError() << "Frame " << frameName; };

        THashSet<TString> slots;
        for (const auto& slot : frame.GetSlots()) {
            const auto& slotName = slot.GetName();
            if (slotName.empty()) {
                return frameError() << " has slot with no name";
            }

            if (slot.GetAcceptedTypes().size() == 0) {
                return frameError() << " has slot " << slotName << " with no accepted types.";
            }

            // TODO(the0): Check slot accepted types somehow

            slots.insert(slotName);
        }

        for (const TSlotRequirement& requirement : ffRequest.GetSlotRequirements()) {
            const auto& slotName = requirement.GetSlotName();
            if (!slots.contains(slotName)) {
                return logicError() << "Frame has no such slot: " << slotName << ".";
            }

            if (requirement.GetLayoutAlternatives().empty()) {
                return logicError() << "Requirement for slot " << slotName << " has no ask alternatives.";
            }
        }

        return Nothing();
    }

    bool MeetsRequirement(const TSemanticFrame& frame, const TString& slotName) {
        for (const auto& slot : frame.GetSlots()) {
            const bool hasValue = !slot.GetType().empty();
            if (slot.GetName() == slotName && hasValue) {
                return true;
            }
        }
        return false;
    }

    TMaybe<TSlotRequest> GetSlotRequest(const TSemanticFrame& frame,
                                        const TFrameFillerRequest& ffRequest) {
        for (const TSlotRequirement& requirement : ffRequest.GetSlotRequirements()) {
            if (MeetsRequirement(frame, requirement.GetSlotName())) {
                continue;
            }
            return TSlotRequest{
                requirement.GetSlotName(),
                requirement.GetLayoutAlternatives(0)
            };
        }
        return Nothing();
    }

    TScenarioRunResponse ProcessAction(
        const NScenarios::TFrameAction& action,
        const TFrameFillerRequest& ffRequest
    ) {
        TLayout layout;
        layout.AddCards()->SetText(NResponse::THREE_DOTS);
        // FIXME(the0): support Callback and Frame effects
        *layout.MutableDirectives() = action.GetDirectives().GetList();
        TScenarioRunResponse response = BuildScenarioRunResponse(ffRequest, layout);
        return response;
    }

    TVector<TString> GetRequiredSlotNames(const TFrameFillerRequest& ffRequest) {
        TVector<TString> result;
        for (const auto& requirement : ffRequest.GetSlotRequirements()) {
            result.push_back(requirement.GetSlotName());
        }
        return result;
    }

    THashSet<TString> GetFrameFilledSlotNames(const TSemanticFrame& frame) {
        THashSet<TString> result;
        for (const auto& slot : frame.GetSlots()) {
            const bool isFilled = !slot.GetType().empty();
            if (isFilled) {
                result.insert(slot.GetName());
            }
        }
        return result;
    }

    TMaybe<ui32> GetRelatedFrameIndex(const TFrameFillerRequest& ffRequest,
                                      const TMaybe<TString>& requestedSlotName,
                                      const ::google::protobuf::RepeatedPtrField<TSemanticFrame>& inputFrames) {
        const TVector<TString> requriedSlotNames = GetRequiredSlotNames(ffRequest);
        TMaybe<ui32> result;
        for (ui32 index = 0; index < static_cast<ui32>(inputFrames.size()); ++index) {
            const auto& frame = inputFrames[index];
            if (frame.GetName() != GetResponseBody(ffRequest).GetSemanticFrame().GetName()) {
                continue;
            }
            const THashSet<TString> filledFrameSlotNames = GetFrameFilledSlotNames(frame);
            for (const auto& requiredSlotName : requriedSlotNames) {
                if (!filledFrameSlotNames.contains(requiredSlotName)) {
                    break;
                }
                if (requestedSlotName.Defined() && requiredSlotName == requestedSlotName) {
                    return index;
                }
            }
            // Will return the first frame with the same name as the frame in ffRequest
            // if no input frame with such a name has required slot filled.
            if (!result.Defined()) {
                result = index;
            }
        }

        return result;
    }

    TMaybe<TError<TScenarioRunResponse>> UpdateStateFromRequest(const NHollywood::TScenarioRunRequestWrapper& request, TFrameFillerState& ffState) {
        if (!ffState.HasRequest()) {
            return TError<TScenarioRunResponse>{} << "FrameFiller state has no request.";
        }
        TFrameFillerRequest& ffRequest = *ffState.MutableRequest();

        if (const auto errorResponse = ValidateRequest(ffRequest); errorResponse.Defined()) {
            return *errorResponse;
        }

        if (!request.Proto().HasInput()) {
            return TError<TScenarioRunResponse>{} << "Request has no input.";
        }

        TSemanticFrame* frame = ffRequest.MutableScenarioResponse()->MutableSemanticFrame();
        const auto& requestedSlot = GetRequestedSlot(*frame);
        TMaybe<TString> requestedSlotName = Nothing();
        if (requestedSlot.Defined()) {
            requestedSlotName = requestedSlot->GetName();
        }

        const auto& inputFrames = request.Input().Proto().GetSemanticFrames();
        const TMaybe<ui32> usedInputFrameIndex = GetRelatedFrameIndex(ffRequest, requestedSlotName, inputFrames);
        if (usedInputFrameIndex.Defined()) {
            *frame = UpdateFrame(*frame, inputFrames[*usedInputFrameIndex]);
        }

        return Nothing();
    }

    TClientScenarioRequest DoBeforeRequestToScenario(const NHollywood::TScenarioRunRequestWrapper& request) {
        // TODO(the0): log run request
        if (!request.Proto().HasBaseRequest()) {
            return TScenarioRunResponse{TError<TScenarioRunResponse>{} << "Unassigned BaseRequest field."};
        }
        const auto& baseRequest = request.Proto().GetBaseRequest();

        if (const bool shouldProxyRequest = !baseRequest.HasState() || !baseRequest.GetState().Is<TFrameFillerState>()) {
            return request.Proto();
        }
        TFrameFillerState ffState;
        baseRequest.GetState().UnpackTo(&ffState);

        if (request.Input().GetCallback() != nullptr) {
            return UnwrapState(request.Proto());
        }

        if (const auto errorResponse = UpdateStateFromRequest(request, ffState); errorResponse.Defined()) {
            return TScenarioRunResponse{*errorResponse};
        }

        const TSemanticFrame& frame = GetResponseBody(ffState.GetRequest()).GetSemanticFrame();

        NScenarios::TScenarioRunRequest result;
        *result.MutableBaseRequest() = request.Proto().GetBaseRequest();
        *result.MutableInput() = request.Proto().GetInput();
        *result.MutableDataSources() = request.Proto().GetDataSources();
        result = UnwrapState(result);

        if (const auto slotRequest = GetSlotRequest(frame, ffState.GetRequest());
            !slotRequest.Defined() && !ffState.GetRequest().HasOnSubmit()
        ) {
            result.MutableInput()->MutableCallback()->SetName(ON_SUBMIT_CALLBACK_NAME);
        }

        return result;
    }

    TClientScenarioResponse RequestScenario(
        const IFrameFillerScenarioRunHandler& handler,
        const TClientScenarioRequest& clientRequest,
        TRTLogger& logger,
        const NAppHost::IServiceContext& serviceCtx
    ) {
        if (const auto* scenarioRunResponse = std::get_if<TScenarioRunResponse>(&clientRequest)) {
            return *scenarioRunResponse;
        }
        if (const auto* ffRequestCtx = std::get_if<TFrameFillerRequestContext>(&clientRequest)) {
            return *ffRequestCtx;
        }
        const auto& runRequestProto = std::get<TScenarioRunRequest>(clientRequest);
        const NHollywood::TScenarioRunRequestWrapper runRequest{runRequestProto, serviceCtx};

        TFrameFillerScenarioResponse scenarioResponse = handler.Do(runRequest, logger);
        const auto responseCase = scenarioResponse.GetResponseCase();
        switch (responseCase) {
            case TFrameFillerScenarioResponse::ResponseCase::kNatural:
                return scenarioResponse.GetNatural();
            case TFrameFillerScenarioResponse::ResponseCase::kFrameFillerRequest:
                if (GetResponseBody(scenarioResponse.GetFrameFillerRequest()).HasLayout()) {
                    return TForwardClientResponseContext{scenarioResponse.GetFrameFillerRequest()};
                }

                // TODO(the0): add frame filler request validation
                return TFrameFillerRequestContext{scenarioResponse.GetFrameFillerRequest()};
            case TFrameFillerScenarioResponse::ResponseCase::RESPONSE_NOT_SET:
                return TScenarioRunResponse{TError<TScenarioRunResponse>{} << "Unexpected response case: " << static_cast<int>(responseCase)};
        }
        return TScenarioRunResponse{TError<TScenarioRunResponse>{} << "Unexpected response case: " << static_cast<int>(responseCase)};
    }

    TScenarioRunResponse DoAfterRequestToScenario(const TClientScenarioResponse& clientScenarioResponse) {
        if (const auto* response = std::get_if<TScenarioRunResponse>(&clientScenarioResponse)) {
            return *response;
        }

        if (const auto* forwardClientResponseCtx = std::get_if<TForwardClientResponseContext>(&clientScenarioResponse)) {
            const TFrameFillerRequest& ffRequest = forwardClientResponseCtx->Request;
            return UnwrapState(BuildScenarioRunResponse(ffRequest, Nothing(), Nothing()));
        }

        const auto& ffCtx = std::get<TFrameFillerRequestContext>(clientScenarioResponse);
        const TFrameFillerRequest& ffRequest = ffCtx.Request;

        const TSemanticFrame& frame = GetResponseBody(ffRequest).GetSemanticFrame();

        if (const auto slotRequest = GetSlotRequest(frame, ffRequest); slotRequest.Defined()) {
            TScenarioRunResponse response = BuildScenarioRunResponse(
                ffRequest, slotRequest->Layout, slotRequest->SlotName
            );
            response.MutableResponseBody()->MutableLayout()->SetShouldListen(true);
            return response;
        }

        if (!ffRequest.HasOnSubmit()) {
            return TScenarioRunResponse{TError<TScenarioRunResponse>{}
                << "Requested for frame filling with no unmet slot requirements "
                << "and with no submit action provided"
            };
        }

        return ProcessAction(ffRequest.GetOnSubmit(), ffRequest);
    }
} // namespace

NScenarios::TScenarioRunResponse Run(
    const NHollywood::TScenarioRunRequestWrapper& request,
    const IFrameFillerScenarioRunHandler& handler,
    TRTLogger& logger
) {
    const TClientScenarioRequest beforeRequestToScenarioResult =
        DoBeforeRequestToScenario(request);

    const TClientScenarioResponse afterRequestToScenarioResult =
        RequestScenario(handler, beforeRequestToScenarioResult, logger, request.ServiceCtx());

    TScenarioRunResponse response = DoAfterRequestToScenario(afterRequestToScenarioResult);
    EnsureHasVersion(response);

    LOG_INFO(logger) << "Frame filler response: " << SerializeProtoText(response);

    return response;
}

NScenarios::TScenarioCommitResponse Commit(
    const NHollywood::TScenarioApplyRequestWrapper& request,
    const IFrameFillerScenarioCommitHandler& handler,
    TRTLogger& logger
) {
    return handler.Do(request, logger);
}

NScenarios::TScenarioApplyResponse Apply(
    const NHollywood::TScenarioApplyRequestWrapper& request,
    const IFrameFillerScenarioApplyHandler& handler,
    TRTLogger& logger
) {
    return handler.Do(request, logger);
}

} // namespace NFrameFiller
} // namespace NAlice
