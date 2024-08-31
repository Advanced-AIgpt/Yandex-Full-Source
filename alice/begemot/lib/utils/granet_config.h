#pragma once

#include <search/begemot/rules/granet_config/proto/granet_config.pb.h>
#include <alice/nlu/granet/lib/grammar/multi_grammar.h>

namespace NBg {

NProto::TGranetConfig MakeGranetConfig(const NGranet::TMultiGrammar::TConstRef& grammar);

} // namespace NBg
