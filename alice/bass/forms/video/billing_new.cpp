#include "billing_new.h"

#include "billing.h"
#include "play_video.h"
#include "push_message.h"
#include "utils.h"
#include "vh_player.h"
#include "web_os_helper.h"

#include <alice/bass/libs/video_common/defs.h>

namespace NBASS::NVideo {

namespace {

inline constexpr TStringBuf CGI_EPISODE_ID = "episode_id";
inline constexpr TStringBuf CGI_PROMO_CHECK = "promo_check";

void SendPushAndShowPayPushScreen(const TCandidateToPlay& candidate, TContext& ctx, const TVhPlayerData& playerData) {
    const auto candidateCurr = candidate.Curr.Scheme();
    const auto candidateParent = candidate.Parent.Scheme();
    auto item = candidateCurr;

    TCgiParameters cgi;
    TMaybe<NVideo::TVideoItemConstScheme> tvShowItem;
    if (candidateParent.Type() == ToString(EItemType::TvShow)) {
        cgi.InsertUnescaped(CGI_EPISODE_ID, playerData.GetPlayableVhPlayerData().Uuid);
        tvShowItem = candidateParent;
        item = candidateParent;
    }

    if (playerData.PurchaseTag == "plus" || playerData.PurchaseTag == "kp-basic") {
        cgi.InsertUnescaped(CGI_PROMO_CHECK, "basic");
    } else if (playerData.PurchaseTag == "kp-amediateka") {
        cgi.InsertUnescaped(CGI_PROMO_CHECK, "premium");
    }
    AddBuyPushMessageResponseDirective(ctx, item, cgi);

    if (!ctx.HasExpFlag(FLAG_VIDEO_USE_OLD_BUY_PUSH_SCREEN) && // and check webview support like in ShowDescription()
        !ctx.HasExpFlag(FLAG_DISABLE_VIDEO_WEBVIEW_VIDEO_ENTITY))
    {
        AddShowBuyPushScreenResponseDirective(ctx, item, cgi);
    } else {
        NVideo::TShowPayScreenCommandData commandData;
        const auto provider = CreateProvider(PROVIDER_KINOPOISK, ctx);
        NVideo::PrepareShowPayScreenCommandData(candidateCurr, tvShowItem, *provider, commandData);
        AddShowPayPushScreenCommand(ctx, commandData);
    }
}

} // namespace

NHttpFetcher::THandle::TRef MakeVhPlayerRequestForCandidate(const TCandidateToPlay& candidate, const TContext& ctx) {
    const auto& candidateCurr = candidate.Curr;
    const auto& candidateParent = candidate.Parent;
    bool useItemProviderId = candidateCurr->Type() != ToString(NVideo::EItemType::TvShowEpisode) ||
                             candidateParent->Type() != ToString(NVideo::EItemType::TvShow) ||
                             IsTvShowEpisodeQuery(ctx);
    // use item itemId when it's film or tv show without particular season/episode in request
    TStringBuf itemId = useItemProviderId ? candidateCurr->ProviderItemId() : candidateParent->ProviderItemId();
    return CreateVhPlayerRequest(ctx, itemId);
}

TResultValue TryPlayItemByVhResponse(const TCandidateToPlay& candidate, TContext& ctx, ESendPayPushMode sendPayPushMode,
                                     bool& isItemAvailable) {
    NHttpFetcher::THandle::TRef vhPlayerRequest = MakeVhPlayerRequestForCandidate(candidate, ctx);
    TMaybe<TVhPlayerData> playerData = GetVhPlayerDataByVhPlayerRequest(vhPlayerRequest);
    if (!playerData.Defined()) {
        LOG(INFO) << "(Billing new) Can not get vh response for item";
        return TError(TError::EType::VHERROR, "Failed to get vh response.");
    }
    isItemAvailable = playerData->HasActiveLicense;

    if (isItemAvailable) {
        // We can play this item
        const auto provider = CreateProvider(PROVIDER_KINOPOISK, ctx);
        return PlayVideoAndAddAttentions(candidate, *provider, ctx, Nothing(), vhPlayerRequest);
    } else {
        // Payment is required
        bool shouldSendPush = sendPayPushMode != ESendPayPushMode::DontSend;
        LOG(INFO) << "Sending pay push is " << (shouldSendPush ? "" : "not ") << "requested" << Endl;
        if (shouldSendPush) {
            if (IsTvOrModuleRequest(ctx)) {
                LOG(INFO) << "TV should not send pay push. Instead TV do buyings by remote" << Endl; // SMARTTVBACKEND-918
                ctx.AddAttention(NVideo::ATTENTION_TV_PAYMENT_WITHOUT_PUSH);
            } else if (ctx.MetaClientInfo().IsLegatus()) {
                AddWebOSLaunchAppCommandForPaymentScreen(ctx, candidate);
                ctx.AddAttention(NVideo::ATTENTION_LEGATUS_PAYMENT_WITHOUT_PUSH);
            } else {
                // Send Push
                SendPushAndShowPayPushScreen(candidate, ctx, *playerData);
                ctx.AddAttention(ATTENTION_SEND_PAY_PUSH_DONE);
            }
            return ResultSuccess();
        } else {
            // Do not send push
            ctx.AddAttention(ATTENTION_PAID_CONTENT);
        }
    }
    return {};
}

} // namespace NBASS
