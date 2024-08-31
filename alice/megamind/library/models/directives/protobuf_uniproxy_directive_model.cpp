#include "protobuf_uniproxy_directive_model.h"

namespace NAlice::NMegamind {

TProtobufUniproxyDirectiveModel::TProtobufUniproxyDirectiveModel(
    const NScenarios::TCancelScheduledActionDirective& directive
)
    : Name_{"TCancelScheduledActionDirective"}
    , Directives_{directive}
{
}

TProtobufUniproxyDirectiveModel::TProtobufUniproxyDirectiveModel(
    const NScenarios::TEnlistScheduledActionDirective& directive
)
    : Name_{"TEnlistScheduledActionDirective"}
    , Directives_{directive}
{
}

void TProtobufUniproxyDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TProtobufUniproxyDirectiveModel::GetName() const {
    return Name_;
}

EDirectiveType TProtobufUniproxyDirectiveModel::GetType() const {
    return EDirectiveType::ProtobufUniproxyAction;
}

void TProtobufUniproxyDirectiveModel::SetUniproxyDirectiveMeta(NSpeechKit::TUniproxyDirectiveMeta uniproxyDirectiveMeta) {
    UniproxyDirectiveMeta = std::move(uniproxyDirectiveMeta);
}

const NSpeechKit::TUniproxyDirectiveMeta* TProtobufUniproxyDirectiveModel::GetUniproxyDirectiveMeta() const {
    return UniproxyDirectiveMeta.Get();
}

} // namespace NAlice::NMegamind
