#include "blackbox.h"

#include "node.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/request_builder.h>
#include <alice/megamind/library/apphost_request/util.h>

#include <alice/library/blackbox/blackbox.h>

#include <util/generic/strbuf.h>

namespace NAlice::NMegamind {

TStatus AppHostBlackBoxSetup(IAppHostCtx& ahCtx, TRequestComponentsView<TClientComponent> request) {
    const TString* clientIp = request.ClientIp();
    if (!clientIp) {
        // It is not an error. We don't ask BlackBox if there is not user ip.
        return Success();
    }

    TMaybe<TString> authToken;
    if (const auto* value = request.AuthToken()) {
        authToken.ConstructInPlace(*value);
    } else {
        return Success();
    }

    TAppHostHttpProxyMegamindRequestBuilder requestBuilder;
    if (auto err = PrepareBlackBoxRequest(requestBuilder, *clientIp, authToken)) {
        if (err->Code() == EBlackBoxErrorCode::NoRequest) {
            return Success();
        }
        return TError{TError::EType::Logic} << "Failed to prepare blackbox request: " << err->Code();
    }

    requestBuilder.SetScheme(NAppHostHttp::THttpRequest::EScheme::THttpRequest_EScheme_Https);
    requestBuilder.CreateAndPushRequest(ahCtx, AH_ITEM_BLACKBOX_HTTP_REQUEST_NAME);

    return Success();
}

TStatus AppHostBlackBoxPostSetup(IAppHostCtx& ahCtx, TBlackBoxFullUserInfoProto& fullInfo) {
    NAppHostHttp::THttpResponse responseProto;
    if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_BLACKBOX_HTTP_RESPONSE_NAME, responseProto)) {
        return TError{TError::EType::Logic} << "No blackbox response item found";
    }

    if (responseProto.GetStatusCode() != HTTP_OK) {
        return TError{TError::EType::Http} << "Blackbox response is not HTTP_OK: "
                                           << responseProto.ShortUtf8DebugString();
    }

    if (auto err = TBlackBoxApi{}.ParseFullInfo(responseProto.GetContent()).MoveTo(fullInfo)) {
        return TError{TError::EType::Parse} << err->Message();
    }

    Y_ASSERT(fullInfo.IsInitialized());

    ahCtx.ItemProxyAdapter().PutIntoContext(fullInfo, AH_ITEM_BLACKBOX);

    return Success();
}

} // namespace NAlice::NMegaamind
