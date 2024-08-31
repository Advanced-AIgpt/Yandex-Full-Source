#include "experiments.h"

#include <alice/library/json/json.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/string/type.h>

namespace NAlice::NHollywood {

TExpFlags ExpFlagsFromProto(const google::protobuf::Struct& experiments) {
    TExpFlags result;
    for (const auto& exp : experiments.fields()) {
        result[exp.first] = exp.second.string_value();
    }
    return result;
}

bool IsExpFlagTrue(const TExpFlags& expFlags, const TStringBuf expFlagKey) {
    const TString expFlagValue = expFlags.Value(expFlagKey, Nothing()).GetOrElse(Default<TString>());
    return IsTrue(expFlagValue);
}

} // namespace NAlice::NHollywood
