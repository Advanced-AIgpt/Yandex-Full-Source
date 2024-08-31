#include "exp_flags.h"

#include <util/string/type.h>


namespace NAlice::NCuttlefish::NExpFlags {

const TString DISREGARD_UAAS = "disregard_uaas";
const TString ONLY_100_PERCENT_FLAGS = "only_100_percent_flags";
const TString SEND_PROTOBUF_TO_MEGAMIND = "send_protobuf_to_megamind";

TMaybe<TString> GetExperimentValue(const NAliceProtocol::TRequestContext& requestCtx, const TStringBuf name) {
    for (const auto& [exp, _] : requestCtx.GetExpFlags()) {
        if (exp.size() >= name.size() + 1 && exp.StartsWith(name) && exp.at(name.size()) == '=') {
            return exp.substr(name.size() + 1);
        }
    }
    return Nothing();
}


bool ConductingExperiment(const NAliceProtocol::TRequestContext& requestCtx, const TString& key) {
    const auto& flags = requestCtx.GetExpFlags();
    const auto iter = flags.find(key);
    return iter != flags.end();
}


bool ParseExperiments(const NJson::TJsonValue& experimentsJson, NAlice::TExperimentsProto& experimentsProto) {
    auto& storageProto = *experimentsProto.MutableStorage();
    if (experimentsJson.IsMap()) {
        for (const auto& exp : experimentsJson.GetMap()) {
            auto& value = storageProto[exp.first];
            switch (exp.second.GetType()) {
                case NJson::EJsonValueType::JSON_INTEGER:
                    value.SetInteger(exp.second.GetIntegerSafe());
                    break;

                case NJson::EJsonValueType::JSON_UINTEGER:
                    value.SetInteger(exp.second.GetUIntegerSafe());
                    break;

                case NJson::EJsonValueType::JSON_DOUBLE:
                    value.SetNumber(exp.second.GetDoubleSafe());
                    break;

                case NJson::EJsonValueType::JSON_BOOLEAN:
                    value.SetBoolean(exp.second.GetBooleanSafe());
                    break;

                case NJson::EJsonValueType::JSON_STRING:
                    value.SetString(exp.second.GetStringSafe());
                    break;

                case NJson::EJsonValueType::JSON_NULL:
                case NJson::EJsonValueType::JSON_UNDEFINED:
                    break;

                case NJson::EJsonValueType::JSON_MAP:
                case NJson::EJsonValueType::JSON_ARRAY:
                    return false; // Invalid type
            }
        }
    } else if (experimentsJson.IsArray()) {
        // Backward compatibility for old applications.
        for (const auto& exp : experimentsJson.GetArray()) {
            if (!exp.IsString()) {
                return false; // Invalid type
            }
            storageProto[exp.GetString()].SetString("1");
        }
    } else if (experimentsJson.IsDefined()) {
        return false; // not dict nor array
    }

    return true;
}


bool ExperimentFlagHasTrueValue(
    const NAliceProtocol::TRequestContext& requestContext,
    const TString& exp
) {
    if (auto expValue = requestContext.GetExpFlags().find(exp); expValue != requestContext.GetExpFlags().end()) {
        // TODO(VOICESERV-4046) very dirty hack, fix me plz
        return IsTrue(expValue->second);
    }
    return false;
}

} // namespace NAlice::NCuttlefish::NExpFlags
