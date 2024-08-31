#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <library/cpp/scheme/scheme.h>

namespace NBASS {
namespace NExternalSkill {

bool DeviceSupportsAccountLinking(const TContext& ctx);

bool DeviceSupportsBilling(const TContext& ctx);

NSc::TValue CreateHookInterfaces(const TContext& ctx);

} // NExternalSkill
} // NBASS
