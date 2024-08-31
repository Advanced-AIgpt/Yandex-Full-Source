#include "personal_intents.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/request_builder.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/kv_saas/request.h>

namespace NAlice::NMegamind {

TStatus AppHostPersonalIntentsSetup(IAppHostCtx& ahCtx, TRequestComponentsView<TClientComponent> view) {
    TAppHostHttpProxyMegamindRequestBuilder request;
    ESourcePrepareType status;
    if (auto err = NKvSaaS::CreatePersonalIntentsRequest(view, request).MoveTo(status)) {
        return std::move(*err);
    }

    if (status == ESourcePrepareType::Succeeded) {
        request.CreateAndPushRequest(ahCtx, AH_ITEM_PERS_INTENT_HTTP_REQUEST_NAME);
    }

    return Success();
}

TStatus AppHostPersonalIntentsPostSetup(IAppHostCtx& ahCtx, NKvSaaS::TPersonalIntentsResponse& protoResponse) {
    NAppHostHttp::THttpResponse httpResponse;
    if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_PERS_INTENT_HTTP_RESPONSE_NAME, httpResponse)) {
        return std::move(*err);
    }

    if (httpResponse.GetStatusCode() != HTTP_OK) {
        return TError{TError::EType::Critical} << "kvaas personal intents returned error: " << httpResponse.ShortUtf8DebugString();
    }

    if (auto err = NKvSaaS::ParseResponse<NKvSaaS::TPersonalIntentsResponse>(httpResponse.GetContent()).MoveTo(protoResponse)) {
        return std::move(*err);
    }

    return Success();
}

} // namespace NAlice::NMegaamind
