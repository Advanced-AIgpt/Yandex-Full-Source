#pragma once

#include <alice/nlu/granet/lib/sample/entity.h>
#include <search/begemot/rules/external_markup/proto/external_markup.pb.h>
#include <search/begemot/rules/external_markup/proto/format.pb.h>
#include <util/generic/vector.h>

namespace NBg::NAliceEntityCollector {

// Geo entities for Alice native intents
TVector<NGranet::TEntity> CollectGeo(const NProto::TExternalMarkupProto& markup);

// Geo entities for PASkills
TVector<NGranet::TEntity> CollectPASkillsGeo(const NProto::TExternalMarkupProto& markup);

} // namespace NBg::NAliceEntityCollector
