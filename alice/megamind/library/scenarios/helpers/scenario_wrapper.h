#pragma once

#include "scenario_ref.h"
#include "session_view.h"

#include <alice/megamind/library/scenarios/helpers/interface/scenario_wrapper.h>

#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/scenarios/interface/protocol_scenario.h>
#include <alice/megamind/library/scenarios/protocol/metrics.h>

#include <alice/megamind/library/apphost_request/protos/scenarios_text_response.pb.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/models/directives/callback_directive_model.h>
#include <alice/megamind/library/proactivity/common/common.h>
#include <alice/megamind/library/request/event/server_action_event.h>
#include <alice/megamind/library/response/utils.h>
#include <alice/megamind/library/session/protos/session.pb.h>

#include <alice/megamind/protos/modifiers/modifiers.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/library/metrics/histogram.h>
#include <alice/library/metrics/names.h>

#include <library/cpp/http/misc/httpcodes.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>

#include <functional>

namespace NAlice {

namespace NImpl {

NMegamind::TGetNextCallbackDirectiveModel
CreateGetNextDirectiveFromStackEngine(const NMegamind::IStackEngine& stackEngine,
                                      const NMegamind::TSerializerMeta& serializerMeta, TRTLogger& logger);

} // namespace NImpl

template<typename TScenarioType>
class TScenarioWrapperBase : public IScenarioWrapper {
public:
    TScenarioWrapperBase(const TScenarioType& scenario,
                         const IContext& ctx,
                         const TSemanticFrames& semanticFrames,
                         const NMegamind::IGuidGenerator& guidGenerator,
                         bool restoreAllFromSession)
        : Scenario(scenario)
        , SemanticFrames(semanticFrames)
        , GuidGenerator(guidGenerator)
    {
        const auto* session = ctx.Session();
        if (session) {
            if (restoreAllFromSession) {
                State = RestoreScenarioState(*session);
                if (session->GetPreviousScenarioName() == scenario.GetName()) {
                    if (const auto analyticsInfo = session->GetMegamindAnalyticsInfo()) {
                        MegamindAnalyticsInfo = *analyticsInfo;
                    }
                    if (const auto storage = session->GetQualityStorage()) {
                        QualityStorage = *storage;
                    }
                }
            }
            if (const auto modifiersStorage = session->GetModifiersStorage()) {
                ModifiersStorage = *modifiersStorage;
            }
        }
    }

    // IScenarioRef overrides:
    const TScenario& GetScenario() const override final {
        return Scenario;
    }

    // IScenarioWrapper overrides:
    const TSemanticFrames& GetSemanticFrames() const override {
        return SemanticFrames;
    }

    NMegamind::TAnalyticsInfoBuilder& GetAnalyticsInfo() override {
        return AnalyticsInfoBuilder;
    }

    const NMegamind::TAnalyticsInfoBuilder& GetAnalyticsInfo() const override {
        return AnalyticsInfoBuilder;
    }

    NMegamind::TUserInfoBuilder& GetUserInfo() override final {
        return UserInfoBuilder;
    }

    const NMegamind::TUserInfoBuilder& GetUserInfo() const override final {
        return UserInfoBuilder;
    }

    NMegamind::TMegamindAnalyticsInfo& GetMegamindAnalyticsInfo() override final {
        return MegamindAnalyticsInfo;
    }

    const TQualityStorage& GetQualityStorage() const override final {
        return QualityStorage;
    }

    TQualityStorage& GetQualityStorage() override final {
        return QualityStorage;
    }

    const NMegamind::TModifiersStorage& GetModifiersStorage() const override final {
        return ModifiersStorage;
    }

    NMegamind::TModifiersStorage& GetModifiersStorage() override final {
        return ModifiersStorage;
    }

    TScenarioEnv GetEnv(const TRequest& request, const IContext& ctx) override final {
        TScenarioEnv env{ctx, request, SemanticFrames, State, *Features.MutableScenarioFeatures(),
                         AnalyticsInfoBuilder, UserInfoBuilder};
        return env;
    }

    TLightScenarioEnv GetApplyEnv(const TRequest& request, const IContext& ctx) override final {
        TLightScenarioEnv env{ctx, request, SemanticFrames, State, AnalyticsInfoBuilder, UserInfoBuilder};
        return env;
    }

    TStatus Init(const TRequest& request, const IContext& ctx, NMegamind::IDataSources& dataSources) override final {
        Error = Success();

        try {
            State.Clear();

            if (ctx.Session()) {
                State = RestoreScenarioState(*ctx.Session());
            }

            Error = InitImpl(request, ctx, dataSources);
        } catch (...) {
            Error = TError(TError::EType::Logic) << "EXCEPTION: " << CurrentExceptionMessage();
        }
        return Error;
    }

    TStatus Ask(const TRequest& request, const IContext& ctx, TScenarioResponse& response) override final {
        if (Error.Defined()) {
            return *Error;
        }

        try {
            if (Error = AskImpl(request, ctx, response); !Error.Defined()) {
                response.SetFeatures(Features);
            }
        } catch (...) {
            Error = TError(TError::EType::Logic) << "EXCEPTION: " << CurrentExceptionMessage();
        }
        return Error;
    }

    TStatus Finalize(const TRequest& request, const IContext& ctx, TScenarioResponse& response) override final {
        if (Error.Defined()) {
            return *Error;
        }

        try {
            if (Error = FinalizeImpl(request, ctx, response); !Error.Defined()) {
                response.SetFeatures(Features);
            }
        } catch (...) {
            Error = TError(TError::EType::Logic) << "EXCEPTION: " << CurrentExceptionMessage();
        }
        return Error;
    }

    TStatus StartHeavyContinue(const TRequest& request, const IContext& ctx) override final {
        if (Error.Defined()) {
            return *Error;
        }

        try {
            Error = StartHeavyContinueImpl(request, ctx);
        } catch (...) {
            Error = TError(TError::EType::Logic) << "EXCEPTION: " << CurrentExceptionMessage();
        }
        return Error;
    }

    TStatus FinishContinue(const TRequest& request, const IContext& ctx, TScenarioResponse& response) override final {
        if (Error.Defined()) {
            return *Error;
        }

        try {
            Error = FinishContinueImpl(request, ctx, response);
        } catch (...) {
            Error = TError(TError::EType::Logic) << "EXCEPTION: " << CurrentExceptionMessage();
        }
        return Error;
    }

    EApplicability SetReasonWhenNonApplicable(const TRequest& /* request */, const IContext& /* ctx */,
                                              TScenarioResponse& /* response */) override {
        return EApplicability::Applicable;
    }

    TErrorOr<EApplyResult> StartApply(const TRequest& request, const IContext& ctx, TScenarioResponse& response,
                                      const NMegamind::TMegamindAnalyticsInfo& megamindAnalyticsInfo,
                                      const TQualityStorage& storage, const TProactivityAnswer& proactivity) override final {
        if (Error.Defined()) {
            return *Error;
        }

        try {
            State.Clear();

            if (ctx.Session()) {
                State = RestoreScenarioState(*ctx.Session());
            }

            auto result = StartApplyImpl(request, ctx, response, megamindAnalyticsInfo, storage, proactivity);

            if (const auto* error = result.Error()) {
                Error = *error;
            }
            return result;
        } catch (...) {
            Error = TError(TError::EType::Logic) << "EXCEPTION: " << CurrentExceptionMessage();
            return *Error;
        }
    }

    TErrorOr<EApplyResult> FinishApply(const TRequest& request, const IContext& ctx, TScenarioResponse& response) override final {
        if (Error.Defined()) {
            return *Error;
        }

        try {
            auto result = FinishApplyImpl(request, ctx, response);

            if (const auto* error = result.Error()) {
                Error = *error;
            }
            return result;
        } catch (...) {
            Error = TError(TError::EType::Logic) << "EXCEPTION: " << CurrentExceptionMessage();
            return *Error;
        }
    }

    bool IsSuccess() const override {
        return !Error.Defined();
    }

    // Own methods
    const TScenarioType& GetConcreteScenario() const {
        return Scenario;
    }

    bool ShouldBecomeActiveScenario() const override {
        return false;
    }

    std::once_flag& GetAskFlag() override {
        return AskFlag;
    }

    std::once_flag& GetContinueFlag() override {
        return ContinueFlag;
    }

    NMegamindAppHost::TScenarioProto GetScenarioProto() const override {
        NMegamindAppHost::TScenarioProto proto;
        proto.SetName(GetScenario().GetName());
        for (const auto& frame : GetSemanticFrames()) {
            *proto.AddSemanticFrame() = frame;
        }
        return proto;
    }

    virtual bool IsApplyNeededOnWarmUpRequestWithSemanticFrame() const override {
        return false;
    }

protected:
    void SetEOUExpected(const IContext& ctx, const TRequest& requestModel, TScenarioResponse& response) {
        // NOTE(the0): Mind the reference! It's here on purpose.
        TResponseBuilder& builder = response.ForceBuilder(ctx.SpeechKitRequest(), requestModel, GuidGenerator);
        builder.Reset(ctx.SpeechKitRequest(), requestModel, GuidGenerator, this->GetScenario().GetName());
        builder.ShouldListen(true).AddError("eou_expected", "Expected end of utterance");
        response.SetHttpCode(HTTP_PARTIAL_CONTENT);
    }

    THolder<NScenarios::IAnalyticsInfoBuilder> CreateScenarioAnalyticsInfoBuilder() {
        return AnalyticsInfoBuilder.CreateScenarioAnalyticsInfoBuilder();
    }

    virtual TStatus InitImpl(const TRequest& /* request */, const IContext& /* ctx */, NMegamind::IDataSources& /* dataSources */) {
        return Success();
    }

    virtual TStatus AskImpl(const TRequest& request, const IContext& ctx, TScenarioResponse& response) = 0;

    virtual TStatus FinalizeImpl(const TRequest& request, const IContext& ctx, TScenarioResponse& response) = 0;

    virtual TErrorOr<EApplyResult> StartApplyImpl(const TRequest& /* request */, const IContext& /* ctx */, TScenarioResponse& /* response */,
                                                  const NMegamind::TMegamindAnalyticsInfo& /* megamindAnalyticsInfo */,
                                                  const TQualityStorage& /* storage */,
                                                  const TProactivityAnswer& /* proactivity */) {
        return EApplyResult::Called;
    }

    virtual TErrorOr<EApplyResult> FinishApplyImpl(const TRequest& /* request */, const IContext& /* ctx */, TScenarioResponse& /* response */) {
        return EApplyResult::Called;
    }

    virtual TStatus StartHeavyContinueImpl(const TRequest& /* request */, const IContext& /* ctx */) {
        return Success();
    }

    virtual TStatus FinishContinueImpl(const TRequest& /* request */, const IContext& /* ctx */,
                                       TScenarioResponse& /* response */) {
        return Success();
    }

private:
    TState RestoreScenarioState(const ISession& session) {
        const auto& actualName = Scenario.GetName();
        return session.GetScenarioSession(actualName).GetState();
    }

protected:

    const TState& GetState() const {
        return State;
    }

    const TScenarioType& Scenario;
    TSemanticFrames SemanticFrames;
    const NMegamind::IGuidGenerator& GuidGenerator;
    TState State;
    TFeatures Features;
    TQualityStorage QualityStorage;
    NMegamind::TAnalyticsInfoBuilder AnalyticsInfoBuilder;
    NMegamind::TUserInfoBuilder UserInfoBuilder;
    NMegamind::TMegamindAnalyticsInfo MegamindAnalyticsInfo;
    NMegamind::TModifiersStorage ModifiersStorage;

    std::once_flag AskFlag;
    std::once_flag ContinueFlag;

    TStatus Error;
};

template <typename TScenarioType, bool SaveProto>
class TEffectfulScenarioWrapperBase : public TScenarioWrapperBase<TScenarioType> {
public:
    TEffectfulScenarioWrapperBase(const TScenarioType& scenario,
                                  const IContext& ctx,
                                  const IScenarioWrapper::TSemanticFrames& semanticFrames,
                                  const NMegamind::IGuidGenerator& guidGenerator,
                                  EDeferredApplyMode deferApply,
                                  bool restoreAllFromSession)
        : TScenarioWrapperBase<TScenarioType>(scenario, ctx, semanticFrames, guidGenerator,
                                              restoreAllFromSession)
        , DeferApplyMode(deferApply)
    {
    }

    virtual EDeferredApplyMode GetDeferredApplyMode() const override {
        return DeferApplyMode;
    }

    // IScenarioWrapper overrides:
    EApplicability SetReasonWhenNonApplicable(const TRequest& request, const IContext& ctx,
                                              TScenarioResponse& response) override {
        if (Mode.Defined()) {
            switch (*Mode) {
                case TScenario::EApplyMode::Skip:
                    [[fallthrough]];
                case TScenario::EApplyMode::Continue:
                    return EApplicability::Inapplicable;
                case TScenario::EApplyMode::Call:
                    break;
            }
        }

        // |this| is used here because this is a
        // template-derived-class, and GetEnv() is a non-dependent
        // name.  Curious reader may learn more about the problem
        // here:
        // https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members
        auto env = this->GetApplyEnv(request, ctx);

        // NOTE: Better safe than sorry - we assume that
        // end-of-utterance is false if it's missed in the speechkit
        // request.

        if (!env.Ctx.SpeechKitRequest().IsEOU().GetOrElse(false)) {
            switch (DeferApplyMode) {
                case EDeferredApplyMode::DontDeferApply:
                    // We have to apply immediately. Our only way to find if we are allowed to do this
                    // is to check EOU presense.
                    this->SetEOUExpected(ctx, request, response);
                    return EApplicability::Inapplicable;

                case EDeferredApplyMode::DeferredCall:
                    // We are called by Uniproxy to apply side effects.
                    // NOTE (a-sidorin@): For now, end-of-utterance is empty for uniproxy apply request.
                    // Skip the check until Uniproxy fix is released.
                    LOG_WARN(env.Ctx.Logger()) << "Got defer_apply request without end_of_utterance set!";
                    break;

                case EDeferredApplyMode::DeferApply:
                    // For deferred apply, end_of_utterance doesn't make sense.
                    // It's up to uniproxy to decide if it is needed to call our side effect application later.
                    break;

                case EDeferredApplyMode::WarmUp:
                    // For warming up apply, as in DeferApply, end_of_utterance doesn't make sense:
                    // we simply don't call apply on warm up.
                    break;
            }
        }

        return EApplicability::Applicable;
    }

    TStatus RestoreInit(NMegamind::TItemProxyAdapter& itemAdapter) override {
        TAppHostItemNames itemNames{TScenarioWrapperBase<TScenarioType>::GetScenario().GetName(),
                                    /* itemRequestSuffix= */ {}, /* itemResponseSuffix= */ {}};
        auto runInputOrError = itemAdapter.GetFromContext<NScenarios::TInput>(itemNames.RequestInput);
        if (runInputOrError.Error()) {
            return std::move(*runInputOrError.Error());
        }
        RunInput.ConstructInPlace(std::move(runInputOrError.Value()));
        return Success();
    }

    bool IsApplyNeededOnWarmUpRequestWithSemanticFrame() const override {
        return IsApplyNeededOnWarmUpRequestWithSemanticFrame_;
    }

protected:
    void WaitAllRequests(const TStringBuf requestType) {
        this->Responses.clear();
        for (auto&& [tag, handle] : this->Requests) {
            auto response = handle->Wait();
            if (requestType == TAG_RUN) {
                this->CreateScenarioAnalyticsInfoBuilder()->AddSourceResponseDuration(
                    TString{requestType}, TString::Join(this->GetConcreteScenario().GetName(), '-', tag), response->Duration);
            }
            this->Responses[tag] = response;
        }
    }

    TSessionProto::TScenarioSession GetCurrentScenarioSession(const TLightScenarioEnv& env) const {
        if (const auto* session = env.Ctx.Session()) {
            return session->GetScenarioSession(this->GetScenario().GetName());
        }
        return {};
    }

    THolder<ISession> GetUpdatedSession(TLightScenarioEnv& env,
                                        const TRequest& request,
                                        const TMaybe<TResponseBuilderProto>& builderProto,
                                        const NMegamind::TMegamindAnalyticsInfo& megamindAnalyticsInfo,
                                        const TQualityStorage& storage,
                                        const TString& intent,
                                        const TProactivityAnswer& proactivity) const {
        THolder<ISessionBuilder> builder;
        if (const auto* session = env.Ctx.Session()) {
            // TODO(g-kostin): split session update and old session
            builder = session->GetUpdater();
        } else {
            builder = MakeSessionBuilder();
        }

        auto scenarioSession = GetCurrentScenarioSession(env);
        scenarioSession.MutableState()->CopyFrom(env.State);

        // TODO(g-kostin): move previous scenario name for apply to another field
        return builder->SetPreviousScenarioName(this->GetScenario().GetName())
            .SetScenarioSession(this->GetScenario().GetName(), scenarioSession)
            .SetScenarioResponseBuilder(builderProto)
            .SetProtocolInfo(ProtocolInfo)
            .SetMegamindAnalyticsInfo(megamindAnalyticsInfo)
            .SetQualityStorage(storage)
            .SetIntentName(intent)
            .SetProactivityRecommendations(proactivity)
            .SetStackEngineCore(request.GetStackEngineCore(), /* invalidate= */ false)
            .SetInput(RunInput)
            .Build();
    }

    TMaybe<TString> GetCurrentSessionSerialized(TLightScenarioEnv& env,
                                                const TRequest& request,
                                                const TMaybe<TResponseBuilderProto>& builderProto,
                                                const NMegamind::TMegamindAnalyticsInfo& megamindAnalyticsInfo,
                                                const TQualityStorage& storage,
                                                const TString& intent,
                                                const TProactivityAnswer& proactivity) {
        if (const auto updatedSession = GetUpdatedSession(env, request, builderProto,
            megamindAnalyticsInfo, storage, intent, proactivity))
        {
            return updatedSession->Serialize();
        }
        return Nothing();
    }

    TStatus DeferApply(TLightScenarioEnv& env, const TRequest& request,
                       TResponseBuilder& builder,
                       const TMaybe<TResponseBuilderProto>& builderProto,
                       const NMegamind::TMegamindAnalyticsInfo& megamindAnalyticsInfo,
                       const TQualityStorage& storage, const TString& intent,
                       const TProactivityAnswer& proactivity) {
        const auto& skr = env.Ctx.SpeechKitRequest();
        builder.Reset(skr, request, this->GuidGenerator, this->GetScenario().GetName());

        const auto session = GetCurrentSessionSerialized(env, request, builderProto, megamindAnalyticsInfo,
                                                         storage, intent, proactivity);
        if (!session.Defined()) {
            return TError() << "Failed to serialize session for deferred application of scenario "
                            << this->GetScenario().GetName();
        }

        // TODO(alkapov, g-kostin): get session from env.Ctx.Request().GetDialogId().GetOrElse("");
        //  Seems it's better to add DialogId as method to IContext.
        const auto dialogId = skr->GetHeader().GetDialogId();
        builder.SetSession(dialogId, *session);
        builder.AddDirective(NMegamind::TDeferApplyDirectiveModel(*session));

        return Success();
    }

    TStatus WarmUpApply(TLightScenarioEnv& env, TResponseBuilder& builder, const TRequest& request) {
        if (const auto* session = env.Ctx.Session()) {
            const auto dialogId = env.Ctx.SpeechKitRequest()->GetHeader().GetDialogId();
            builder.SetSession(dialogId, session->Serialize());
        }
        if (const auto* serverActionEvent = request.GetEvent().AsServerActionEvent()) {
            // TODO(g-kostin): MEGAMIND-1893. add special case for TypedSemanticFrame
            NMegamind::TCallbackDirectiveModel directive{
                serverActionEvent->GetName(), /* ignoreAnswer= */ false, serverActionEvent->GetPayload(),
                /* isLedSilent= */ true
            };
            builder.AddDirective(directive);
        } else if (request.GetRequestSource() == NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext) {
            constexpr TStringBuf errorPrefix = "Failed to warm up apply for GetNext: ";
            const auto* session = env.Ctx.Session();
            if (!session) {
                return TError(TError::EType::Logic) << errorPrefix << "Malformed session.";
            }
            NMegamind::TStackEngine originalStackEngine{session->GetStackEngineCore()};
            if (originalStackEngine.IsEmpty()) {
                return TError(TError::EType::Logic) << errorPrefix << "Empty stack.";
            }

            builder.AddDirective(NImpl::CreateGetNextDirectiveFromStackEngine(
                originalStackEngine, builder.GetSerializerMeta(), env.Ctx.Logger()));

            IsApplyNeededOnWarmUpRequestWithSemanticFrame_ = true;
        } else {
            LOG_WARN(env.Ctx.Logger()) << "Failed to warm up apply for " <<
                EEventType_Name(request.GetEvent().SpeechKitEvent().GetType()) << " event. Only server_action and get_next is supported.";
        }

        return Success();
    }

protected:
    const EDeferredApplyMode DeferApplyMode;
    TMaybe<TScenario::EApplyMode> Mode;

    // Moved from protocol scenario wrapper to eliminate two session updaters.
    // Serializable wrapper state to share between run and apply.
    TSessionProto::TProtocolInfo ProtocolInfo;

    TMaybe<NScenarios::TInput> RunInput;
    bool IsApplyNeededOnWarmUpRequestWithSemanticFrame_ = false;

private:
    virtual TStatus CallApply(const TRequest& request, const IContext& ctx) = 0;
    virtual TStatus GetApplyResponse(const TRequest& request, const IContext& ctx, TScenarioResponse& response) = 0;

    // TScenarioWrapperBase overrides:
    TErrorOr<EApplyResult> StartApplyImpl(const TRequest& request, const IContext& ctx, TScenarioResponse& response,
                                     const NMegamind::TMegamindAnalyticsInfo& megamindAnalyticsInfo,
                                     const TQualityStorage& storage, const TProactivityAnswer& proactivity) override {
        auto env = this->GetApplyEnv(request, ctx);
        env.RequestFrames = TArrayRef<TSemanticFrame>{};

        switch (DeferApplyMode) {
            case EDeferredApplyMode::WarmUp: {
                LOG_INFO(ctx.Logger()) << "Apply needed while warming up request";
                ctx.Sensors().IncRate(/* labels= */ {
                    {NSignal::NAME, NSignal::SCENARIO_WARMUP_PER_SECOND},
                    {NSignal::SCENARIO_NAME, this->GetScenario().GetName()},
                });
                auto& builder = response.ForceBuilder(env.Ctx.SpeechKitRequest(), request, this->GuidGenerator);
                const auto error = WarmUpApply(env, builder, request);
                if (error.Defined()) {
                    return *error;
                }
                LOG_INFO(ctx.Logger()) << "Scenario " << this->GetScenario().GetName() << " has successfully warmed up and deferred apply";
                return EApplyResult::Deferred;
            }
            case EDeferredApplyMode::DeferApply: {
                auto& builder = response.ForceBuilder(env.Ctx.SpeechKitRequest(), request, this->GuidGenerator);
                TMaybe<TResponseBuilderProto> builderProto;
                if (SaveProto) {
                    builderProto = builder.ToProto();
                }
                const auto error = DeferApply(env, request, builder, builderProto, megamindAnalyticsInfo,
                                              storage, response.GetIntentFromFeatures(), proactivity);
                if (error.Defined()) {
                    return *error;
                }
                LOG_INFO(ctx.Logger()) << "Scenario " << this->GetScenario().GetName() << " has successfully deferred apply";
                return EApplyResult::Deferred;
            }

            case EDeferredApplyMode::DeferredCall:
                [[fallthrough]];
            case EDeferredApplyMode::DontDeferApply: {
                THistogramScope applyScopeTimer(env.Ctx.Sensors(),
                                                {{NSignal::NAME, NSignal::SCENARIO_BASED_TIMER_APPLY_STAGE},
                                                 {NSignal::SCENARIO_NAME, this->GetScenario().GetName()}},
                                                THistogramScope::ETimeUnit::Millis);
                const auto error = CallApply(request, ctx);
                if (error.Defined()) {
                    return *error;
                }
                LOG_INFO(ctx.Logger()) << "Scenario " << this->GetScenario().GetName() << " has successfully called apply";
                return EApplyResult::Called;
            }
        }
        Y_UNREACHABLE();
    }

    TErrorOr<EApplyResult> FinishApplyImpl(const TRequest& request, const IContext& ctx, TScenarioResponse& response) override {
        if (const auto error = GetApplyResponse(request, ctx, response); error.Defined()) {
            return *error;
        }
        LOG_INFO(ctx.Logger()) << "Scenario " << this->GetScenario().GetName() << " has successfully answered apply";
        return EApplyResult::Called;
    }
};

class TProtocolScenarioWrapper : public TEffectfulScenarioWrapperBase<TConfigBasedProtocolScenario, /* SaveProto= */ true> {
public:
    TProtocolScenarioWrapper(const TConfigBasedProtocolScenario& scenario,
                             const IContext& ctx,
                             const TSemanticFrames& semanticFrames,
                             const NMegamind::IGuidGenerator& guidGenerator,
                             EDeferredApplyMode deferApply,
                             bool restoreAllFromSession,
                             NMegamind::TItemProxyAdapter& itemProxyAdapter,
                             bool passDataSourcesInRequest);
private:
    using TScenarioError = NScenarios::TScenarioError;
    using TScenarioResponseBody = NScenarios::TScenarioResponseBody;
    using TScenarioCommitResponse = NScenarios::TScenarioCommitResponse;
    using TScenarioApplyResponse = NScenarios::TScenarioApplyResponse;
    using TScenarioContinueResponse = NScenarios::TScenarioContinueResponse;

    // TScenarioWrapperBase overrides:
    NMegamind::TScenarioSessionView CreateSessionView(const TLightScenarioEnv& env) const;
    NMegamind::TMementoDataView CreateMementoDataView(const TLightScenarioEnv& env) const;

    TStatus InitImpl(const TRequest& request, const IContext& ctx, NMegamind::IDataSources& dataSources) override;
    TStatus AskImpl(const TRequest& request, const IContext& ctx, TScenarioResponse& scenarioResponse) override;
    TStatus FinalizeImpl(const TRequest& request, const IContext& ctx, TScenarioResponse& scenarioResponse) override;

    TStatus StartHeavyContinueImpl(const TRequest& request, const IContext& ctx) override;
    TStatus FinishContinueImpl(const TRequest& request, const IContext& ctx, TScenarioResponse& scenarioResponse) override;

    // TEffectfulScenarioWrapperBase overrides:
    TStatus CallApply(const TRequest& request, const IContext &ctx) override;
    TStatus GetApplyResponse(const TRequest& request, const IContext& ctx, TScenarioResponse& scenarioResponse) override;

    TStatus BuildResponse(const TRequest& request, const IContext& ctx,
                          const TScenarioResponseBody& scenarioResponseBody, TState& state, const TString& version,
                          TScenarioResponse& response);

    TStatus OnResponseBody(const IContext& ctx, const TScenarioResponseBody& scenarioResponseBody,
                           const TString& version, TScenarioResponse& response);

    [[nodiscard]] TStatus CheckScenarioVersion(const TString& version, const IContext& ctx) const;

    TStatus OnCommitResponse(const IContext& ctx, const TScenarioCommitResponse& scenarioResponse,
                             TScenarioResponse& response);
    TStatus OnApplyResponse(const IContext& ctx, const TScenarioApplyResponse& scenarioResponse,
                            TScenarioResponse& response);
    TStatus OnContinueResponse(const IContext& ctx, const TScenarioContinueResponse& scenarioResponse,
                               TScenarioResponse& response);

    template <typename TProtoResponse>
    TStatus OnFinalResponse(const IContext& ctx, const TProtoResponse& scenarioResponse, TScenarioResponse& response,
                            const TString& method);

    TStatus OnError(const TScenarioError& error, TScenarioResponse& response);

    bool ShouldBecomeActiveScenario() const override {
        return ProtocolInfo.GetRequestIsExpected();
    }

protected:
    bool PassDataSourcesInRequest;

private:
    [[nodiscard]] const TString& GetSerializerScenarioName(const IContext& ctx) const;

    friend class TProtocolScenarioWrapperTest;

    enum class EApplyType {
        Commit = 1,
        Apply = 2,
        Continue = 3
    };

    // Set iff Mode is set.
    TMaybe<EApplyType> Type;

    TMaybe<TErrorOr<NHttpFetcher::THandle::TRef>> ContinueHandle;

    NMegamind::TItemProxyAdapter& ItemProxyAdapter;
};

class TAppHostProxyProtocolScenarioWrapper final : public TProtocolScenarioWrapper {
public:
    TAppHostProxyProtocolScenarioWrapper(const TConfigBasedAppHostProxyProtocolScenario& scenario,
                             const IContext& ctx,
                             const TSemanticFrames& semanticFrames,
                             const NMegamind::IGuidGenerator& guidGenerator,
                             EDeferredApplyMode deferApply,
                             bool restoreAllFromSession,
                             NMegamind::TItemProxyAdapter& itemProxyAdapter);

    void Accept(const IScenarioVisitor& visitor) const override;

private:
    // This is needed for keeping TProtocolScenarioWrapper a non-template class
    const TConfigBasedAppHostProxyProtocolScenario& ScenarioCopy;
};

class TAppHostPureProtocolScenarioWrapper final : public TProtocolScenarioWrapper {
public:
    TAppHostPureProtocolScenarioWrapper(const TConfigBasedAppHostPureProtocolScenario& scenario,
                                        const IContext& ctx,
                                        const TSemanticFrames& semanticFrames,
                                        const NMegamind::IGuidGenerator& guidGenerator,
                                        EDeferredApplyMode deferApply,
                                        bool restoreAllFromSession,
                                        NMegamind::TItemProxyAdapter& itemProxyAdapter);

    void Accept(const IScenarioVisitor& visitor) const override;

private:
    // This is needed for keeping TProtocolScenarioWrapper a non-template class
    const TConfigBasedAppHostPureProtocolScenario& ScenarioCopy;
};

void BuildScenarioResponseFromResponseBody(const NScenarios::TScenarioResponseBody& scenarioResponseBody,
                                           TScenarioResponse& response, const IContext& ctx, const TRequest& request,
                                           const NMegamind::IGuidGenerator& guidGenerator,
                                           NMegamind::TMegamindAnalyticsInfo& megamindAnalyticsInfo,
                                           NMegamind::TAnalyticsInfoBuilder& analyticsInfoBuilder,
                                           const TString& version,
                                           const NScenarios::TScenarioBaseRequest::ERequestSourceType requestSourceType = {},
                                           const TMaybe<TDirectiveChannel::EDirectiveChannel> channel = Nothing(),
                                           const TMaybe<bool> forcedShouldListen = Nothing(),
                                           const TMaybe<TString> forcedEmotion = Nothing());

NMegamindAppHost::TScenarioTextResponse ParseTextResponse(const NScenarios::TScenarioResponseBody& scenarioResponseBody,
                                                          const TScenarioResponse& response);

TVector<TSemanticFrame> MakeFramesForRequest(const TVector<TSemanticFrame>& requestFrames,
                                             const TVector<TSemanticFrame>& allParsedSemanticFrames,
                                             const TScenario& scenario);

void AddStartScenarioContinueFlag(const TStringBuf scenarioName, NMegamind::TItemProxyAdapter& itemProxyAdapter);

template<typename ...TScenarioTypes>
void OnlyFor(
    const TScenarioWrapperPtrs& wrappers,
    std::function<void(const TScenarioTypes&, TIntrusivePtr<IScenarioWrapper>)> ... fns
) {
    for (const auto& wrapper : wrappers) {
        OnlyVisit<TScenarioTypes...>(wrapper, [&wrapper, &fns](const TScenarioTypes& scenario) {
            fns(scenario, wrapper);
        }...);
    }
}

template<typename ...TScenarioTypes>
void OnlyFor(
    const TScenarioWrapperPtrs& wrappers,
    std::function<void(const TScenario&, TIntrusivePtr<IScenarioWrapper>)> fn
) {
    OnlyFor<TScenarioTypes...>(wrappers, [&fn](const TScenarioTypes& scenario, TIntrusivePtr<IScenarioWrapper> wrapper) {
            fn(scenario, wrapper);
    }...);
}

} // namespace NAlice
