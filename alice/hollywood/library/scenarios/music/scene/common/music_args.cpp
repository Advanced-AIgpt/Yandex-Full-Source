#include "music_args.h"

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>

#include <alice/hollywood/library/biometry/client_biometry.h>
#include <alice/hollywood/library/crypto/aes.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

NHollywood::TMusicArguments MakeMusicArguments(const TRunRequest& request, bool isNewContentRequestedByUser) {
    NHollywood::TMusicArguments args;
    args.SetExecutionFlowType(NHollywood::TMusicArguments_EExecutionFlowType_ThinClientDefault);
    args.SetIsNewContentRequestedByUser(isNewContentRequestedByUser);
    if (const auto* ds = request.GetDataSource(EDataSourceType::BLACK_BOX)) {
        const auto& userInfo = ds->GetUserInfo();
        auto& accountStatus = *args.MutableAccountStatus();
        accountStatus.SetUid(userInfo.GetUid());
        accountStatus.SetHasPlus(userInfo.GetHasYandexPlus());
        accountStatus.SetHasMusicSubscription(userInfo.GetHasMusicSubscription());
        accountStatus.SetMusicSubscriptionRegionId(userInfo.GetMusicSubscriptionRegionId());
    }
    if (const auto* ds = request.GetDataSource(EDataSourceType::IOT_USER_INFO)) {
        args.MutableIoTUserInfo()->CopyFrom(ds->GetIoTUserInfo());
    }
    if (const auto* ds = request.GetDataSource(EDataSourceType::GUEST_OPTIONS)) {
        const auto& guestOptions = ds->GetGuestOptions();
        args.SetIsOwnerEnrolled(guestOptions.GetIsOwnerEnrolled());
        if (NHollywood::NMusic::IsClientBiometryModeRunRequest(TRTLogger::NullLogger(), request, &guestOptions)) {
            auto& guestCredentials = *args.MutableGuestCredentials();
            guestCredentials.SetUid(guestOptions.GetYandexUID());

            TString guestOAuthTokenEncrypted;
            const bool encrypted = NHollywood::NCrypto::AESEncryptWeakWithSecret(
                NHollywood::NMusic::MUSIC_GUEST_OAUTH_TOKEN_AES_ENCRYPTION_KEY_SECRET,
                guestOptions.GetOAuthToken(),
                guestOAuthTokenEncrypted
            );
            Y_ENSURE(encrypted, "Error while encrypting guest OAuth token");
            guestCredentials.SetOAuthTokenEncrypted(Base64Encode(guestOAuthTokenEncrypted));
        }
    }
    if (const auto* ds = request.GetDataSource(EDataSourceType::ENVIRONMENT_STATE)) {
        args.MutableEnvironmentState()->CopyFrom(ds->GetEnvironmentState());
    }
    return args;
}

} // namespace NAlice::NHollywoodFw::NMusic
