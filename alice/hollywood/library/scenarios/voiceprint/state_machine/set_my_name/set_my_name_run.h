#pragma once

#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint.pb.h>
#include <alice/hollywood/library/scenarios/voiceprint/state_machine/context.h>
#include <alice/hollywood/library/scenarios/voiceprint/state_machine/handle_result.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <memory>

namespace NAlice::NHollywood::NVoiceprint {

class TSetMyNameRunContext;

namespace NImpl {

class TSetMyNameRunStateBase {
public:
    using TRunHandleResult = THandleResult<TSetMyNameRunStateBase>;

public:
    explicit TSetMyNameRunStateBase(TSetMyNameRunContext* context);

    virtual ~TSetMyNameRunStateBase() = default;

    virtual TRunHandleResult HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder);

    virtual TString Name() const {
        return "SetMyName Unknown";
    }

protected:
    void RenderResponse(TRunResponseBuilder& builder, const TFrame& frame, bool isServerError);

    TRunHandleResult Irrelevant() const;
    TRunHandleResult ReportUnknownState(const TFrame& frame, TRunResponseBuilder& builder);

    void ClearSetMyNameState();

protected:
    TSetMyNameRunContext* Context_;
    TVoiceprintSetMyNameState& SetMyNameState_;
    NJson::TJsonArray RenderSlots_;
};

class TSetMyNameBiometryDispatchState : public TSetMyNameRunStateBase {
public:
    explicit TSetMyNameBiometryDispatchState(TSetMyNameRunContext* context);

    TRunHandleResult HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "SetMyName BiometryDispatch";
    }
};

class TSetMyNameClientBiometryState : public TSetMyNameRunStateBase {
public:
    TSetMyNameClientBiometryState(TSetMyNameRunContext* context);

    TRunHandleResult HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "SetMyName ClientBiometry";
    }
};

class TSetMyNameServerBiometryState : public TSetMyNameRunStateBase {
public:
    TSetMyNameServerBiometryState(TSetMyNameRunContext* context);

    TRunHandleResult HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) override;

    TString Name() const override {
        return "SetMyName ServerBiometry";
    }
};

} // namespace NImpl

class TSetMyNameRunContext : public TRunStateMachineContextBase<NImpl::TSetMyNameRunStateBase> {
public:
    static std::unique_ptr<TSetMyNameRunContext> MakeFrom(TVoiceprintHandleContext& voiceprintCtx);

    bool HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder);

private:
    explicit TSetMyNameRunContext(TVoiceprintHandleContext& voiceprintCtx);
};

} // namespace NAlice::NHollywood::NVoiceprint
