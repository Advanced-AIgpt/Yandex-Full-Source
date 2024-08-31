#include "http_init.h"

#include "frame_converter.h"
#include "http_processor.h"
#include "metadata.h"
#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/gproxy/library/protos/metadata.pb.h>
#include <alice/gproxy/library/protos/service.gproxy.pb.h>


namespace NGProxy {

void TGSetupHttpInit::Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext) {
    NGProxy::TMetadata meta;
    NGProxy::GSetupRequestInfo req;

    if (!ctx.HasProtobufItem("proto_http_request")) {
        AddError(ctx, "unknown", "GSETUP_HTTP_INIT", "no http_request item in apphost context", logContext);
        return;
    }
    const auto& http_request = ctx.GetOnlyProtobufItem<NAppHostHttp::THttpRequest>("proto_http_request");

    auto requestPath = http_request.GetPath().substr(0, http_request.GetPath().find('?'));

    // cutting off /gproxy
    if (!requestPath.StartsWith("/gproxy")) {
        AddError(ctx, "unknown", "GSETUP_HTTP_INIT", "wrong path: " + requestPath, logContext);
        return;
    }
    const auto& requestMethod = requestPath.substr(7);

    auto processor = THttpProcessorFabric::THttpProcessorFabric::GetProcessorForHttpPath(requestMethod, ctx, logContext);
    processor->ProcessHttpInit();

    ctx.Flush();
    processor.release();
}

}   // namespace NGProxy
