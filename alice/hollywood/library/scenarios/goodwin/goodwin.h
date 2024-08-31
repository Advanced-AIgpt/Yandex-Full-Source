#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/frame_filler/lib/frame_filler_handlers.h>
#include <alice/hollywood/library/http_requester/apphost_http_requester.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/goodwin/handlers/goodwin_handlers.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/string/builder.h>
#include <util/system/backtrace.h>

#include <tuple>

namespace NAlice::NHollywood {

constexpr TStringBuf APPHOST_HTTP_META_FLAG = "apphost_http";
constexpr TStringBuf NO_APPHOST_HTTP_META_FLAG = "no_apphost_http";
constexpr TStringBuf GOODWIN_APPHOST_TYPE_PREFIX = "goodwin_";
constexpr TStringBuf GOODWIN_APPHOST_NODE_PREFIX = "GOODWIN_";

class TGoodwinRunHandle : public TScenario::THandleBase {
public:
    explicit TGoodwinRunHandle(NFrameFiller::TAcceptDocPredicate acceptDoc)
        : AcceptDoc(acceptDoc)
    {
    }

    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override {
        const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
        const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

        TSimpleSharedPtr<IHttpRequester> urlRequester =
            MakeApphostHttpRequester(ctx.ServiceCtx, ctx.RequestMeta, ctx.AppHostParams, ctx.Ctx.Logger(),
                TString{GOODWIN_APPHOST_TYPE_PREFIX}, TString{GOODWIN_APPHOST_NODE_PREFIX});
        NFrameFiller::TGoodwinScenarioRunHandler runHandler(urlRequester, AcceptDoc);
        try {
            const NScenarios::TScenarioRunResponse response = NFrameFiller::Run(request, runHandler, ctx.Ctx.Logger());
            ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        } catch (const TAppHostNodeExecutionBreakException& e) {
            LOG_WARN(ctx.Ctx.Logger()) << e.AsStrBuf();
        } catch (...) {
            TString errorMsg = TStringBuilder{} << "EXCEPTION: " << CurrentExceptionMessage() << '\n';
            TStringOutput errorMsgStream(errorMsg);
            FormatBackTrace(&errorMsgStream);
            LOG_ERROR(ctx.Ctx.Logger()) << errorMsg;
            TRunResponseBuilder response;
            response.SetError("error", errorMsg);
            ctx.ServiceCtx.AddProtobufItem(*std::move(response).BuildResponse(), RESPONSE_ITEM);
        }
    }

private:
    NFrameFiller::TAcceptDocPredicate AcceptDoc;
};


class TGoodwinCommitHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit";
    }

    void Do(TScenarioHandleContext& ctx) const override {
        const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
        const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

        TSimpleSharedPtr<IHttpRequester> urlRequester =
            MakeApphostHttpRequester(ctx.ServiceCtx, ctx.RequestMeta, ctx.AppHostParams, ctx.Ctx.Logger(),
                TString{GOODWIN_APPHOST_TYPE_PREFIX}, TString{GOODWIN_APPHOST_NODE_PREFIX});
        NFrameFiller::TGoodwinScenarioCommitHandler commitHandler(urlRequester);
        try {
            const NScenarios::TScenarioCommitResponse response = NFrameFiller::Commit(request, commitHandler, ctx.Ctx.Logger());
            ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        } catch (const TAppHostNodeExecutionBreakException& e) {
            LOG_WARN(ctx.Ctx.Logger()) << e.AsStrBuf();
        } catch (...) {
            TString errorMsg = TStringBuilder{} << "EXCEPTION: " << CurrentExceptionMessage() << '\n';
            TStringOutput errorMsgStream(errorMsg);
            FormatBackTrace(&errorMsgStream);
            LOG_ERROR(ctx.Ctx.Logger()) << errorMsg;
            TCommitResponseBuilder response;
            response.SetError("error", errorMsg);
            ctx.ServiceCtx.AddProtobufItem(*std::move(response).BuildResponse(), RESPONSE_ITEM);
        }
    }
};


class TGoodwinApplyHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "apply";
    }

    void Do(TScenarioHandleContext& ctx) const override {
        const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
        const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

        NFrameFiller::TGoodwinScenarioApplyHandler applyHandler;
        try {
            const NScenarios::TScenarioApplyResponse response = NFrameFiller::Apply(request, applyHandler, ctx.Ctx.Logger());
            ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        } catch (const TAppHostNodeExecutionBreakException& e) {
            LOG_WARN(ctx.Ctx.Logger()) << e.AsStrBuf();
        } catch (...) {
            TString errorMsg = TStringBuilder{} << "EXCEPTION: " << CurrentExceptionMessage() << '\n';
            TStringOutput errorMsgStream(errorMsg);
            FormatBackTrace(&errorMsgStream);
            LOG_ERROR(ctx.Ctx.Logger()) << errorMsg;
            TApplyResponseBuilder response;
            response.SetError("error", errorMsg);
            ctx.ServiceCtx.AddProtobufItem(*std::move(response).BuildResponse(), RESPONSE_ITEM);
        }
    }
};


class TGoodwinRunDispatchHandle : public TScenario::THandleBase {
public:
    void Do(TScenarioHandleContext& ctx) const override {
        ctx.ServiceCtx.AddFlag(APPHOST_HTTP_META_FLAG);
    }

    TString Name() const override {
        return "run_dispatch";
    }
};


class TGoodwinCommitDispatchHandle : public TScenario::THandleBase {
public:
    void Do(TScenarioHandleContext& ctx) const override {
        ctx.ServiceCtx.AddFlag(APPHOST_HTTP_META_FLAG);
    }

    TString Name() const override {
        return "commit_dispatch";
    }
};


#define REGISTER_GOODWIN_SCENARIO(tag, acceptDoc)                                                   \
    REGISTER_SCENARIO(                                                                              \
        tag,                                                                                        \
        AddHandle<TGoodwinRunHandle>(acceptDoc)                                                     \
        .AddHandle<TGoodwinCommitHandle>()                                                          \
        .AddHandle<TGoodwinApplyHandle>()                                                           \
        .AddHandle<TGoodwinRunDispatchHandle>()                                                     \
        .AddHandle<TGoodwinCommitDispatchHandle>()                                                  \
    )

}  // namespace NAlice::NHollywood
