#include "scenario_proto_serializer.h"

#include "meta.h"

#include <alice/megamind/library/common/defs.h>

#include <util/generic/vector.h>

namespace NAlice::NMegamind {

namespace {

[[nodiscard]] google::protobuf::Struct DropKeys(const google::protobuf::Struct& data, const TVector<TString>& keys) {
    google::protobuf::Struct result;
    result.CopyFrom(data);
    auto* fields = result.mutable_fields();
    for (const auto& key : keys) {
        if (auto entry = fields->find(key); entry != fields->end()) {
            fields->erase(entry);
        }
    }
    return result;
}

} // namespace

// static
NScenarios::TCallbackDirective TScenarioProtoSerializer::SerializeDirective(const TCallbackDirectiveModel& model) {
    NScenarios::TCallbackDirective directive{};
    directive.SetName(model.GetName());
    directive.SetIgnoreAnswer(model.GetIgnoreAnswer());
    directive.SetIsLedSilent(model.GetIsLedSilent());
    *directive.MutablePayload() =
        DropKeys(model.GetPayload(), {TString{REQUEST_ID_JSON_KEY}, TString{SCENARIO_NAME_JSON_KEY}});
    return directive;
}

} // namespace NAlice::NMegamind
