#pragma once

#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint.pb.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <alice/hollywood/library/frame/frame.h>

#include <functional>
#include <memory>

namespace NAlice::NHollywood::NVoiceprint {

template <class TState>
class TRunStateMachineContextBase {
public:
    TRTLogger& Logger() {
        return VoiceprintCtx_.Logger;
    }

    const TScenarioRunRequestWrapper& GetRequest() const {
        return VoiceprintCtx_.Request;
    }

    const TBlackBoxUserInfo* UserInfo() const {
        return VoiceprintCtx_.UserInfo;
    }

    TVoiceprintState& ScenarioStateProto() {
        return VoiceprintCtx_.ScenarioStateProto;
    }

    bool IsValidRegion() const {
        return IsValidRegionImpl(VoiceprintCtx_.Ctx, VoiceprintCtx_.Request);
    }

protected:
    using TRunHandleResult = typename TState::TRunHandleResult;

    explicit TRunStateMachineContextBase(TVoiceprintHandleContext& voiceprintCtx)
        : VoiceprintCtx_(voiceprintCtx)
    {}

    bool HandleFrame(const TFrame& frame, TRunResponseBuilder& builder,
                     std::function<TRunHandleResult(const TFrame&, TRunResponseBuilder&)> handler)
    {
        TRunHandleResult handleResult;
        do {
            LogStatus(frame.Name());
            handleResult = handler(frame, builder);
            if (handleResult.NewState) {
                State_ = std::move(handleResult.NewState);
            }
        } while (handleResult.ContinueProcessing);
        return !handleResult.IsIrrelevant;
    }

    void LogStatus(const TString& frameName) {
        Y_ENSURE(State_);
        LOG_INFO(Logger()) << "Handling " << frameName << " frame in " << State_->Name() << " state...";
    }

protected:
    TVoiceprintHandleContext& VoiceprintCtx_;
    std::unique_ptr<TState> State_{nullptr};
};

} // namespace NAlice::NHollywood::NVoiceprint

#define DEFINE_CONTEXT_HANDLE_METHOD(contextClassName, handleMethodName) \
bool contextClassName::handleMethodName(const TFrame& frame, TRunResponseBuilder& builder) { \
    return HandleFrame(frame, builder, [this](const TFrame& frame, TRunResponseBuilder& builder) { \
        Y_ENSURE(State_); \
        return State_->handleMethodName(frame, builder); \
    }); \
}
