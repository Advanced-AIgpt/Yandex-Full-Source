#include "service.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>

#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/protos/events.pb.h>

#include <util/string/builder.h>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

void AddDirective(NAppHost::TServiceContextPtr ctx) {
    NAliceProtocol::TDirective directive;
    directive.MutableLogAckResponse();
    ctx->AddProtobufItem(directive, ITEM_TYPE_DIRECTIVE);
}

bool IsStreamControl(const TStringBuf type, const NAppHost::NService::TProtobufItem& item, const TLogContext& logContext) {
    if (type != ITEM_TYPE_AUDIO) {
        return false;
    }

    try {
        NAliceProtocol::TAudio audio;
        ParseProtobufItem(item, audio);
        if (audio.HasEndSpotter() || audio.HasEndStream()) {
            logContext.LogEvent(NEvClass::InfoMessage("Log.Spotter found the end of a stream"));
            return true;
        }
    } catch (...) {
        logContext.LogEvent(NEvClass::WarningMessage(TStringBuilder() << "LogSpotter ProcessAppHostProtoItem error: " << CurrentExceptionMessage()));
    }
    return false;
}

void OnNextInput(NThreading::TPromise<void> promise, NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    const auto items = ctx->GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (IsStreamControl(it.GetType(), *it, logContext)) {
            AddDirective(ctx);
            promise.SetValue();
            return;
        }
    }

    ctx->NextInput().Apply([promise = std::move(promise), ctx = std::move(ctx), logContext = std::move(logContext)](auto hasData) mutable {
        if (!hasData.GetValue()) {
            promise.SetValue();
            return;
        }
        OnNextInput(std::move(promise), std::move(ctx), std::move(logContext));
    });
}

}

NThreading::TPromise<void> LogSpotter(NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    auto promise = NThreading::NewPromise<void>();
    OnNextInput(promise, std::move(ctx), std::move(logContext));
    return promise;
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
