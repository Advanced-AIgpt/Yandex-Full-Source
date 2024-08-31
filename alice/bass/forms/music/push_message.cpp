#include "push_message.h"

#include <alice/bass/forms/common/personal_data.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/video_common/defs.h>

namespace NBASS::NMusic {

namespace {

constexpr TStringBuf PROMO_PUSH_TAG_PREFIX = "QUASAR_BASS.PROMO_PERIOD.yandexplus.";
const TString PROMO_PUSH_ID = "alice_quasar_bass_promo_period_yandexplus";
const TString PROMO_PUSH_THROTTLE_POLICY = "quasar_default_install_id";
const TString PROMO_PUSH_TITLE = "Яндекс.Плюс";
const TString PROMO_PUSH_TEXT = "Нажмите для активации " + PROMO_PUSH_TITLE;

} // namespace

void AddPushMessageResponseDirective(TContext& ctx, const TPushMessageParams& pushMessageParams) {
    NSc::TValue payload;
    payload["title"].SetString(pushMessageParams.Title);
    payload["text"].SetString(pushMessageParams.Text);
    payload["url"].SetString(pushMessageParams.Url);
    payload["tag"].SetString(pushMessageParams.Tag);
    payload["id"].SetString(pushMessageParams.Id);
    payload["throttle"].SetString(pushMessageParams.ThrottlePolicy);

    ctx.AddUniProxyAction(NAlice::NVideoCommon::COMMAND_SEND_PUSH, payload);
}

void AddPromoSendPushDirective(TContext& ctx, NAlice::NBilling::TPromoAvailability promoAvailability) {
    TPushMessageParams promoPushParams;

    TString uid;
    if (NBASS::TPersonalDataHelper(ctx).GetUid(uid)) {
        promoPushParams.Tag = TString::Join(PROMO_PUSH_TAG_PREFIX, uid);
    } else {
        promoPushParams.Tag = ToString(PROMO_PUSH_TAG_PREFIX);
        LOG(ERR) << "Failed to get uid from context, sending push with tag " << promoPushParams.Tag << Endl;
    }

    promoPushParams.Id = PROMO_PUSH_ID;
    promoPushParams.Title = PROMO_PUSH_TITLE;
    promoPushParams.Text = PROMO_PUSH_TEXT;
    promoPushParams.ThrottlePolicy = PROMO_PUSH_THROTTLE_POLICY;
    promoPushParams.Url = promoAvailability.ActivatePromoUri;

    AddPushMessageResponseDirective(ctx, promoPushParams);
}

}
