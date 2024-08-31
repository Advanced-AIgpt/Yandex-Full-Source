#include "greetings_consts.h"

#include <alice/hollywood/library/scenarios/onboarding/proto/onboarding.pb.h>
#include <dj/services/alisa_skills/server/proto/client/onboarding_response.pb.h>
#include <dj/services/alisa_skills/server/proto/client/response.pb.h>

#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace NAlice::NHollywoodFw::NOnboarding {

    const TVector<TResponseItem> DEFAULT_ITEMS {
        TResponseItem{
            "onboarding_weather3", 
            "Погода на завтра", 
            "https://avatars.mds.yandex.net/get-dialogs/1535439/onboard_Wheather/mobile-logo-round-x2",
            "1535439/onboard_Wheather"
        },
        TResponseItem{
            "onboarding_music_fairy_tale2", 
            "Расскажи сказку", 
            "https://avatars.mds.yandex.net/get-dialogs/1525540/onboard_FairyTale/mobile-logo-round-x2",
            "1525540/onboard_FairyTale"
        },
        TResponseItem{
            "onboarding_find_poi2", 
            "Где поужинать?", 
            "https://avatars.mds.yandex.net/get-dialogs/1535439/onboard_Map/mobile-logo-round-x2",
            "1535439/onboard_Map"
        }
    };

    const TVector<TPromoSkillItem> DEFAULT_PROMO_SKILLS {
        TPromoSkillItem{
            "Умная камера",
            "SmartCam",
            "div_action_",
            "https://avatars.mds.yandex.net/get-dialogs/758954/onb_skill_camera/orig"
        },
        TPromoSkillItem{
            "Чат с Алисой",
            "AliceChat",
            "div_action_",
            "https://avatars.mds.yandex.net/get-dialogs/1027858/onb_skill_chat/orig"
        },
        TPromoSkillItem{
            "Что ты умеешь?",
            "Что ты умеешь?",
            "div_action_",
            "https://avatars.mds.yandex.net/get-dialogs/1027858/onb_skill_what/orig"
        },
        TPromoSkillItem{
            "Включи музыку",
            "Включи музыку",
            "div_action_",
            "https://avatars.mds.yandex.net/get-dialogs/758954/onb_skill_music/orig"
        },
        TPromoSkillItem{
            "Умный дом",
            "SmartHome",
            "div_action_",
            "https://avatars.mds.yandex.net/get-dialogs/998463/onb_skill_smart_home/orig"
        },
        TPromoSkillItem{
            "Что это играет?",
            "Что это играет?",
            "div_action_",
            "https://avatars.mds.yandex.net/get-dialogs/399212/onb_skill_shazam/orig"
        },
        TPromoSkillItem{
            "Игры",
            "Во что поиграть?",
            "div_action_",
            "https://avatars.mds.yandex.net/get-dialogs/758954/onb_skill_games/orig"
        },
    };

    NDJ::NAS::TServiceResponse GetOldResponseFallbackValues() {
        NDJ::NAS::TServiceResponse response;

        for (auto defaultItem : DEFAULT_ITEMS) {
            NDJ::NAS::TItemDescription* item = response.AddItems();
            item->SetId(defaultItem.Id);
            item->SetActivation(defaultItem.Activation);
            item->SetLogoFgRoundImageUrl(defaultItem.LogoUrl);
            item->SetLogoAvatarId(defaultItem.LogoAvatarId);
        }

        return response;
    }

    NDJ::NAS::TOnboardingResponse GetNewResponseFallbackValues() {
        NDJ::NAS::TOnboardingResponse response;

        for (const auto& defaultItem : DEFAULT_ITEMS) {
            NDJ::NAS::TProtoItem* item = response.AddItems();
            item->SetId(defaultItem.Id);
            item->MutableResult()->SetTitle(defaultItem.Activation);
            item->MutableResult()->MutableLogo()->SetImageUrl(defaultItem.LogoUrl);
        }

        return response;
    }

    TGreetingsRenderProto GetPromoSkillsStub(std::size_t id, const bool isSearchApp) {
        TGreetingsRenderProto promoSkills;

        for (const auto& defaultPromo : DEFAULT_PROMO_SKILLS) {

            if (defaultPromo.Title == "Умный дом" && !isSearchApp) {
                // show smart home for SearchApp only
                continue;
            }

            TPromoSkill* promoSkill = promoSkills.AddPromoSkills();
            promoSkill->SetTitle(defaultPromo.Title);
            promoSkill->SetActivation(defaultPromo.Activation);
            promoSkill->SetDivActionId(defaultPromo.DivActionId + ToString(id++));
            promoSkill->SetImageUrl(defaultPromo.ImageUrl);
        }

        return promoSkills;
    }

}
