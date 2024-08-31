#include "link_a_remote.h"

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/link_a_remote/nlg/register.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <alice/library/proto/proto.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf LINK_A_REMOTE_NLG = "link_a_remote";

constexpr TStringBuf SETUP_RCU = "setup_rcu";
constexpr TStringBuf SETUP_RCU_REPEATED = "setup_rcu_repeated";
constexpr TStringBuf SETUP_RCU_SUCCESS_NO_TV = "setup_rcu_success_no_tv";
constexpr TStringBuf SETUP_RCU_INACTIVE_TIMEOUT = "setup_rcu_inactive_timeout";
constexpr TStringBuf SETUP_RCU_AUTO = "setup_rcu_auto";
constexpr TStringBuf SETUP_RCU_AUTO_SKIP = "setup_rcu_auto_skip";
constexpr TStringBuf SETUP_RCU_CHECK = "setup_rcu_check";
constexpr TStringBuf SETUP_RCU_CHECK_REPEATED = "setup_rcu_check_repeated";
constexpr TStringBuf SETUP_RCU_CHECK_SUCCESS = "setup_rcu_check_success";
constexpr TStringBuf SETUP_RCU_CHECK_INACTIVE_TIMEOUT = "setup_rcu_check_inactive_timeout";
constexpr TStringBuf SETUP_RCU_ADVANCED = "setup_rcu_advanced";
constexpr TStringBuf SETUP_RCU_ADVANCED_ERROR = "setup_rcu_advanced_error";
constexpr TStringBuf SETUP_RCU_MANUAL = "setup_rcu_manual";
constexpr TStringBuf SETUP_RCU_STOP = "setup_rcu_stop";
constexpr TStringBuf FIND_RCU = "find_rcu";
constexpr TStringBuf REMOTE_FOUND = "remote_found";
constexpr TStringBuf CANT_FIND_REMOTE = "cant_find_remote";
constexpr TStringBuf CANT_FIND_REMOTE_WRONG_SURFACE = "cant_find_remote_wrong_surface";
constexpr TStringBuf CANT_LINK_REMOTE_WRONG_SURFACE = "cant_link_remote_wrong_surface";
constexpr TStringBuf GO_HOME = "go_home";
constexpr TStringBuf TTS_PLAY_PLACEHOLDER = "tts_play_placeholder";
constexpr TStringBuf REQUEST_TECHNICAL_SUPPORT = "request_technical_support";

constexpr TStringBuf LINK_A_REMOTE_FRAME = "personal_assistant.scenarios.quasar.link_a_remote";
constexpr TStringBuf SETUP_RCU_STATUS_FRAME = "personal_assistant.scenarios.quasar.setup_rcu.status";
constexpr TStringBuf SETUP_RCU_AUTO_STATUS_FRAME = "personal_assistant.scenarios.quasar.setup_rcu_auto.status";
constexpr TStringBuf SETUP_RCU_CHECK_STATUS_FRAME = "personal_assistant.scenarios.quasar.setup_rcu_check.status";
constexpr TStringBuf SETUP_RCU_ADVANCED_STATUS_FRAME = "personal_assistant.scenarios.quasar.setup_rcu_advanced.status";
constexpr TStringBuf SETUP_RCU_MANUAL_START_FRAME = "personal_assistant.scenarios.quasar.setup_rcu_manual.start";
constexpr TStringBuf SETUP_RCU_AUTO_START_FRAME = "personal_assistant.scenarios.quasar.setup_rcu_auto.start";
constexpr TStringBuf SETUP_RCU_STOP_FRAME = "personal_assistant.scenarios.quasar.setup_rcu.stop";
constexpr TStringBuf REQUEST_TECNHICAL_SUPPORT_FRAME = "personal_assistant.scenarios.request_technical_support";
constexpr TStringBuf FIND_REMOTE_FRAME = "alice.find_remote";

constexpr TStringBuf STATUS_SLOT = "status";
constexpr TStringBuf TV_MODEL_SLOT = "tv_model";
constexpr TStringBuf LINK_TYPE_SLOT = "link_type";
constexpr TStringBuf LINK_TYPE_IR = "ir";

TMaybe<NAlice::TSetupRcuStatusSlot_EValue> ParseStatusSlot(const TFrame& frame) {
    const auto statusSlot = frame.FindSlot(STATUS_SLOT);
    if (statusSlot == nullptr) {
        return Nothing();
    }
    const auto statusStr = statusSlot->Value.AsString();
    NAlice::TSetupRcuStatusSlot_EValue status;
    if (NAlice::TSetupRcuStatusSlot::EValue_Parse(statusStr, &status)) {
        return status;
    } else {
        return Nothing();
    }
}

class TFrameHandler {
public:
    TFrameHandler(
        const TFrame& frame,
        const TScenarioRunRequestWrapper& request,
        TRTLogger& logger,
        TRunResponseBuilder& builder,
        TResponseBodyBuilder& bodyBuilder
    )
        : Frame(frame)
        , Request(request)
        , Logger(logger)
        , Builder(builder)
        , BodyBuilder(bodyBuilder)
        , NlgData(Logger, Request)
    { }

    virtual ~TFrameHandler() = default;

    virtual void Handle() = 0;

    virtual bool IsRelevant() const = 0;

    virtual TString Name() const = 0;

    TDeviceState_TRcuState_ESetupState GetSetupState() const {
        return Request.BaseRequestProto().GetDeviceState().HasRcuState()
            ? Request.BaseRequestProto().GetDeviceState().GetRcuState().GetSetupState()
            : TDeviceState_TRcuState_ESetupState_None;
    }

protected:
    bool IsSetup() const {
        return GetSetupState() != TDeviceState_TRcuState_ESetupState_None;
    }

    void AddResponseText(const TStringBuf phrase) {
        BodyBuilder.AddRenderedTextWithButtonsAndVoice(LINK_A_REMOTE_NLG, phrase, /* buttons = */ {}, NlgData);
    }

    void AddDirective(const TStringBuf name, const NJson::TJsonValue& payload = {}) {
        BodyBuilder.AddClientActionDirective(TString{name}, TString{name}, payload);
    }

    void AddTtsPlayPlaceholder(const TStringBuf name) {
        BodyBuilder.AddClientActionDirective(TString{TTS_PLAY_PLACEHOLDER}, TString{name});
    }

    void AddNluHint(const TStringBuf frameName, const TVector<TString>& phrases = {}) {
        TFrameNluHint nluHint;
        nluHint.SetFrameName(TString{frameName});
        for (const auto& phrase : phrases) {
            TNluPhrase& nluPhrase = *nluHint.AddInstances();
            nluPhrase.SetLanguage(ELang::L_RUS);
            nluPhrase.SetPhrase(phrase);
        }
        BodyBuilder.AddNluHint(std::move(nluHint));
    }

protected:
    const TFrame Frame;
    const TScenarioRunRequestWrapper& Request;
    TRTLogger& Logger;
    TRunResponseBuilder& Builder;
    TResponseBodyBuilder& BodyBuilder;
    TNlgData NlgData;
};

class TSetupIrHelper : public TFrameHandler {
public:
    using TFrameHandler::TFrameHandler;

    void InitializeIrSetup(const TStringBuf responsePhrase) {
        if (Request.BaseRequestProto().GetInterfaces().GetIsTvPlugged()) {
            AddResponseText(responsePhrase);
            AddDirective(SETUP_RCU_AUTO);
            AddTtsPlayPlaceholder(SETUP_RCU_AUTO);
            AddNluHint(SETUP_RCU_STOP_FRAME);
        } else {
            AddResponseText(SETUP_RCU_SUCCESS_NO_TV);
        }
    }

    void StartManualSetup() {
        AddResponseText(SETUP_RCU_MANUAL);
        AddDirective(SETUP_RCU_MANUAL);
        AddTtsPlayPlaceholder(SETUP_RCU_MANUAL);
        AddNluHint(SETUP_RCU_STOP_FRAME);
    }

    void Handle() override {
    }

    bool IsRelevant() const override {
        return false;
    }

    TString Name() const override {
        return "SetupIrHelper";
    }
};

class TLinkARemoteFrameHandler : public TFrameHandler {
public:
    using TFrameHandler::TFrameHandler;

    void Handle() override {
        if (!Request.BaseRequestProto().GetInterfaces().GetSupportsBluetoothRCU()) {
            return AddResponseText(CANT_LINK_REMOTE_WRONG_SURFACE);
        }
        const auto linktypeSlot = Frame.FindSlot(LINK_TYPE_SLOT);
        if (linktypeSlot && linktypeSlot->Value.AsString() == LINK_TYPE_IR
            && Request.BaseRequestProto().GetDeviceState().GetRcuState().GetIsRcuConnected()
        ) {
            LOG_INFO(Logger) << "Skipping link, starting IR setup";
            TSetupIrHelper(Frame, Request, Logger, Builder, BodyBuilder).InitializeIrSetup(SETUP_RCU_AUTO_SKIP);
            return;
        }
        AddResponseText(SETUP_RCU);
        AddDirective(SETUP_RCU);
        AddTtsPlayPlaceholder(SETUP_RCU_AUTO);
        AddNluHint(SETUP_RCU_STOP_FRAME);
    }

    bool IsRelevant() const override {
        return true;
    }

    TString Name() const override {
        return "LinkARemote";
    }
};

class TSetupRcuAutoStartFrameHandler : public TFrameHandler {
public:
    using TFrameHandler::TFrameHandler;

    void Handle() override {
        AddResponseText(SETUP_RCU_AUTO);
        const auto tvModelSlot = Frame.FindSlot(TV_MODEL_SLOT);
        NJson::TJsonMap tvModel;
        if (tvModelSlot != nullptr) {
            tvModel["tv_model"] = tvModelSlot->Value.AsString();
            LOG_INFO(Logger) << "TV model is " << tvModel["tv_model"];
        }
        AddDirective(SETUP_RCU_AUTO, tvModel);
        AddTtsPlayPlaceholder(SETUP_RCU_AUTO);
        AddNluHint(SETUP_RCU_STOP_FRAME);
    }

    bool IsRelevant() const override {
        return IsSetup();
    }

    TString Name() const override {
        return "SetupRcuAutoStart";
    }
};

class TSetupRcuManualStartFrameHandler : public TFrameHandler {
public:
    using TFrameHandler::TFrameHandler;

    void Handle() override {
        AddResponseText(SETUP_RCU_MANUAL);
        AddDirective(SETUP_RCU_MANUAL);
        AddTtsPlayPlaceholder(SETUP_RCU_MANUAL);
        AddNluHint(SETUP_RCU_STOP_FRAME);
        AddNluHint(REQUEST_TECNHICAL_SUPPORT_FRAME);
    }

    bool IsRelevant() const override {
        return GetSetupState() == TDeviceState_TRcuState_ESetupState_Advanced;
    }

    TString Name() const override {
        return "SetupRcuManualStart";
    }
};

class TSetupRcuStopFrameHandler : public TFrameHandler {
public:
    using TFrameHandler::TFrameHandler;

    void Handle() override {
        AddResponseText(SETUP_RCU_STOP);
        AddDirective(GO_HOME);
    }

    bool IsRelevant() const override {
        return IsSetup() && !Request.IsNewSession();
    }

    TString Name() const override {
        return "TSetupRcuStop";
    }
};

class TRequestTechnicalSupportFrameHandler : public TFrameHandler {
public:
    using TFrameHandler::TFrameHandler;

    void Handle() override {
        AddResponseText(REQUEST_TECHNICAL_SUPPORT);
        AddNluHint(SETUP_RCU_STOP_FRAME);
    }

    bool IsRelevant() const override {
        return GetSetupState() == TDeviceState_TRcuState_ESetupState_Manual;
    }

    TString Name() const override {
        return "RequestTechnicalSupport";
    }
};

class TStatusFrameHandler : public TFrameHandler {
public:
    using TFrameHandler::TFrameHandler;

    void Handle() override {
        const auto status = ParseStatusSlot(Frame);
        if (!status) {
            return;
        }
        if (!StatusHandlers.contains(*status)) {
            LOG_ERROR(Logger) << "No handler for status " << NAlice::TSetupRcuStatusSlot_EValue_Name(*status);
            Builder.SetIrrelevant();
            return;
        }
        (this->*StatusHandlers.at(*status))();
    }

    virtual void OnSuccess() = 0;
    virtual void OnError() = 0;

    virtual void OnInactiveTimeout() {
    }

private:
    using TStatusHandler = void (TStatusFrameHandler::*)();
    const THashMap<NAlice::TSetupRcuStatusSlot_EValue, TStatusHandler> StatusHandlers = {
        { NAlice::TSetupRcuStatusSlot_EValue_Success, &TStatusFrameHandler::OnSuccess },
        { NAlice::TSetupRcuStatusSlot_EValue_InactiveTimeout, &TStatusFrameHandler::OnInactiveTimeout },
        { NAlice::TSetupRcuStatusSlot_EValue_Error, &TStatusFrameHandler::OnError },
    };
};

class TSetupRcuStatusFrameHandler : public TStatusFrameHandler {
public:
    using TStatusFrameHandler::TStatusFrameHandler;

    bool IsRelevant() const override {
        return GetSetupState() == TDeviceState_TRcuState_ESetupState_Link;
    }

    TString Name() const override {
        return "SetupRcuStatus";
    }

    void OnSuccess() override {
        TSetupIrHelper(Frame, Request, Logger, Builder, BodyBuilder).InitializeIrSetup(SETUP_RCU_AUTO);
    }

    void OnError() override {
        AddResponseText(SETUP_RCU_REPEATED);
        AddDirective(SETUP_RCU);
        AddTtsPlayPlaceholder(SETUP_RCU_REPEATED);
        AddNluHint(SETUP_RCU_STOP_FRAME);
    }

    void OnInactiveTimeout() override {
        AddResponseText(SETUP_RCU_INACTIVE_TIMEOUT);
    }
};

class TSetupRcuAutoStatusFrameHandler : public TStatusFrameHandler {
public:
    using TStatusFrameHandler::TStatusFrameHandler;

    bool IsRelevant() const override {
        return GetSetupState() == TDeviceState_TRcuState_ESetupState_Auto;
    }

    TString Name() const override {
        return "SetupRcuAutoStatus";
    }

    void OnSuccess() override {
        AddResponseText(SETUP_RCU_CHECK);
        AddDirective(SETUP_RCU_CHECK);
        AddTtsPlayPlaceholder(SETUP_RCU_CHECK);
        AddNluHint(SETUP_RCU_STOP_FRAME);
        AddNluHint(SETUP_RCU_CHECK_STATUS_FRAME);
    }

    void OnError() override {
        TSetupIrHelper(Frame, Request, Logger, Builder, BodyBuilder).StartManualSetup();
    }
};

class TSetupRcuCheckStatusFrameHandler : public TStatusFrameHandler {
public:
    using TStatusFrameHandler::TStatusFrameHandler;

    bool IsRelevant() const override {
        return GetSetupState() == TDeviceState_TRcuState_ESetupState_Check;
    }

    TString Name() const override {
        return "SetupRcuCheckStatus";
    }

    void OnSuccess() override {
        AddResponseText(SETUP_RCU_CHECK_SUCCESS);
        AddDirective(GO_HOME);
    }

    void OnError() override {
        const auto totalCodesets = Request.BaseRequestProto().GetDeviceState().GetRcuState().GetTotalCodesets();
        if (totalCodesets == 1) {
            TSetupIrHelper(Frame, Request, Logger, Builder, BodyBuilder).StartManualSetup();
            return;
        }
        AddResponseText(SETUP_RCU_ADVANCED);
        AddDirective(SETUP_RCU_ADVANCED);
        AddTtsPlayPlaceholder(SETUP_RCU_ADVANCED);
        AddNluHint(SETUP_RCU_STOP_FRAME);
        AddNluHint(SETUP_RCU_MANUAL_START_FRAME);
    }

    void OnInactiveTimeout() override {
        AddResponseText(SETUP_RCU_CHECK_INACTIVE_TIMEOUT);
        AddNluHint(SETUP_RCU_STOP_FRAME);
        AddNluHint(SETUP_RCU_CHECK_STATUS_FRAME);
    }
};

class TSetupRcuAdvancedStatusFrameHandler : public TStatusFrameHandler {
public:
    using TStatusFrameHandler::TStatusFrameHandler;

    bool IsRelevant() const override {
        return GetSetupState() == TDeviceState_TRcuState_ESetupState_Advanced;
    }

    TString Name() const override {
        return "SetupRcuAdvancedStatus";
    }

    void OnSuccess() override {
        AddResponseText(SETUP_RCU_CHECK_REPEATED);
        AddDirective(SETUP_RCU_CHECK);
        AddTtsPlayPlaceholder(SETUP_RCU_CHECK_REPEATED);
        AddNluHint(SETUP_RCU_STOP_FRAME);
        AddNluHint(SETUP_RCU_CHECK_STATUS_FRAME);
    }

    void OnError() override {
        AddResponseText(SETUP_RCU_ADVANCED_ERROR);
        AddDirective(SETUP_RCU_ADVANCED);
        AddTtsPlayPlaceholder(SETUP_RCU_ADVANCED_ERROR);
        AddNluHint(SETUP_RCU_STOP_FRAME);
        AddNluHint(SETUP_RCU_MANUAL_START_FRAME);
    }
};

class TFindRemoteFrameHandler : public TFrameHandler {
public:
    using TFrameHandler::TFrameHandler;

    void Handle() override {
        if (!Request.BaseRequestProto().GetInterfaces().GetSupportsBluetoothRCU()) {
            return AddResponseText(CANT_FIND_REMOTE_WRONG_SURFACE);
        }
        if (!Request.BaseRequestProto().GetDeviceState().GetRcuState().GetRcuCapabilities().GetCanMakeSounds()) {
            return AddResponseText(CANT_FIND_REMOTE);
        }

        TDirective directive;
        directive.MutableFindRcuDirective()->SetName(TString{FIND_RCU});
        BodyBuilder.AddDirective(std::move(directive));

        AddResponseText(REMOTE_FOUND);
    }

    bool IsRelevant() const override {
        return Request.HasExpFlag(EXP_HW_FIND_REMOTE);
    }

    TString Name() const override {
        return "FindRemote";
    }
};

using TFrameHandlerFactory = THolder<TFrameHandler> (*)(
    const TFrame&,
    const TScenarioRunRequestWrapper&,
    TRTLogger&,
    TRunResponseBuilder&,
    TResponseBodyBuilder&
);

template<class THandler>
THolder<TFrameHandler> Make(
    const TFrame& frame,
    const TScenarioRunRequestWrapper& request,
    TRTLogger& logger,
    TRunResponseBuilder& builder,
    TResponseBodyBuilder& bodyBuilder
) {
    return MakeHolder<THandler>(frame, request, logger, builder, bodyBuilder);
}

const TVector<std::pair<TStringBuf, TFrameHandlerFactory>> FRAME_HANDLER_FACTORIES = {
    // Specific steps of setting up the remote control
    { SETUP_RCU_STATUS_FRAME, Make<TSetupRcuStatusFrameHandler> },
    { SETUP_RCU_AUTO_STATUS_FRAME, Make<TSetupRcuAutoStatusFrameHandler> },
    { SETUP_RCU_CHECK_STATUS_FRAME, Make<TSetupRcuCheckStatusFrameHandler> },
    { SETUP_RCU_ADVANCED_STATUS_FRAME, Make<TSetupRcuAdvancedStatusFrameHandler> },
    { SETUP_RCU_MANUAL_START_FRAME, Make<TSetupRcuManualStartFrameHandler> },
    { SETUP_RCU_AUTO_START_FRAME, Make<TSetupRcuAutoStartFrameHandler> },
    { SETUP_RCU_STOP_FRAME, Make<TSetupRcuStopFrameHandler> },
    { REQUEST_TECNHICAL_SUPPORT_FRAME, Make<TRequestTechnicalSupportFrameHandler> },

    // Initial frames of different intents
    // They have lower priority to not interrupt the RCU setup sequence
    { FIND_REMOTE_FRAME, Make<TFindRemoteFrameHandler> },
    { LINK_A_REMOTE_FRAME, Make<TLinkARemoteFrameHandler> },
};

THolder<TFrameHandler> TryMakeFrameHandler(
    const TScenarioRunRequestWrapper& request,
    TRTLogger& logger,
    TRunResponseBuilder& builder,
    TResponseBodyBuilder& bodyBuilder
) {
    for (const auto& [frameName, frameHandlerFactory] : FRAME_HANDLER_FACTORIES) {
        if (const auto frame = request.Input().FindSemanticFrame(frameName)) {
            auto frameHandler = frameHandlerFactory(
                TFrame::FromProto(*frame),
                request,
                logger,
                builder,
                bodyBuilder
            );
            if (frameHandler->IsRelevant()) {
                return frameHandler;
            }
        }
    }
    return nullptr;
}

} // namespace

void TLinkARemoteRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    if (auto frameHandler = TryMakeFrameHandler(request, ctx.Ctx.Logger(), builder, bodyBuilder)) {
        LOG_INFO(ctx.Ctx.Logger()) << "Chosen " << frameHandler->Name() << " frame handler";
        LOG_INFO(ctx.Ctx.Logger()) << "Setup state is " << TDeviceState_TRcuState_ESetupState_Name(frameHandler->GetSetupState());

        frameHandler->Handle();
    } else {
        LOG_ERROR(ctx.Ctx.Logger()) << "No valid semantic frame";
        builder.SetIrrelevant();
    }

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

REGISTER_SCENARIO("link_a_remote",
AddHandle<TLinkARemoteRunHandle>()
.SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NLinkARemote::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
