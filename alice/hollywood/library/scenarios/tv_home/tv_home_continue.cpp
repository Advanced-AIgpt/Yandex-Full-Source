#include "tv_home_continue.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/metrics/metrics.h>

#include <util/stream/str.h>

#include <library/cpp/uri/uri.h>
#include <library/cpp/json/json_value.h>

#include <alice/protos/data/video/tv_backend_request.pb.h>

using namespace NAlice::NScenarios;
using namespace NJson;

namespace NAlice::NHollywood {

void TTvHomeContinueHandle::Do(TScenarioHandleContext& ctx) const {
    const NScenarios::TScenarioApplyRequest requestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper applyRequestWrapper(requestProto, ctx.ServiceCtx);
    const NAlice::TTvBackendRequest backendRequest(applyRequestWrapper.UnpackArguments<NAlice::TTvBackendRequest>());
    const auto path = backendRequest.GetPath();

    THttpProxyRequestBuilder httpRequestBuilder(path, ctx.RequestMeta, ctx.Ctx.Logger());
    auto httpRequest = httpRequestBuilder.Build();
    AddHttpRequestItems(ctx, httpRequest);
}

} // namespace NAlice::NHollywood
