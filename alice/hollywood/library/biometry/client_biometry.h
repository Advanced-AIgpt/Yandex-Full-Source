#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/logger/logger.h>

#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/protos/endpoint/capabilities/bio/capability.pb.h>

namespace NAlice::NHollywood {

bool DeviceSupportsBiometry(const TEnvironmentState& environmentState, TStringBuf deviceId);

bool SupportsClientBiometry(const TScenarioRunRequestWrapper& request);

bool SupportsClientBiometry(const NAlice::NHollywoodFw::TRunRequest& request);

bool ValidateGuestOptionsDataSource(TRTLogger& logger, const NAlice::TGuestOptions& guestOptions);

} // namespace NAlice::NHollywood
