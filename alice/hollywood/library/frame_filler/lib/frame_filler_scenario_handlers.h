#pragma once

#include <alice/hollywood/library/frame_filler/proto/scenario_response.pb.h>

#include <alice/hollywood/library/request/request.h>

#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice {
namespace NFrameFiller {

class IFrameFillerScenarioRunHandler {
public:
    virtual ~IFrameFillerScenarioRunHandler() = default;
    virtual TFrameFillerScenarioResponse Do(
        const NHollywood::TScenarioRunRequestWrapper& request,
        TRTLogger& logger
    ) const = 0;
};

class IFrameFillerScenarioCommitHandler {
public:
    virtual ~IFrameFillerScenarioCommitHandler() = default;
    virtual NScenarios::TScenarioCommitResponse Do(
        const NHollywood::TScenarioApplyRequestWrapper& request,
        TRTLogger& logger
    ) const = 0;
};

class IFrameFillerScenarioApplyHandler {
public:
    virtual ~IFrameFillerScenarioApplyHandler() = default;
    virtual NScenarios::TScenarioApplyResponse Do(
        const NHollywood::TScenarioApplyRequestWrapper& request,
        TRTLogger& logger
    ) const = 0;
};

} // namespace NFrameFiller
} // namespace NAlice
