#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>
#include <apphost/api/service/cpp/service.h>
#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/service.h>
#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/processor.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/protos/bio_context_sync.pb.h>
#include <alice/megamind/protos/guest/enrollment_headers.pb.h>
#include <voicetech/library/messages/build.h>
#include <voicetech/library/proto_api/yabio.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    template <typename TServiceCtx, typename TProcessorPtr>
    void BioContextSyncInternal(
        TServiceCtx& ctx,
        TProcessorPtr processor,
        TLogContext logContext,
        TSourceMetrics& metrics
    ) {
        if (!ctx.HasProtobufItem(ITEM_TYPE_ENROLLMENT_HEADERS)) {
            logContext.LogEventErrorCombo<NEvClass::ErrorMessage>("enrollment_headers item is not received");
            metrics.SetError("enrollment_headers_missed");
            return;
        } else {
            metrics.PushRate("enrollment_headers", "ok");
        }

        if (!ctx.HasProtobufItem(ITEM_TYPE_YABIO_CONTEXT)) {
            logContext.LogEventInfoCombo<NEvClass::InfoMessage>("No YabioContext received");
            metrics.PushRate("yabio_context", "no_data");
            return;
        } else {
            metrics.PushRate("yabio_context", "ok");
        }

        TVector<THolder<NAliceProtocol::TEnrollmentUpdateDirective>> updateDirectives = processor->Process(
            ctx.template GetOnlyProtobufItem<NAlice::TEnrollmentHeaders>(ITEM_TYPE_ENROLLMENT_HEADERS),
            ctx.template GetOnlyProtobufItem<YabioProtobuf::YabioContext>(ITEM_TYPE_YABIO_CONTEXT)
        );

        for (auto& updateDirectiveHolder : updateDirectives) {
            ctx.AddProtobufItem(*updateDirectiveHolder, ITEM_TYPE_UPDATE_CLIENT_ENROLLMENT_DIRECTIVE);

            // Enrollment part of the directive is a heavy base-64 string, so it doesn't really make sense to log it.
            logContext.LogEventInfoCombo<NEvClass::SendToAppHostEnrollmentUpdateDirective>(updateDirectiveHolder->GetHeader().ShortUtf8DebugString());
        }
    }

    void BioContextSync(NAppHost::IServiceContext& ctx, TLogContext logContext);

}
