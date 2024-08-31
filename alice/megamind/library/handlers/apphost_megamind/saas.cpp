#include "saas.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/request_builder.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/requestctx/requestctx.h>
#include <alice/megamind/library/saas/saas.h>

namespace NAlice::NMegamind {

TStatus AppHostSaasSkillDiscoverySetup(IAppHostCtx& ahCtx, const TString& utterance) {
    TAppHostHttpProxyMegamindRequestBuilder requestBuilder;
    const auto result = NSaasSearch::PrepareSaasRequest(
        utterance,
        ahCtx.GlobalCtx().Config().GetSaasSkillDiscoveryOptions(),
        requestBuilder);

    if (result.IsSuccess()) {
        requestBuilder.CreateAndPushRequest(ahCtx, AH_ITEM_SAAS_SKILL_DISCOVERY_HTTP_REQUEST_NAME);
    }

    return result.Status();
}

TStatus AppHostSaasSkillDiscoveryPostSetup(IAppHostCtx& ahCtx,
                                           NScenarios::TSkillDiscoverySaasCandidates& protoResponse) {
    NAppHostHttp::THttpResponse responseProto;
    if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_SAAS_SKILL_DISCOVERY_HTTP_RESPONSE_NAME, responseProto)) {
        return std::move(*err);
    }

    if (responseProto.GetStatusCode() != HTTP_OK) {
        return TError{TError::EType::Http} << "Saas skill discovery response error: "
                                           << responseProto.ShortUtf8DebugString();
    }

    protoResponse =
        NSaasSearch::ParseSaasSkillDiscoveryReply(responseProto.GetContent(),
                                                  ahCtx.GlobalCtx().Config().GetSaasSkillDiscoveryOptions());

    return Success();
}

} // namespace NAlice::NMegaamind
