#include "serialized_grammar.h"

namespace NGranet {

void WriteGrammarToProto(const TGrammar::TConstRef& grammar, NProto::TSerializedGrammar* proto) {
    Y_ENSURE(proto);
    grammar->GetDomain().WriteToProto(proto->MutableDomain());
    TBuffer buffer;
    TBufferOutput output(buffer);
    grammar->Save(&output);
    proto->SetData(buffer.Data(), buffer.Size());
}

void WriteGrammarsToProto(const TVector<TGrammar::TConstRef>& grammars, NProto::TSerializedGrammars* proto) {
    Y_ENSURE(proto);
    for (const TGrammar::TConstRef& grammar : grammars) {
        WriteGrammarToProto(grammar, proto->AddGrammars());
    }
}

TGrammar::TConstRef ReadGrammarFromProto(const NProto::TSerializedGrammar& proto) {
    TMemoryInput input(proto.GetData().Data(), proto.GetData().Size());
    return TGrammar::LoadFromStream(&input);
}

TVector<TGrammar::TConstRef> ReadGrammarsForDomain(const NProto::TSerializedGrammars& protos,
    const TGranetDomain& domain)
{
    TVector<TGrammar::TConstRef> grammars;
    for (const NProto::TSerializedGrammar& proto : protos.GetGrammars()) {
        if (TGranetDomain::ReadFromProto(proto.GetDomain()) == domain) {
            grammars.push_back(ReadGrammarFromProto(proto));
        }
    }
    return grammars;
}

} // namespace NGranet
