#pragma once

#include <alice/hollywood/library/scenarios/voiceprint/state_machine/context.h>
#include <alice/hollywood/library/scenarios/voiceprint/state_machine/handle_result.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <memory>

namespace NAlice::NHollywood::NVoiceprint {

class TEnrollmentRunContext;

namespace NImpl {

class TEnrollmentRunStateBase {
public:
    using TRunHandleResult = THandleResult<TEnrollmentRunStateBase>;
    using EnrollmentDirectiveFunc = void(*)(TRTLogger&, const TVoiceprintEnrollState&, TResponseBodyBuilder&);

public:
    explicit TEnrollmentRunStateBase(TEnrollmentRunContext* context);

    virtual ~TEnrollmentRunStateBase() = default;

    virtual TRunHandleResult HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& builder);
    virtual TRunHandleResult HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder);
    virtual TRunHandleResult HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder);
    virtual TRunHandleResult HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder);
    virtual TRunHandleResult HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder);
    virtual TRunHandleResult HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder);
    virtual TRunHandleResult HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder);
    virtual TRunHandleResult HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder);
    virtual TRunHandleResult HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder);

    virtual TString Name() const {
        return "Unknown";
    }

protected:
    void RenderResponse(
        TRunResponseBuilder& builder,
        const TFrame& frame,
        TStringBuf renderTemplate,
        const TString& extraDirective,
        EnrollmentDirectiveFunc directiveFunc,
        bool isReady
    );

    TRunHandleResult ReportUnexpectedFrame(const TFrame& frame, TRunResponseBuilder& builder);
    TRunHandleResult CheckAndRepeat(const TFrame& frame, TRunResponseBuilder& builder, bool isServerRepeat);

    bool CheckSwear(const TPtrWrapper<TSlot>& userNameSlot);
    bool CheckPrerequisites(bool isChangeNameRequest = false,
                            bool isNewGuestFrame = false);

    void CancelActiveEnrollment(const TFrame& frame, TRunResponseBuilder& builder);

    void RenderRepeat(const TFrame& frame, TRunResponseBuilder& builder,
                      TStringBuf forceFrameName, TStringBuf renderTemplate, bool isReady);

    void ClearEnrollState();

private:
    TRunHandleResult ReportUnknownStage(const TFrame& frame, TRunResponseBuilder& builder);

protected:
    TEnrollmentRunContext* Context_;
    TVoiceprintEnrollState& EnrollState_;
    TNlgData NlgData_;
    NJson::TJsonArray RenderSlots_;
};

class TEnrollmentNotStartedState : public TEnrollmentRunStateBase {
public:
    explicit TEnrollmentNotStartedState(TEnrollmentRunContext* context);

    TRunHandleResult HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "NotStarted";
    }

private:
    TRunHandleResult HandleFrameWithUserName(const TFrame& frame,
                                             TRunResponseBuilder& builder,
                                             bool isChangeNameRequest,
                                             const TPtrWrapper<TSlot>& userNameSlot);

    TRunHandleResult HandleFrameWithoutUserName(const TFrame& frame,
                                                TRunResponseBuilder& builder,
                                                bool isChangeNameRequest);

    TStringBuf PrepareEnroll(bool isChangeNameRequest, bool isNewGuestFrame);
};

class TEnrollmentIntroState : public TEnrollmentRunStateBase {
public:
    explicit TEnrollmentIntroState(TEnrollmentRunContext* context);

    TRunHandleResult HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "Intro";
    }

private:
    TRunHandleResult HandleFrameWithUserName(const TFrame& frame, TRunResponseBuilder& builder, bool isChangeNameRequest);
    TRunHandleResult TransitToWaitState(const TFrame& frame, TRunResponseBuilder& builder);
};

class TEnrollmentWaitUsernameState : public TEnrollmentRunStateBase {
public:
    explicit TEnrollmentWaitUsernameState(TEnrollmentRunContext* context);

    TRunHandleResult HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    
    TString Name() const override {
        return "WaitUsername";
    }

private:
    TRunHandleResult HandleFrameWithUserName(const TFrame& frame,
                                             TRunResponseBuilder& builder,
                                             bool isChangeNameRequest,
                                             const TPtrWrapper<TSlot>& userNameSlot);
};

class TEnrollmentWaitReadyState : public TEnrollmentRunStateBase {
public:
    explicit TEnrollmentWaitReadyState(TEnrollmentRunContext* context);

    TRunHandleResult HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "WaitReady";
    }

private:
    TRunHandleResult HandleFrameWithUserName(const TFrame& frame,
                                             TRunResponseBuilder& builder,
                                             bool isChangeNameRequest,
                                             const TPtrWrapper<TSlot>& userNameSlot);
};

class TEnrollmentCollectState : public TEnrollmentRunStateBase {
public:
    explicit TEnrollmentCollectState(TEnrollmentRunContext* context);

    TRunHandleResult HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "Collect";
    }

private:
    TRunHandleResult CollectPhrase(const TFrame& frame, TRunResponseBuilder& builder, bool isRepeat);
};

class TEnrollmentCompleteState : public TEnrollmentRunStateBase {
public:
    explicit TEnrollmentCompleteState(TEnrollmentRunContext* context);

    TRunHandleResult HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "Complete";
    }

private:
    TRunHandleResult FallBackToNotStarted(const TFrame& frame); 
};

} // namespace NImpl

class TEnrollmentRunContext : public TRunStateMachineContextBase<NImpl::TEnrollmentRunStateBase> {
public:
    static std::unique_ptr<TEnrollmentRunContext> MakeFrom(TVoiceprintHandleContext& voiceprintCtx);

    bool HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& builder);
    bool HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder);
    bool HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder);
    bool HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder);
    bool HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder);
    bool HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder);
    bool HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder);
    bool HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder);
    bool HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder);

    const TFrame* FindFrame(TStringBuf frameName);
    const TSemanticFrame* FindSemanticFrame(TStringBuf frameName);

private:
    explicit TEnrollmentRunContext(TVoiceprintHandleContext& voiceprintCtx);
};

} // namespace NAlice::NHollywood::NVoiceprint
