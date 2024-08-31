#pragma once

#include <alice/bass/forms/context/context.h>

#include <alice/bass/util/error.h>

namespace NBASS {

/** It finds the nearest city for the given coordinates.
 */
TResultValue LLToGeo(const TContext& ctx, double lat, double lon,
    NGeobase::TId* userId);

/* Find normalized road name by toponym or return error
 * Example: "ленинградка" => "ленинградское шоссе"
 */
TResultValue TextToRoadName(TContext& ctx, TStringBuf text, TString* roadName);

}
