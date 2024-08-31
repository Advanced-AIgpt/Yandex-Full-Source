#include "frame_filler_utils.h"

#include <google/protobuf/any.pb.h>

#include <util/generic/hash.h>
#include <util/generic/is_in.h>

namespace NAlice {
namespace NFrameFiller {

namespace {
    NScenarios::TScenarioResponseBody& InitResponseBody(
        NScenarios::TScenarioRunResponse& response,
        const TFrameFillerRequest& ffRequest
    ) {
        if (ffRequest.HasCommitCandidate()) {
            *response.MutableCommitCandidate() = ffRequest.GetCommitCandidate();
            return *response.MutableCommitCandidate()->MutableResponseBody();
        }

        *response.MutableResponseBody() = ffRequest.GetScenarioResponse();
        return *response.MutableResponseBody();
    }
} // namespace

    TFrameFillerScenarioResponse ToScenarioResponse(const TFrameFillerRequest& ffRequest) {
        TFrameFillerScenarioResponse response;
        *response.MutableFrameFillerRequest() = ffRequest;
        return response;
    }

    TFrameFillerScenarioResponse ToScenarioResponse(const NScenarios::TScenarioRunResponse& runResponse) {
        TFrameFillerScenarioResponse response;
        *response.MutableNatural() = runResponse;
        return response;
    }

    TSemanticFrame& MakeSlotRequested(TSemanticFrame& frame, const TMaybe<TString>& requestedSlotName) {
        for (auto& slot : *frame.MutableSlots()) {
            if (requestedSlotName.Defined() && slot.GetName() == requestedSlotName) {
                slot.SetIsRequested(true);
            } else {
                slot.SetIsRequested(false);
            }
        }
        return frame;
    }

    void EnsureHasVersion(NScenarios::TScenarioRunResponse& response) {
        if (response.GetVersion().empty()) {
            response.SetVersion(DEFAULT_VERSION);
        }
    }

    NScenarios::TScenarioRunResponse BuildScenarioRunResponse(
        TFrameFillerRequest ffRequest,
        const TMaybe<NScenarios::TLayout>& layout,
        const TMaybe<TString>& requestedSlotName
    ) {
        NScenarios::TScenarioRunResponse response;
        auto& responseBody = InitResponseBody(response, ffRequest);

        MakeSlotRequested(*ffRequest.MutableScenarioResponse()->MutableSemanticFrame(), requestedSlotName);

        if (layout.Defined()) {
            *responseBody.MutableLayout() = *layout;
        }

        response.MutableFeatures()->SetIsIrrelevant(false);

        TFrameFillerState ffState;
        *ffState.MutableRequest() = ffRequest;

        responseBody.MutableState()->PackFrom(ffState);

        EnsureHasVersion(response);

        return response;
    }

    TSemanticFrame UpdateFrame(TSemanticFrame frame, const TSemanticFrame& frameUpdate) {
        THashMap<TString, TSemanticFrame::TSlot*> slotNameToSlot;
        for (auto& slot : *frame.MutableSlots()) {
            slotNameToSlot[slot.GetName()] = &slot;
        }

        for (const auto& filledSlot : frameUpdate.GetSlots()) {
            if (slotNameToSlot.contains(filledSlot.GetName())) {
                auto& slot = *slotNameToSlot[filledSlot.GetName()];
                if (!IsIn(slot.GetAcceptedTypes(), filledSlot.GetType())) {
                    continue;
                }
                slot.SetType(filledSlot.GetType());
                slot.SetValue(filledSlot.GetValue());
                slot.SetIsRequested(filledSlot.GetIsRequested());
            } else {
                *frame.AddSlots() = filledSlot;
            }
        }

        return frame;
    }

    NScenarios::TScenarioRunRequest UnwrapState(NScenarios::TScenarioRunRequest request) {
        TFrameFillerState ffState;
        if (!request.GetBaseRequest().GetState().UnpackTo(&ffState)) {
            return request;
        }
        *request.MutableBaseRequest()->MutableState() = GetResponseBody(ffState.GetRequest()).GetState();
        request.MutableInput()->ClearSemanticFrames();
        *request.MutableInput()->AddSemanticFrames() = GetResponseBody(ffState.GetRequest()).GetSemanticFrame();
        return request;
    }

    NScenarios::TScenarioRunResponse UnwrapState(NScenarios::TScenarioRunResponse response) {
        TFrameFillerState ffState;
        if (response.HasResponseBody()) {
            if (!response.GetResponseBody().GetState().UnpackTo(&ffState)) {
                return response;
            }
            *response.MutableResponseBody()->MutableState() = GetResponseBody(ffState.GetRequest()).GetState();
        } else if (response.HasCommitCandidate()) {
            if (!response.GetCommitCandidate().GetResponseBody().GetState().UnpackTo(&ffState)) {
                return response;
            }
            *response.MutableCommitCandidate()
                ->MutableResponseBody()
                ->MutableState() = GetResponseBody(ffState.GetRequest()).GetState();
        }
        return response;
    }

    const NScenarios::TScenarioResponseBody& GetResponseBody(const TFrameFillerRequest& ffRequest) {
        if (ffRequest.HasCommitCandidate()) {
            return ffRequest.GetCommitCandidate().GetResponseBody();
        }

        return ffRequest.GetScenarioResponse();
    }

} // namespace NFrameFiller
} // namespace NAlice
