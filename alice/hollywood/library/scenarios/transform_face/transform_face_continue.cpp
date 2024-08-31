#include "transform_face_continue.h"
#include "transform_face_impl.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/metrics/metrics.h>

#include <util/stream/str.h>
#include <library/cpp/uri/uri.h>
#include <library/cpp/json/json_value.h>

using namespace NAlice::NScenarios;
using namespace NJson;

namespace NAlice::NHollywood {

void TTransformFaceContinueHandle::Do(TScenarioHandleContext& ctx) const {
    TTransformFaceContinueImpl impl(ctx);

    TJsonMap requestJson{
        {"ImageInfo", TJsonMap{
            {"Engine", "GPU"},
        }},
        {"FaceTransformation", TJsonArray{
            TJsonMap{
                {"Name", impl.Transform.GetName()},
                {"Style", impl.Transform.GetStyle()},
                {"Engine", "GPU"},
                {"OriginalCrop", true},
                {"Watermark", true},
            },
        }},
    };

    TString request;
    TStringOutput stream(request);
    stream << "images_cbirdaemon_request=";
    WriteJson(&stream, &requestJson);

    TCgiParameters params = {
        std::make_pair("cbird", "71"),
        std::make_pair("type", "json"),
        std::make_pair("url", impl.Transform.GetImage()),
        std::make_pair("flags", request),
    };
    auto path = TString("?") + params.Print();
    auto httpRequest = PrepareHttpRequest(path, impl.Ctx.RequestMeta, impl.Ctx.Ctx.Logger());

    AddHttpRequestItems(impl.Ctx, httpRequest);
}

} // namespace NAlice::NHollywood
