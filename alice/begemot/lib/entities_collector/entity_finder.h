#pragma once

#include <alice/nlu/granet/lib/sample/entity.h>

#include <search/begemot/rules/entity_finder/proto/entity_finder.pb.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NBg::NAliceEntityCollector {

TVector<NGranet::TEntity> CollectEntityFinder(const NProto::TEntityFinderResult& entityFinderResult);

} // namespace NBg::NAliceEntityCollector
