#include "http_output.h"

#include "http_processor.h"

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/gproxy/library/protos/metadata.pb.h>
#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>


namespace NGProxy {

void TGSetupHttpOutput::Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext) {
    NAppHostHttp::THttpResponse response;

    if (!ctx.HasProtobufItem("response")) {
        AddError(ctx, "unknown", "GSETUP_HTTP_OUTPUT", "no response item in apphost context", logContext);
        return;
    }
    const auto& http_request = ctx.GetOnlyProtobufItem<NAppHostHttp::THttpRequest>("proto_http_request");

    auto requestPath = http_request.GetPath().substr(0, http_request.GetPath().find('?'));

    // cutting off /gproxy
    const auto& requestMethod = requestPath.substr(requestPath.find('y') + 1);

    auto processor = THttpProcessorFabric::THttpProcessorFabric::GetProcessorForHttpPath(requestMethod, ctx, logContext);
    processor->ProcessHttpOutput();

    ctx.Flush();
    processor.release();
}

}   // namespace NGProxy
