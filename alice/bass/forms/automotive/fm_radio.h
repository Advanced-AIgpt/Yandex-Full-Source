#pragma once

#include <alice/bass/forms/vins.h>
#include <alice/bass/forms/context/fwd.h>

#include <alice/bass/libs/radio/fmdb.h>

#include <util/system/types.h>

namespace NBASS {
namespace NAutomotive {

TResultValue HandleFMRadio(TContext& ctx, const TFMRadioDatabase& fmdb);

i32 GetRegionId(TContext& ctx, const TFMRadioDatabase& fmdb);

} // NAutomotive
} // NBASS
