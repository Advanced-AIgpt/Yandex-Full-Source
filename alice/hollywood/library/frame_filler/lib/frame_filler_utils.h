#pragma once

#include <alice/hollywood/library/frame_filler/proto/frame_filler_request.pb.h>
#include <alice/hollywood/library/frame_filler/proto/frame_filler_state.pb.h>
#include <alice/hollywood/library/frame_filler/proto/scenario_response.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/stream/str.h>
#include <util/system/yassert.h>

namespace NAlice {
namespace NFrameFiller {
    template <typename TResponseProto>
    class TError : public TResponseProto { };

    template <typename TResponseProto, typename T>
    static inline TError<TResponseProto>& operator<<(TError<TResponseProto>& errorProto, const T& t) {
        TStringOutput out{*errorProto.MutableError()->MutableMessage()};
        out << t;

        return errorProto;
    }

    template <typename TResponseProto, typename T>
    static inline TError<TResponseProto>&& operator<<(TError<TResponseProto>&& errorProto, const T& t) {
        TStringOutput out{*errorProto.MutableError()->MutableMessage()};
        out << t;

        return std::move(errorProto);
    }

    TFrameFillerScenarioResponse ToScenarioResponse(const TFrameFillerRequest& ffRequest);

    TFrameFillerScenarioResponse ToScenarioResponse(const NScenarios::TScenarioRunResponse& runResponse);

    TSemanticFrame& MakeSlotRequested(TSemanticFrame& frame, const TMaybe<TString>& requestedSlotName = {});

    inline const TString DEFAULT_VERSION = "0";
    void EnsureHasVersion(NScenarios::TScenarioRunResponse& response);

    NScenarios::TScenarioRunResponse BuildScenarioRunResponse(
        TFrameFillerRequest ffRequest,
        const TMaybe<NScenarios::TLayout>& layout = Nothing(),
        const TMaybe<TString>& requestedSlotName = Nothing()
    );

    TSemanticFrame UpdateFrame(TSemanticFrame frame, const TSemanticFrame& frameUpdate);

    NScenarios::TScenarioRunRequest UnwrapState(NScenarios::TScenarioRunRequest request);
    NScenarios::TScenarioRunResponse UnwrapState(NScenarios::TScenarioRunResponse response);

    const NScenarios::TScenarioResponseBody& GetResponseBody(const TFrameFillerRequest& ffRequest);

} // namespace NFrameFiller
} // namespace NAlice
