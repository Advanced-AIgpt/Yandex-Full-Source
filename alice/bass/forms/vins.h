#pragma once

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/util/error.h>

#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/noncopyable.h>
#include <util/generic/scope.h>

namespace NBASS {

class TRequestHandler;

// Interface for form handler.
class IHandler {
public:
    virtual ~IHandler() = default;
    virtual TResultValue Do(TRequestHandler& r) = 0;
    virtual TResultValue DoSetup(TSetupContext& ctx);
};

class IContinuation {
public:
    using TPtr = std::unique_ptr<IContinuation>;

    enum class EFinishStatus { Completed, NeedCommit, NeedApply };

    explicit IContinuation(EFinishStatus finishStatus = EFinishStatus::Completed,
                           TResultValue result = ResultSuccess())
        : Result(std::move(result))
        , FinishStatus(finishStatus)
    {
    }

    virtual ~IContinuation() = default;

    TResultValue ApplyIfNotFinished() {
        Y_SCOPE_EXIT(this) {
            FinishStatus = EFinishStatus::Completed;
        };
        if (IsFinished())
            return GetResult();
        return Apply();
    }

    bool NeedsContinuation() const {
        return !IsFinished();
    }

    bool IsFinished() const {
        return FinishStatus == EFinishStatus::Completed;
    }

    EFinishStatus GetFinishStatus() const {
        return FinishStatus;
    }

    bool NeedsApply() const {
        return FinishStatus == EFinishStatus::NeedApply;
    }

    TResultValue GetResult() const {
        if (NeedsApply())
            return TError{TError::EType::SYSTEM, "Attempting to get the result of an incomplete continuation!"};
        return Result;
    }

    NSc::TValue ToJson() const;

    virtual TContext& GetContext() const = 0;
    virtual TStringBuf GetName() const = 0;

    NAlice::NScenarios::TScenarioRunResponse AsProtocolRunResponse() const;
    NAlice::NScenarios::TScenarioCommitResponse AsProtocolCommitResponse() const;
    NAlice::NScenarios::TScenarioApplyResponse AsProtocolApplyResponse() const;

protected:
    virtual TResultValue Apply() = 0;
    virtual NSc::TValue ToJsonImpl() const = 0;

private:
    TResultValue Result;
    EFinishStatus FinishStatus;
};

class TCompletedContinuation : public IContinuation {
public:
    explicit TCompletedContinuation(TContext& ctx, TResultValue result = TResultValue())
        : IContinuation{EFinishStatus::Completed, std::move(result)}
        , Context{&ctx}
    {
    }

    explicit TCompletedContinuation(TIntrusivePtr<TContext> ctx, TResultValue result = TResultValue())
        : IContinuation{EFinishStatus::Completed, std::move(result)}
        , Context{ctx}
    {
    }

    static IContinuation::TPtr Make(TContext& ctx, TResultValue result = TResultValue()) {
        return std::make_unique<TCompletedContinuation>(ctx, std::move(result));
    }

    TStringBuf GetName() const override {
        return TStringBuf("TCompletedContinuation");
    }

    NSc::TValue ToJsonImpl() const override {
        LOG(ERR) << "Attempting to serialize TCompletedContinuation!" << Endl;
        return NSc::TValue::Null();
    }

    static TMaybe<TCompletedContinuation> FromJson(NSc::TValue value, TGlobalContextPtr /* globalContext */,
                                                   NSc::TValue /* meta */, const TString& /* authHeader */,
                                                   const TString& /* appInfoHeader */,
                                                   const TString& /* fakeTimeHeader */,
                                                   const TMaybe<TString>& /* userTicketHeader */,
                                                   const NSc::TValue& /* configPatch */) {
        LOG(ERR) << "Attempting to deserialize TCompletedContinuation: " << value << Endl;
        return Nothing();
    }

    TContext& GetContext() const override {
        return *Context;
    }

    TIntrusivePtr<TContext> Context;

protected:
    TResultValue Apply() override {
        return GetResult();
    }
};

// to support Hollywood's brand of the protocol, override these in your child class:
// - TStringBuf GetName() const
// - TResultValue Apply()
class THollywoodContinuation : public IContinuation {
public:
    THollywoodContinuation(TContext::TPtr context, NSc::TValue&& applyArguments, NSc::TValue&& featuresData)
        : IContinuation{IContinuation::EFinishStatus::NeedApply}
        , Context_{context}
        , ApplyArguments_{std::move(applyArguments)}
        , FeaturesData_{std::move(featuresData)}
    {}

    // NOTE(a-square): meta is added by Hollywood, it's not serialized by the prepare stage
    THollywoodContinuation(NSc::TValue value, TGlobalContextPtr globalContext, NSc::TValue meta,
                           const TString& authHeader, const TMaybe<TString>& userTicketHeader,
                           const NSc::TValue& configPatch);

    TContext& GetContext() const override {
        return *Context_;
    }

    const NSc::TValue& ApplyArguments() const {
        return ApplyArguments_;
    }

    const NSc::TValue& FeaturesData() const {
        return FeaturesData_;
    }

protected:
    NSc::TValue ToJsonImpl() const override {
        Y_ENSURE(Context_->HasForm());

        NSc::TValue result;
        result["apply_arguments"] = ApplyArguments_;
        result["features_data"] = FeaturesData_;
        result["context"] = Context_->ToJson();
        return result;
    }

private:
    TContext::TPtr Context_;

protected:
    // descendants may modify these
    NSc::TValue ApplyArguments_;
    NSc::TValue FeaturesData_;
};

class TContinuableHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;
    virtual IContinuation::TPtr Prepare(TRequestHandler& r) = 0;
};


class TDefaultHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;
};

class THandlersMap : NNonCopyable::TNonCopyable {
public:
    using THandlerFactory = std::function<THolder<IHandler>()>;
    using TContinuableHandlerFactory = std::function<THolder<TContinuableHandler>()>;

public:
    explicit THandlersMap(IGlobalContext& globalCtx);

    // TODO just for backward compat (remove when changed to RegisterFormHandler()).
    void emplace(TStringBuf name, THandlerFactory factory) {
        RegisterFormHandler(name, factory);
    }

    void RegisterFormHandler(TStringBuf name, THandlerFactory factory);
    void RegisterActionHandler(TStringBuf name, THandlerFactory factory);
    void RegisterContinuableHandler(TStringBuf name, TContinuableHandlerFactory factory);
    void RegisterFormAndContinuableHandler(TStringBuf name, TContinuableHandlerFactory factory);

    template <typename THandler, typename... TArgs>
    void RegisterFormAndContinuableHandler(TStringBuf name, TArgs&&... args) {
        auto factory = [&args...]() { return THolder<THandler>(new THandler{std::forward<TArgs>(args)...}); };
        RegisterFormAndContinuableHandler(name, factory);
    }

    const THandlerFactory* FormHandler(TStringBuf name) const;
    const THandlerFactory* ActionHandler(TStringBuf name) const;
    const TContinuableHandlerFactory* ContinuableHandler(TStringBuf name) const;

    const TDuration FormSLA(TStringBuf name) const;
    const TDuration ActionSLA(TStringBuf name) const;

public:
    IGlobalContext& GlobalCtx;

private:
    using THandlers = TMap<TString, THandlerFactory>;
    using TContinuableHandlers = THashMap<TString, TContinuableHandlerFactory>;

    THandlers FormHandlers;
    THandlers ActionHandlers;
    TContinuableHandlers ContinuableHandlers;
    THashMap<TString, TDuration> FormSLAs;
    THashMap<TString, TDuration> ActionSLAs;
    THandlerFactory DefaultHandlerFactory;
};

// VINS request handler runner and context for request handler.
class TRequestHandler {
public:
    explicit TRequestHandler(TContext::TPtr context)
        : Context(context)
    {
    }

    // run handler for the form specified in Context
    TResultValue RunFormHandler();

    // run handler for the form specified in Context
    // output result form to <out>
    TResultValue RunFormHandler(NSc::TValue* out);

    // run handler for the action specified in Context
    // output result form to <out>
    TResultValue RunActionHandler(NSc::TValue* out);

    TResultValue RunContinuableHandler(NSc::TValue* out);
    TResultValue RunContinuableHandler(NAlice::NScenarios::TScenarioRunResponse& out,
                                       NSc::TValue& outValue);

    TContext& Ctx() {
        return *Context.Get();
    }

    TStringBuf ReqId() const {
        return Context->ReqId();
    }

    void SwapContext(TContext::TPtr context) {
        Context = std::move(context);
    }

private:
    TResultValue RunContinuableHandlerImpl(IContinuation::TPtr& resultContinuation);

    TContext::TPtr Context;
};

class TContinuationParserRegistry {
    using TParser = std::function<IContinuation::TPtr(NSc::TValue, TGlobalContextPtr, NSc::TValue, const TString&,
                                                      const TString&, const TString&, const TMaybe<TString>&,
                                                      const NSc::TValue&)>;

public:
    TContinuationParserRegistry() {
        Register<TCompletedContinuation>("TCompletedContinuation");
    }

    template <typename TContinuation>
    void Register(TStringBuf name) {
        TParser parser = [](NSc::TValue json, TGlobalContextPtr globalContext,
                            NSc::TValue meta, const TString& authHeader,
                            const TString& appInfoHeader, const TString& fakeTimeHeader,
                            const TMaybe<TString>& userTicketHeader, const NSc::TValue configPatch) -> IContinuation::TPtr {
            auto continuation = TContinuation::FromJson(json, globalContext, std::move(meta), authHeader,
                                                        appInfoHeader, fakeTimeHeader, userTicketHeader,
                                                        configPatch);
            if (!continuation.Defined()) {
                LOG(ERR) << "Cannot make IContinuation from json: " << json << Endl;
                return {};
            }

            return std::make_unique<TContinuation>(std::move(*continuation));
        };
        parsers.emplace(name, parser);
    }

    IContinuation::TPtr MakeFromJson(NSc::TValue json, TGlobalContextPtr globalCtx, NSc::TValue meta,
                                     const TString& authHeader, const TString& appInfoHeader,
                                     const TString& fakeTimeHeader, const TMaybe<TString>& userTicketHeader,
                                     const NSc::TValue& configPatch) {
        TString objectName = json["ObjectTypeName"].ForceString();
        const auto* parser = parsers.FindPtr(objectName);
        if (!parser) {
            LOG(ERR) << "Have no registered parser with name " << objectName << Endl;
            return {};
        }
        return (*parser)(std::move(json), globalCtx, std::move(meta), authHeader, appInfoHeader, fakeTimeHeader,
                         userTicketHeader, configPatch);
    }

private:
    THashMap<TString, TParser> parsers;
};

} // namespace NBASS
