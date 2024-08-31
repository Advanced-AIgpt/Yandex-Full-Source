#pragma once

#include <alice/megamind/protos/blackbox/blackbox.pb.h>

#include <alice/library/blackbox/proto/blackbox.pb.h>

namespace NAlice::NMegamind {

TBlackBoxUserInfo CreateBlackBoxData(const TBlackBoxFullUserInfoProto& bbResponse);

} // namespace NAlice::NMegamind
