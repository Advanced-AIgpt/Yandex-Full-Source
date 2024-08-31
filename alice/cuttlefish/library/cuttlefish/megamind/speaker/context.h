#pragma once

#include <alice/cuttlefish/library/cuttlefish/common/datasync_parser.h>
#include <alice/cuttlefish/library/protos/personalization.pb.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/guest/guest_data.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/megamind/protos/guest/guest_data.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/library/blackbox/proto/blackbox.pb.h>
#include <alice/library/blackbox/blackbox.h>
#include <alice/library/json/json.h>
#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    struct TSpeakerContext {
        NAlice::TGuestData GuestUserData;
        NAlice::TGuestOptions GuestUserOptions;
    };

    struct TRefSpeakerContext {
        NAlice::TGuestOptions& GuestUserOptions;
        NAlice::TGuestData& GuestUserData;

        TRefSpeakerContext(
            NAlice::TGuestOptions& guestUserOptions,
            NAlice::TGuestData& guestUserData
        )
            : GuestUserOptions(guestUserOptions)
            , GuestUserData(guestUserData)
        {}
    };

    template<typename TContext>
    void EnrichFromMatchResult(TContext& context, NAliceProtocol::TMatchVoiceprintResult matchResult) {
        context.GuestUserOptions.Swap(matchResult.MutableGuestOptions());
    }

    template<typename TContext>
    void EnrichFromDatasyncResponse(TContext& context, const NAppHostHttp::THttpResponse& datasyncResponse) {
        TDatasyncResponseParser parser;
        parser.ParseDatasyncResponse(datasyncResponse);

        if (!parser.PersonalData.GetMap().empty()) {
            context.GuestUserData.SetRawPersonalData(NAlice::JsonToString(parser.PersonalData));
        }
    }

    template<typename TContext>
    void EnrichFromBlackboxResponse(TContext& context, const NAppHostHttp::THttpResponse& blackboxResponse) {
        auto error = TBlackBoxApi()
            .ParseFullInfo(blackboxResponse.GetContent())
            .OnResult([&context](const TBlackBoxApi::TFullUserInfo& userInfoProto) mutable {
                if (userInfoProto.HasUserInfo()) {
                    const TBlackBoxFullUserInfoProto::TUserInfo &blackboxUserInfo = userInfoProto.GetUserInfo();
                    NAlice::TBlackBoxUserInfo& guestUserInfo = *context.GuestUserData.MutableUserInfo();

                    guestUserInfo.SetUid(blackboxUserInfo.GetUid());
                    guestUserInfo.SetEmail(blackboxUserInfo.GetEmail());
                    guestUserInfo.SetFirstName(blackboxUserInfo.GetFirstName());
                    guestUserInfo.SetLastName(blackboxUserInfo.GetLastName());
                    guestUserInfo.SetPhone(blackboxUserInfo.GetPhone());
                    guestUserInfo.SetHasYandexPlus(blackboxUserInfo.GetHasYandexPlus());
                    guestUserInfo.SetIsStaff(blackboxUserInfo.GetIsStaff());
                    guestUserInfo.SetIsBetaTester(blackboxUserInfo.GetIsBetaTester());
                    guestUserInfo.SetHasMusicSubscription(blackboxUserInfo.GetHasMusicSubscription());
                    guestUserInfo.SetMusicSubscriptionRegionId(blackboxUserInfo.GetMusicSubscriptionRegionId());
                }
            });

        if (error.Defined()) {
            ythrow yexception() << error->Message();
        }
    }

}
