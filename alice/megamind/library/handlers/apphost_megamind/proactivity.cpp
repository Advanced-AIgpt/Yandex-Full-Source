#include "proactivity.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/request_builder.h>

namespace NAlice::NMegamind {

TStatus AppHostProactivitySetup(IAppHostCtx& ahCtx,
                                const IContext& ctx,
                                const TRequest& requestModel,
                                const TProactivityStorage& storage,
                                const TScenarioToRequestFrames& scenarioToFrames)
{
    TAppHostHttpProxyMegamindRequestBuilder request;
    TMaybe<ESourcePrepareType> prepareStatus;
    if (auto err = PrepareSkillProactivityRequest(ctx, requestModel, scenarioToFrames, storage, request).MoveTo(prepareStatus)) {
        return std::move(*err);
    } else if (prepareStatus == ESourcePrepareType::Succeeded) {
        request.SetMethod(NAppHostHttp::THttpRequest::Post);
        request.CreateAndPushRequest(ahCtx, AH_ITEM_PROACTIVITY_HTTP_REQUEST_NAME);
        ahCtx.ItemProxyAdapter().AddFlag(AH_FLAG_EXPECT_PROACTIVITY_RESPONSE);
    }

    return Success();
}

TStatus AppHostProactivityPostSetup(IAppHostCtx& ahCtx, NDJ::NAS::TProactivityResponse& proactivityResponse) {
    TAhHttpResponse responseProto;
    if (auto err = ahCtx.ItemProxyAdapter().GetFromContext<TAhHttpResponse>(AH_ITEM_PROACTIVITY_HTTP_RESPONSE_NAME).MoveTo(responseProto)) {
        return std::move(*err);
    }

    if (responseProto.GetStatusCode() != HTTP_OK) {
        return TError{TError::EType::Http} << responseProto.ShortUtf8DebugString();
    } else {
       ParseSkillProactivityResponse(responseProto.GetContent(), proactivityResponse);
       return Success();
    }
}

} // namespace NAlice::NMegamind
