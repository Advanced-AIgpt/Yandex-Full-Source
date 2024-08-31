#pragma once

#include "entities.h"
#include "indexer.h"
#include "query_parser.h"
#include "structs.h"

#include <alice/megamind/protos/common/iot.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash.h>
#include <util/string/builder.h>


namespace NAlice::NIot {

using TExpFlags = THashMap<TString, TMaybe<TString>>;

NSc::TValue MakeHypotheses(const TSmartHomeIndex& index,
                           const TIoTEntitiesInfo& entitiesInfo,
                           const ELanguage language,
                           const TExpFlags& expFlags = TExpFlags(),
                           TStringBuilder* logBuilder = nullptr);

} // namespace NAlice::NIot
