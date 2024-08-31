#include "requests_helper_base.h"

#include <alice/hollywood/library/scenarios/music/cache/common.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>

#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusic {

namespace {

NAppHostHttp::THttpRequest ConstructProxyRequest(TScenarioHandleContext& ctx, const TStringBuf path,
                                        const TStringBuf name) {
    return THttpProxyNoRtlogRequestBuilder(path,
                                    ctx.RequestMeta,
                                    ctx.Ctx.Logger(),
                                    TString{name}).BuildAndMove();
}

NAppHostHttp::THttpRequest  ConstructMusicProxyRequest(TScenarioHandleContext& ctx, const TScenarioBaseRequestWrapper& request,
                                             const TStringBuf path, const TStringBuf name) {
    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    return TMusicRequestBuilder(path,
                                ctx.RequestMeta,
                                request.ClientInfo(),
                                ctx.Ctx.Logger(),
                                enableCrossDc,
                                TMusicRequestModeInfoBuilder().BuildAndMove(),
                                TString{name}).BuildAndMove();
}

} // namespace

TBeforeHttpRequestHelper::TBeforeHttpRequestHelper(TScenarioHandleContext& ctx)
    : Ctx_{ctx}
    , RequestAdded_{false}
{}

void TBeforeHttpRequestHelper::AddRequest(const NAppHostHttp::THttpRequest& request) {
    Y_ENSURE(!RequestAdded_);
    Ctx_.ServiceCtx.AddProtobufItem(request, GetRequestItemName());
    RequestAdded_ = true;
}

void TBeforeHttpRequestHelper::AddRequest(const TStringBuf path) {
    AddRequest(ConstructProxyRequest(Ctx_, path, Name()));
}

TAfterHttpRequestHelper::TAfterHttpRequestHelper(TScenarioHandleContext& ctx)
    : Ctx_{ctx}
    , ResponseLoaded_{false}
{}

const TMaybe<NAppHostHttp::THttpResponse>& TAfterHttpRequestHelper::TryGetResponse() const {
    if (!ResponseLoaded_) {
        Response_ = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(Ctx_.ServiceCtx, GetResponseItemName());
        ResponseLoaded_ = true;

        // log response
        if (Response_.Defined()) {
            const bool success = (Response_->GetStatusCode() >= 200 && Response_->GetStatusCode() < 300);
            LOG_INFO(Ctx_.Ctx.Logger()) << "Response " << GetResponseItemName() << " "
                             << (success ? "succeded" : "failed")
                             << " with the status code: " << Response_->GetStatusCode()
                             << ", body: " + Response_->GetContent();
        } else {
            LOG_WARNING(Ctx_.Ctx.Logger()) << "Response for " << GetResponseItemName() << " missing";
        }
    }
    return Response_;
}

const TMaybe<NJson::TJsonValue>& TAfterHttpJsonRequestHelper::TryGetResponse() const {
    if (!JsonResponseLoaded_) {
        if (const auto& httpResponse = TAfterHttpRequestHelper::TryGetResponse()) {
            const TStringBuf rawResponse = httpResponse->GetContent();
            NJson::TJsonValue value;
            if (NJson::ReadJsonFastTree(rawResponse, &value, /* throwOnError= */ false, /* notClosedBracketIsError= */ false)) {
                JsonResponse_ = std::move(value);
            }
        }
        JsonResponseLoaded_ = true;
    }
    return JsonResponse_;
}

bool TAfterHttpJsonRequestHelper::HasResponse() const {
    return TryGetResponse().Defined();
}

TBeforeMusicHttpRequestHelper::TBeforeMusicHttpRequestHelper(TScenarioHandleContext& ctx,
                                                             const TScenarioBaseRequestWrapper& request)
    : TBeforeHttpRequestHelper{ctx}
    , Request_{request}
    , MusicArgs_{nullptr}
{}

TBeforeMusicHttpRequestHelper::TBeforeMusicHttpRequestHelper(TScenarioHandleContext& ctx,
                                                             const TScenarioApplyRequestWrapper& request)
    : TBeforeHttpRequestHelper{ctx}
    , Request_{request}
    , MusicArgs_{&request.UnpackArgumentsAndGetRef<TMusicArguments>()}
{}

void TBeforeMusicHttpRequestHelper::SetUseCache(bool useCache) {
    UseCache_ = useCache;
}

void TBeforeMusicHttpRequestHelper::AddRequest(const TStringBuf path) {
    TBeforeHttpRequestHelper::AddRequest(ConstructMusicProxyRequest(Ctx_, Request_, path, Name()));
    if (UseCache_) {
        TInputCacheMeta inputCacheMeta;
        inputCacheMeta.SetUseCache(UseCache_);
        Ctx_.ServiceCtx.AddProtobufItem(inputCacheMeta, NCache::INPUT_CACHE_META_ITEM);
    }
}

} // NAlice::NHollywood::NMusic::NMusicSdk
