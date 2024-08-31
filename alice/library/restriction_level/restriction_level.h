#pragma once

#include <alice/library/restriction_level/protos/content_settings.pb.h>

#include <util/generic/maybe.h>

namespace NAlice {

enum class EContentRestrictionLevel : int {
    Medium /* "medium" */,
    Children /* "children" */,
    Without /* "without" */,
    Safe /* "safe" */
};

EContentSettings CalculateContentRestrictionLevel(EContentSettings contentSettings, TMaybe<ui32> filtrationLevel = Nothing());

EContentRestrictionLevel GetContentRestrictionLevel(EContentSettings level);

EContentSettings ToProtoType(EContentRestrictionLevel level);

} // namespace NAlice
