#pragma once

#include <alice/library/blackbox/proto/blackbox.pb.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices::NMappers {

    class TBlackBoxUserInfoMapper {
    public:
        static void Map(const TBlackBoxFullUserInfoProto::TUserInfo& source, NAlice::TBlackBoxUserInfo& destination);
        static void Map(const TBlackBoxFullUserInfoProto::TUserInfo& source, NAlice::TGuestOptions& destination);
    };

} // namespace NAlice::NCuttlefish::NAppHostServices::NMappers
