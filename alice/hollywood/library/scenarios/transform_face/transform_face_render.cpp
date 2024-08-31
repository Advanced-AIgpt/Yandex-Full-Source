#include "transform_face_render.h"

#include <extsearch/images/daemons/cbirdaemon2/response_proto/face_transform.pb.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/metrics/metrics.h>

namespace NAlice::NHollywood {

namespace {

// paths relative to root
constexpr TStringBuf STATUS = "cbirdaemon.face_transformation.Status";
constexpr TStringBuf TRANSFORMS = "cbirdaemon.face_transformation.Transform";
constexpr TStringBuf ORIGINAL = "cbirdaemon.face_transformation.OriginalCrop.sizes.orig.path";
constexpr TStringBuf ORIGINAL_PREVIEW = "cbirdaemon.face_transformation.OriginalCrop.sizes.preview.path";

// paths relative to transform object
// constexpr TStringBuf NAME = "Name";
// constexpr TStringBuf STYLE = "Style";
// constexpr TStringBuf RESULT = "JpegImage.sizes.orig.path";
constexpr TStringBuf RESULT_PREVIEW = "JpegImage.sizes.preview.path";
constexpr TStringBuf RESULT_WM = "Watermarked.sizes.orig.path";
// constexpr TStringBuf RESULT_WM_PREVIEW = "Watermarked.sizes.preview.path";

constexpr TStringBuf URL_PREFIX = "https://avatars.mds.yandex.net";

inline bool IsInvalid(const NJson::TJsonValue* value) {
    if (value && value->IsString()) {
        return false;
    }
    return true;
}

}

void TTransformFaceRenderHandle::Do(TScenarioHandleContext& ctx) const {
    NJson::TJsonValue responseItems;
    TTransformFaceContinueImpl impl(ctx);

    try {
        responseItems = RetireHttpResponseJson(ctx);
    } catch (const yexception& e) {
        LOG_ERR(ctx.Ctx.Logger()) << "cbirdaemon error: " << e;
        Fail(impl);
        return;
    }

    const auto* status = responseItems.GetValueByPath(STATUS);
    if (!status) {
        LOG_ERR(ctx.Ctx.Logger()) << "response JSON does not contain path: " << STATUS;
        Fail(impl);
        return;
    }

    if (!status->IsInteger()) {
        LOG_ERR(ctx.Ctx.Logger()) << "response JSON path " << STATUS << " is not int";
        Fail(impl);
        return;
    }

    if (status->GetInteger() == NImages::NCbirWebdaemon::TFaceTransformResponse_TStatus_NO_FACE_FOUND) {
        impl.RenderNoFaceFound();
        return;
    }

    const auto* transformsPtr = responseItems.GetValueByPath(TRANSFORMS);
    if (!transformsPtr || !transformsPtr->IsArray()) {
        LOG_ERR(ctx.Ctx.Logger()) << "response JSON path " << TRANSFORMS << " is absent or is not an array";
        Fail(impl);
        return;
    }

    const auto& transforms = transformsPtr->GetArray();
    if (transforms.empty()) {
        LOG_ERR(ctx.Ctx.Logger()) << "response JSON path " << TRANSFORMS << " is an empty array";
        Fail(impl);
        return;
    }

    if (transforms.size() > 1) {
        LOG_WARN(ctx.Ctx.Logger()) << "found more than one transform in cbirdaemon response";
    }

    const auto* originalPtr = responseItems.GetValueByPath(ORIGINAL);
    const auto* originalPreviewPtr = responseItems.GetValueByPath(ORIGINAL_PREVIEW);

    // const auto* resultPtr = transforms[0].GetValueByPath(RESULT);
    const auto* resultPreviewPtr = transforms[0].GetValueByPath(RESULT_PREVIEW);
    const auto* resultWmPtr = transforms[0].GetValueByPath(RESULT_WM);
    // const auto* resultWmPreviewPtr = transforms[0].GetValueByPath(RESULT_PREVIEW);

    // const auto* namePtr = transforms[0].GetValueByPath(NAME);
    // const auto* stylePtr = transforms[0].GetValueByPath(STYLE);

    if (IsInvalid(originalPtr) || IsInvalid(originalPreviewPtr) || IsInvalid(resultPreviewPtr) || IsInvalid(resultWmPtr)) {
        LOG_ERR(ctx.Ctx.Logger()) << "response JSON path " << TRANSFORMS << "[0]" << " or OriginalCrop contains invalid values";
        Fail(impl);
        return;
    }

    impl.RenderResult(
        URL_PREFIX + originalPtr->GetString(),
        URL_PREFIX + originalPreviewPtr->GetString(),
        URL_PREFIX + resultWmPtr->GetString(),
        URL_PREFIX + resultPreviewPtr->GetString());
}

void TTransformFaceRenderHandle::Fail(TTransformFaceContinueImpl &impl) const {
    NMonitoring::TLabels labels = ScenarioLabels(*this);
    labels.Add("name", "cbird_facetransform_error");
    impl.Ctx.Ctx.GlobalContext().Sensors().IncRate(labels);
}

} // namespace NAlice::NHollywood
