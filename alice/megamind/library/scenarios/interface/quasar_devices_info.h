#pragma once

#include <alice/megamind/library/util/status.h>

// Fwds.
namespace NAlice {
class TIoTUserInfo;
class TQuasarDevicesInfo;
} // namespace NAlice

namespace NAlice::NMegamind {

TStatus CreateQuasarDevicesInfo(const TIoTUserInfo& iotUserInfo, TQuasarDevicesInfo& qdi);

} // namespace NAlice::NMegamind
