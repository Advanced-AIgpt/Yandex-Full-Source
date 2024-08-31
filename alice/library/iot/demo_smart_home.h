#pragma once

#include "structs.h"

#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/protos/data/device/info.pb.h>
#include <alice/protos/data/location/group.pb.h>
#include <alice/protos/data/location/room.pb.h>


namespace NAlice::NIot {

const TIoTUserInfo* GetDemoSmartHome(ELanguage language);

}  // namespace NAlice::NIot
