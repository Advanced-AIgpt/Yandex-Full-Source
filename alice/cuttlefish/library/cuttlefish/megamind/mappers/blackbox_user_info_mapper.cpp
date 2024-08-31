#include "blackbox_user_info_mapper.h"

namespace NAlice::NCuttlefish::NAppHostServices::NMappers {

    void TBlackBoxUserInfoMapper::Map(const TBlackBoxFullUserInfoProto::TUserInfo& source, NAlice::TBlackBoxUserInfo& destination) {
        destination.SetUid(source.GetUid());
        destination.SetEmail(source.GetEmail());
        destination.SetFirstName(source.GetFirstName());
        destination.SetLastName(source.GetLastName());
        destination.SetPhone(source.GetPhone());
        destination.SetHasYandexPlus(source.GetHasYandexPlus());
        destination.SetIsStaff(source.GetIsStaff());
        destination.SetIsBetaTester(source.GetIsBetaTester());
        destination.SetHasMusicSubscription(source.GetHasMusicSubscription());
        destination.SetMusicSubscriptionRegionId(source.GetMusicSubscriptionRegionId());
    }

    void TBlackBoxUserInfoMapper::Map(const TBlackBoxFullUserInfoProto::TUserInfo& source, NAlice::TGuestOptions& destination) {
        destination.SetYandexUID(source.GetUid());
    }

} // namespace NAlice::NCuttlefish::NAppHostServices::NMappers
