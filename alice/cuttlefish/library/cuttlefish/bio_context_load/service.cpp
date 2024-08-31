#include "service.h"
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/library/proto/protobuf.h>
#include <voicetech/library/messages/build.h>
#include <voicetech/library/proto_api/yabio.pb.h>

using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;

namespace {

    class TBioContextLoadProcessor {
    public:
        TBioContextLoadProcessor(NAppHost::IServiceContext&, TLogContext);

        void Post();

    private:
        void InitNewContext();
        void AddEventException(const TString& text);

    private:
        NAppHost::IServiceContext& Ctx;
        TLogContext LogContext;
        mutable TSourceMetrics Metrics;

        const NCachalotProtocol::TYabioContextRequest CachalotRequest;
        const NCachalotProtocol::TYabioContextResponse CachalotResponse;
        YabioProtobuf::YabioContext YabioCtx;
    };

    template<typename TProtobuf>
    TProtobuf TryLoad(const NAppHost::IServiceContext& ctx, const TStringBuf type) {
        TProtobuf proto;
        if (ctx.HasProtobufItem(type)) {
            proto = ctx.GetOnlyProtobufItem<TProtobuf>(type);
        }
        return proto;
    }

    TBioContextLoadProcessor::TBioContextLoadProcessor(NAppHost::IServiceContext& ctx, TLogContext logContext)
        : Ctx(ctx), LogContext(logContext), Metrics(ctx, "bio_context_load")
        , CachalotRequest(TryLoad<NCachalotProtocol::TYabioContextRequest>(ctx, ITEM_TYPE_YABIO_CONTEXT_REQUEST))
        , CachalotResponse(TryLoad<NCachalotProtocol::TYabioContextResponse>(ctx, ITEM_TYPE_YABIO_CONTEXT_RESPONSE)
    ) {
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostYabioContextRequest>(CachalotRequest.ShortUtf8DebugString());
    }

    void TBioContextLoadProcessor::Post() {
        TString debugStrContext{"new"};
        if (CachalotResponse.HasError()) {
            const auto& error = CachalotResponse.GetError();
            if (error.HasStatus() && (
                // TODO (paxakor): simplify this condition when cacahlot is released (~ march of 2022)
                error.GetStatus() == NCachalotProtocol::NO_CONTENT ||
                error.GetStatus() == NCachalotProtocol::NOT_FOUND
                )
            ) {
                // not fatal error (simple create new empty context)
                InitNewContext();
                debugStrContext = "new (NOT_FOUND)";
            } else {
                TString err{TStringBuilder() << "status=" << int(error.GetStatus()) << " " << error.GetText()};
                AddEventException(err);
                return;
            }
        } else if (CachalotResponse.HasSuccess()) {
            auto& success = CachalotResponse.GetSuccess();
            if (success.HasOk() && success.GetOk() && success.HasContext()) {
                try {
                    if (!YabioCtx.ParseFromString(success.GetContext())) {
                        throw yexception() << "fail parse yabio protobuf context";
                    }
                    debugStrContext.clear();
                    TStringOutput so(debugStrContext);
                    so << "group_id=" << YabioCtx.group_id() << " users=" << YabioCtx.users().size() << " guests=" << YabioCtx.guests().size() << " enrollings=" << YabioCtx.enrolling().size();
                } catch (...) {
                    AddEventException(TStringBuilder() << "bio_context_load-post: fail parse yabio context: " << CurrentExceptionMessage());
                    return;
                }
            } else {
                InitNewContext();
                debugStrContext = "new (!has_context)";
            }
        } else {
            // handle unexpected error
            AddEventException("unexpected YabioCachalotResponse (not error & not success)");
            return;
        }

        // on success generate yabio_context
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostYabioContext>(debugStrContext);
        Ctx.AddProtobufItem(YabioCtx, ITEM_TYPE_YABIO_CONTEXT);
    }

    void TBioContextLoadProcessor::InitNewContext() {
        YabioCtx.set_group_id(CachalotRequest.GetLoad().GetKey().GetGroupId());
    }

    void TBioContextLoadProcessor::AddEventException(const TString& message) {
        NAliceProtocol::TDirective directive;
        auto& exc = *directive.MutableException();
        exc.SetScope("BioContextLoadProcessor");
        exc.SetText(message);
        LogContext.LogEventErrorCombo<NEvClass::SendToAppHostDirective>(directive.ShortUtf8DebugString());
        Ctx.AddProtobufItem(directive, ITEM_TYPE_DIRECTIVE);
    }
}

void NAlice::NCuttlefish::NAppHostServices::BioContextLoadPost(NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TBioContextLoadProcessor(ctx, logContext).Post();
}
