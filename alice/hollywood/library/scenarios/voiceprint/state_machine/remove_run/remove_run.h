#pragma once

#include <alice/hollywood/library/scenarios/voiceprint/state_machine/context.h>
#include <alice/hollywood/library/scenarios/voiceprint/state_machine/handle_result.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <memory>

namespace NAlice::NHollywood::NVoiceprint {

class TRemoveRunContext;

namespace NImpl {

class TRemoveRunStateBase {
public:
    using TRunHandleResult = THandleResult<TRemoveRunStateBase>;

public:
    explicit TRemoveRunStateBase(TRemoveRunContext* context);

    virtual ~TRemoveRunStateBase() = default;

    virtual TRunHandleResult HandleRemoveFrame(const TFrame& frame, TRunResponseBuilder& builder);
    virtual TRunHandleResult HandleCancelFrame(const TFrame& frame, TRunResponseBuilder& builder);

    virtual TString Name() const {
        return "Remove Unknown";
    }

protected:
    TRunHandleResult ReportUnexpectedFrame(const TFrame& frame, TRunResponseBuilder& builder);
    TRunHandleResult ReportUnknownStage(const TFrame& frame, TRunResponseBuilder& builder);

    void RenderResponse(
        TRunResponseBuilder& builder,
        const TFrame& frame,
        TStringBuf renderTemplate,
        bool expectsAnswer
    );

    void ClearRemoveState();

protected:
    TRemoveRunContext* Context_;
    TVoiceprintRemoveState& RemoveState_;
    TNlgData NlgData_;
    NJson::TJsonArray RenderSlots_;
};

class TRemoveNotStartedState : public TRemoveRunStateBase {
public:
    explicit TRemoveNotStartedState(TRemoveRunContext* context);

    TRunHandleResult HandleRemoveFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "Remove NotStarted";
    }
};

class TRemoveClientBiometryInitState : public TRemoveRunStateBase {
public:
    enum class EPrepareResult {
        Success /* "success" */,
        NoMatch /* "no_match" */,
        ServerError /* "server_error" */,
    };

public:
    explicit TRemoveClientBiometryInitState(TRemoveRunContext* context);

    TRunHandleResult HandleRemoveFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "Remove ClientBiometryInit";
    }

private:
    EPrepareResult PrepareRemove();
};

class TRemoveServerBiometryInitState : public TRemoveRunStateBase {
public:
    explicit TRemoveServerBiometryInitState(TRemoveRunContext* context);

    TRunHandleResult HandleRemoveFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "Remove ServerBiometryInit";
    }

private:
    TStringBuf PrepareRemove();
};

class TRemoveWaitConfirmState : public TRemoveRunStateBase {
public:
    explicit TRemoveWaitConfirmState(TRemoveRunContext* context);

    TRunHandleResult HandleRemoveFrame(const TFrame& frame, TRunResponseBuilder& builder) override;
    TRunHandleResult HandleCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "Remove WaitConfirm";
    }
};

} // namespace NImpl

class TRemoveRunContext : public TRunStateMachineContextBase<NImpl::TRemoveRunStateBase> {
public:
    static std::unique_ptr<TRemoveRunContext> MakeFrom(TVoiceprintHandleContext& voiceprintCtx);

    bool HandleRemoveFrame(const TFrame& frame, TRunResponseBuilder& builder);
    bool HandleCancelFrame(const TFrame& frame, TRunResponseBuilder& builder);

private:
    explicit TRemoveRunContext(TVoiceprintHandleContext& voiceprintCtx);
};

} // namespace NAlice::NHollywood::NVoiceprint
