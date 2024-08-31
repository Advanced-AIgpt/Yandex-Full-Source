#pragma once

#include <alice/nlu/granet/lib/sample/entity.h>
#include <search/begemot/rules/external_markup/proto/external_markup.pb.h>
#include <search/begemot/rules/external_markup/proto/format.pb.h>
#include <util/generic/vector.h>

namespace NBg::NAliceEntityCollector {

TVector<NGranet::TEntity> CollectPASkillsDate(const NProto::TExternalMarkupProto& markup);

} // namespace NBg::NAliceEntityCollector
