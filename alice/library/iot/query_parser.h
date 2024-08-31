#pragma once

#include "entities.h"
#include "structs.h"

#include <util/generic/vector.h>
#include <util/string/builder.h>


namespace NAlice::NIot {

TRawParsingHypotheses Parse(const TIoTEntitiesInfo& entitiesInfo, ELanguage language, TStringBuilder* logBuilder = nullptr);

} // namespace NAlice::NIot
