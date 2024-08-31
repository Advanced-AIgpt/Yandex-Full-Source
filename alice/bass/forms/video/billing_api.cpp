#include "billing_api.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/library/billing/billing.h>
#include <alice/library/network/headers.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/strbuf.h>

namespace NBASS {
namespace NVideo {
namespace {
constexpr TStringBuf REQUEST_CONTENT_BUY = "/billing/requestContentBuy";
constexpr TStringBuf REQUEST_CONTENT_PLAY = "/billing/requestContentPlay";
constexpr TStringBuf REQUEST_PLUS_AVAILABILITY_INFO = "/billing/requestPlus";

void SetJsonBody(NHttpFetcher::TRequest& request, const NSc::TValue& body) {
    request.SetBody(body.ToJson(), "POST");
    request.AddHeader("Content-Type", "application/json");
}
} // namespace

TBillingAPI::TBillingAPI(TContext& ctx)
    : Ctx(ctx)
    , MultiRequest(NHttpFetcher::MultiRequest()) {
}

TBillingAPI::TBillingAPI(TContext& ctx, NHttpFetcher::IMultiRequest::TRef multiRequest)
    : Ctx(ctx)
    , MultiRequest(multiRequest) {
}

NHttpFetcher::THandle::TRef TBillingAPI::GetPlusPromoAvailability() {
    NHttpFetcher::TRequestPtr request =
        Ctx.GetSources().QuasarBillingPromoAvailability(REQUEST_PLUS_AVAILABILITY_INFO).AttachRequest(MultiRequest);
    Y_ASSERT(request);

    SetupRequest(*request);
    AddCodecHeadersIntoBillingRequest(request);
    request->SetMethod("POST");

    return request->Fetch();
}


NHttpFetcher::THandle::TRef TBillingAPI::RequestContent(const TRequestContentOptions& options,
                                                        const NSc::TValue& contentItem,
                                                        const NSc::TValue& contentPlayPayload) {
    NHttpFetcher::TRequestPtr request;
    switch (options.Type) {
        case TRequestContentOptions::EType::Buy:
            request = Ctx.GetSources().QuasarBillingContentBuy(REQUEST_CONTENT_BUY).AttachRequest(MultiRequest);
            break;
        case TRequestContentOptions::EType::Play:
            request = Ctx.GetSources().QuasarBillingContentPlay(REQUEST_CONTENT_PLAY).AttachRequest(MultiRequest);
            request->AddCgiParam("startPurchaseProcess", options.StartPurchaseProcess ? "true" : "false");
            break;
    }

    Y_ASSERT(request);

    SetupRequest(*request);
    AddCodecHeadersIntoBillingRequest(request);
    request->AddCgiParam("contentItem", contentItem.ToJson());

    NSc::TValue body;
    body["contentPlayPayload"] = contentPlayPayload;
    SetJsonBody(*request, body);

    return request->Fetch();
}

void TBillingAPI::SetupRequest(NHttpFetcher::TRequest& request) {
    request.AddCgiParam("deviceId", Ctx.Meta().DeviceState().DeviceId());
    request.AddCgiParam("sendPush", "false");
    request.AddCgiParam("sendPersonalCards", "false");

    if (Ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_DISABLE_OAUTH) && Ctx.UserTicket().Defined()) {
        request.AddHeader(NAlice::NNetwork::HEADER_X_YA_USER_TICKET, *Ctx.UserTicket());
    } else {
        request.AddHeader("Authorization", Ctx.UserAuthorizationHeader());
    }
    request.AddHeader("User-Agent", Ctx.MetaClientInfo().UserAgent);
    request.AddHeader("X-Forwarded-For", Ctx.UserIP());
    request.AddHeader("X-Request-Id", Ctx.ReqId());

    if (const auto expFlagsJson = NAlice::NBilling::ExpFlagsToBillingHeader(Ctx.ExpFlags())) {
        LOG(INFO) << "Billing experiment flags json: " << *expFlagsJson << Endl;
        request.AddHeader("X-Yandex-ExpFlags", Base64Encode(*expFlagsJson));
    }
}

void TBillingAPI::AddCodecHeadersIntoBillingRequest(NHttpFetcher::TRequestPtr& request) const {
    NVideo::AddCodecHeadersIntoRequest(request, Ctx);
}

} // namespace NVideo
} // namespace NBASS
