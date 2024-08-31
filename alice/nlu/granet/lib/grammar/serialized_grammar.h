#pragma once

#include "grammar.h"
#include <alice/nlu/granet/lib/grammar/proto/serialized_grammar.pb.h>

namespace NGranet {

void WriteGrammarToProto(const TGrammar::TConstRef& grammar, NProto::TSerializedGrammar* proto);
void WriteGrammarsToProto(const TVector<TGrammar::TConstRef>& grammars, NProto::TSerializedGrammars* proto);

TGrammar::TConstRef ReadGrammarFromProto(const NProto::TSerializedGrammar& proto);
TVector<TGrammar::TConstRef> ReadGrammarsForDomain(const NProto::TSerializedGrammars& proto, const TGranetDomain& domain);

} // namespace NGranet
