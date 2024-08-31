#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

namespace NAlice::NHollywood::NMusic {

// Base classes
class IBeforeRequestHelper {
protected:
    virtual TStringBuf GetRequestItemName() const = 0;
};

class IAfterRequestHelper {
protected:
    virtual TStringBuf GetResponseItemName() const = 0;
};

enum class ERequestPhase {
    Before,
    After,
};

class TBeforeHttpRequestHelper;
class TAfterHttpRequestHelper;

template<ERequestPhase RequestPhase,
         typename TBeforeHelper = TBeforeHttpRequestHelper,
         typename TAfterHelper = TAfterHttpRequestHelper>
using TRequestHelperChooser = std::conditional_t<RequestPhase == ERequestPhase::Before, TBeforeHelper, TAfterHelper>;

// HTTP-proxy helpers
class TBeforeHttpRequestHelper : public IBeforeRequestHelper {
public:
    TBeforeHttpRequestHelper(TScenarioHandleContext& ctx);
    void AddRequest(const NAppHostHttp::THttpRequest& request);
    void AddRequest(const TStringBuf path);

    virtual TStringBuf GetRtlogTokenKey() const { return PROXY_RTLOG_TOKEN_KEY_DEFAULT; };
    virtual TStringBuf Name() const = 0;

protected:
    TScenarioHandleContext& Ctx_;
    bool RequestAdded_;
};

class TAfterHttpRequestHelper : public IAfterRequestHelper {
public:
    TAfterHttpRequestHelper(TScenarioHandleContext& ctx);
    const TMaybe<NAppHostHttp::THttpResponse>& TryGetResponse() const;

protected:
    TScenarioHandleContext& Ctx_;
    mutable bool ResponseLoaded_;
    mutable TMaybe<NAppHostHttp::THttpResponse> Response_;
};

// JSON http proxy helpers
class TAfterHttpJsonRequestHelper : protected TAfterHttpRequestHelper {
public:
    using TAfterHttpRequestHelper::TAfterHttpRequestHelper;
    const TMaybe<NJson::TJsonValue>& TryGetResponse() const;
    bool HasResponse() const;

protected:
    mutable bool JsonResponseLoaded_ = false;
    mutable TMaybe<NJson::TJsonValue> JsonResponse_;
};

// Music-related proxy helpers
class TBeforeMusicHttpRequestHelper : private TBeforeHttpRequestHelper {
public:
    TBeforeMusicHttpRequestHelper(TScenarioHandleContext& ctx, const TScenarioBaseRequestWrapper& request);
    TBeforeMusicHttpRequestHelper(TScenarioHandleContext& ctx, const TScenarioApplyRequestWrapper& request);
    void SetUseCache(bool useCache = true); // slow music requests may be cached (if supported)
    void AddRequest(const TStringBuf path);

    virtual TStringBuf GetRtlogTokenKey() const { return MUSIC_REQUEST_RTLOG_TOKEN_ITEM; };

protected:
    const TScenarioBaseRequestWrapper& Request_;
    const TMusicArguments* MusicArgs_;

private:
    bool UseCache_ = false;
};

} // NAlice::NHollywood::NMusic::NMusicSdk
