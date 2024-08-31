#pragma once

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/request_composite/composite.h>
#include <alice/megamind/library/request_composite/view.h>
#include <alice/megamind/library/requestctx/requestctx.h>

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/apphost_request/protos/required_node_meta.pb.h>

#include <memory>

namespace NAlice::NMegamind {

class TAppHostNodeHandler {
public:
    TAppHostNodeHandler(IGlobalCtx& globalCtx, bool useAppHostStreaming)
        : GlobalCtx{globalCtx}
        , UseAppHostStreaming_{useAppHostStreaming}
    {
    }

public:
    virtual std::unique_ptr<IAppHostCtx> CreateContext(NAppHost::IServiceContext& ctx, TRTLogger& log) const;
    virtual TRTLogger CreateLogger(NAppHost::IServiceContext& ctx) const;
    static void UpdateLoggerRequestId(const TString& nodeLocation,
                                      const NMegamindAppHost::TRequiredNodeMeta& nodeMeta,
                                      TRTLogger& log);

    void RunSync(NAppHost::IServiceContext& ctx) const;
    NThreading::TFuture<void> RunAsync(NAppHost::TServiceContextPtr ctx) const;

protected:
    virtual TStatus Execute(IAppHostCtx& ahCtx) const = 0;

private:
    bool UseAppHostStreaming() const {
        return UseAppHostStreaming_;
    }

protected:
    IGlobalCtx& GlobalCtx;
    const bool UseAppHostStreaming_;
};

class TAppHostRequestCtx : public TRequestCtx {
public:
    class TInitializer : public TRequestCtx::IInitializer {
    public:
        TInitializer(IAppHostCtx& ahCtx, NUri::TUri uri, TCgiParameters cgi, THttpHeaders headers);
        TInitializer(IAppHostCtx& ahCtx, NUri::TUri uri, TCgiParameters cgi, THttpHeaders headers, TStageTimersPtr stageTimers);

        // TRequestCtx::IInitializer overrides.
        TRTLogger& Logger() override {
            return AhCtx_.Log();
        }

        TCgiParameters StealCgi() override {
            return std::move(Cgi_);
        }

        NUri::TUri StealUri() override {
            return std::move(Uri_);
        }

        THttpHeaders StealHeaders() override {
            return std::move(Headers_);
        }

        TStageTimersPtr StealStageTimers() override {
            return std::move(StageTimers_);
        }

    protected:
        IAppHostCtx& AhCtx_;
        NUri::TUri Uri_;
        TCgiParameters Cgi_;
        THttpHeaders Headers_;
        TStageTimersPtr StageTimers_;
    };

public:
    explicit TAppHostRequestCtx(IAppHostCtx& ahCtx);
    TAppHostRequestCtx(IAppHostCtx& ahCtx, TInitializer&& initializer)
        : TRequestCtx{ahCtx.GlobalCtx(), std::move(initializer)}
        , ItemProxyAdapter_{ahCtx.ItemProxyAdapter()}
    {
    }

    // TRequestCtx overrides:
    THolder<IHttpResponse> CreateHttpResponse() const override {
        return {};
    }

    const TString& Body() const override {
        // NB. Body doesn't need in all nodes except skr.
        return Default<TString>();
    }

    TStringBuf NodeLocation() const override {
        return ItemProxyAdapter_.NodeLocation().Path;
    }

protected:
    TItemProxyAdapter& ItemProxyAdapter_;
};

} // namespace NAlice::NMegamind
