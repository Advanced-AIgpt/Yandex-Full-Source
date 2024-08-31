#pragma once

#include <alice/wonderlogs/sdk/app_converter/protos/types.pb.h>

#include <util/generic/string.h>

namespace NAlice::NWonderSdk {

EAppType ConvertApp(const TString& app);

TString AppToString(EAppType app);

} // namespace NAlice::NWonderSdk
