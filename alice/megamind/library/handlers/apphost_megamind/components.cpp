#include "components.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/protos/uniproxy_request.pb.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/speechkit/request.h>

namespace NAlice::NMegamind {

// TFromAppHostInitContext ----------------------------------------------------
TFromAppHostInitContext::TFromAppHostInitContext(IAppHostCtx& ahCtx, const TCgiParameters& cgi, const THttpHeaders& headers, const TString& path)
    : TSpeechKitInitContext{cgi, headers, path, ahCtx.GlobalCtx().RngSeedSalt()}
    , AhCtx{ahCtx}
{
}

// TFromAppHostEventComponent -------------------------------------------------
// static
TErrorOr<TFromAppHostEventComponent> TFromAppHostEventComponent::Create(TFromAppHostInitContext& initCtx) {
    return TFromAppHostEventComponent{initCtx.EventProtoPtr};
};

// TFromAppHostClientComponent ------------------------------------------------
// static
TErrorOr<TFromAppHostClientComponent> TFromAppHostClientComponent::Create(TFromAppHostInitContext& initCtx) {
    NMegamindAppHost::TClientItem item;
    if (auto err = GetFirstProtoItem(initCtx.AhCtx.ItemProxyAdapter(), AH_ITEM_SKR_CLIENT_INFO, item)) {
        return std::move(*err);
    }
    return TFromAppHostClientComponent(std::move(item));
};

TFromAppHostClientComponent::TFromAppHostClientComponent(const NMegamindAppHost::TClientItem& item)
    : ExpFlags_{CreateExpFlags(item.GetExperiments())}
    , ClientFeatures_{item.GetClientInfo(), item.GetSupportedFeatures(), item.GetUnsupportedFeatures(), ExpFlags_}
    , DeviceState_{item.GetDeviceState()}
{
    if (item.HasAuthToken()) {
        AuthToken_.ConstructInPlace(item.GetAuthToken());
    }

    if (item.HasClientIp()) {
        ClientIp_.ConstructInPlace(item.GetClientIp());
    }
}

// TFromAppHostSpeechKitRequest ------------------------------------------------
TFromAppHostSpeechKitRequest::TFromAppHostSpeechKitRequest(TFromAppHostInitContext& initCtx, TStatus& status)
    : TRequestComposite{initCtx, status}
{
}

TErrorOr<TFromAppHostSpeechKitRequest::TPtr> TFromAppHostSpeechKitRequest::Create(IAppHostCtx& ahCtx) {
    NMegamindAppHost::TUniproxyRequestInfoProto uniproxyItemRequest;
    if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_UNIPROXY_REQUEST, uniproxyItemRequest)) {
        return std::move(*err);
    }

    const TCgiParameters cgi{uniproxyItemRequest.GetCgi()};
    THttpHeaders headers;
    for (const auto& h : uniproxyItemRequest.GetHeaders()) {
        headers.AddHeader({ h.GetName(), h.GetValue() });
    }

    TFromAppHostInitContext initCtx{ahCtx, cgi, headers, uniproxyItemRequest.GetUri()};

    if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_SPEECHKIT_REQUEST, *initCtx.Proto)) {
        return std::move(*err);
    }

    if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_SKR_EVENT, *initCtx.EventProtoPtr)) {
        return std::move(*err);
    }

    TStatus status;
    auto skrComposite = MakeSimpleShared<TFromAppHostSpeechKitRequest>(initCtx, status);
    if (status) {
        return std::move(*status);
    }
    return skrComposite;
}

} // namespace NAlice::NMegamind
