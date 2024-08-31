#include "blackbox.h"

namespace NAlice::NMegamind {

TBlackBoxUserInfo CreateBlackBoxData(const TBlackBoxFullUserInfoProto& bbResponse) {
    if (!bbResponse.HasUserInfo()) {
        return {};
    }

    const auto& srcUserInfo = bbResponse.GetUserInfo();

    TBlackBoxUserInfo userInfo;
    userInfo.SetUid(srcUserInfo.GetUid());
    userInfo.SetEmail(srcUserInfo.GetEmail());
    userInfo.SetFirstName(srcUserInfo.GetFirstName());
    userInfo.SetLastName(srcUserInfo.GetLastName());
    userInfo.SetPhone(srcUserInfo.GetPhone());
    userInfo.SetHasYandexPlus(srcUserInfo.GetHasYandexPlus());
    userInfo.SetIsStaff(srcUserInfo.GetIsStaff());
    userInfo.SetIsBetaTester(srcUserInfo.GetIsBetaTester());
    userInfo.SetHasMusicSubscription(srcUserInfo.GetHasMusicSubscription());
    userInfo.SetMusicSubscriptionRegionId(srcUserInfo.GetMusicSubscriptionRegionId());

    return userInfo;
}

} // namespace NAlice::NMegamind
