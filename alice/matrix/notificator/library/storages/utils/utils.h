#pragma once

#include <alice/protos/api/matrix/user_device.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>


namespace NMatrix::NNotificator {

// For old tables, this shard key was used
// It's hard to migrate without downtime, so we just use this function
// TODO(ZION-69) Finally migrate
ui64 LegacyComputeShardKey(const TString& st);

TMaybe<NApi::TUserDeviceInfo::ESupportedFeature> TryParseSupportedFeatureFromString(const TString& supportedFeature);
TString SupportedFeatureToString(const NApi::TUserDeviceInfo::ESupportedFeature supportedFeature);

} // namespace NMatrix::NNotificator
