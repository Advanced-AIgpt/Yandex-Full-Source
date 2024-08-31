#include "flags_json.h"
#include "utils.h"

#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <util/generic/ymath.h>
#include <util/string/builder.h>
#include <util/string/cast.h>


namespace NVoice::NExperiments {

    bool MakeFlagsList(
        TString* dst,
        const NJson::TJsonValue& fjResponseMain,
        TString expBoxes,
        const TStringBuf handler
    ) {
        NJson::TJsonValue formatedFlags = fjResponseMain[handler]["flags"];
        formatedFlags.SetType(NJson::JSON_ARRAY);
        if (const NJson::TJsonValue::TArray& array = formatedFlags.GetArray(); array.size() > 0) {
            for (const NJson::TJsonValue& value : array) {
                if (!value.IsString()) {
                    return false;
                }
            }
            NJson::TJsonValue result;
            result[handler]["flags"] = std::move(formatedFlags);

            if (expBoxes.size() > 0) {
                result[handler]["boxes"] = std::move(expBoxes);
            }

            *dst = WriteJson(result, false, true);
            return true;
        }
        return false;
    }

    bool ParseFlagsInfoFromJsonResponse(NAliceProtocol::TFlagsInfo* result, const NJson::TJsonValue& response) {
        bool ok = true;

        long long expConfigVersion = -1;
        if (
            const NJson::TJsonValue& version = response["exp_config_version"];
            !version.GetInteger(&expConfigVersion)
        ) {
            if (!TryFromString(version.GetString(), expConfigVersion)) {
                ok = false;
            }
        }
        if (ok) {
            result->SetExpConfigVersion(expConfigVersion);
        }

        if (TString requestId; response["reqid"].GetString(&requestId)) {
            result->SetRequestId(std::move(requestId));
        } else {
            ok = false;
        }

        TString expBoxes;
        if (response["exp_boxes"].GetString(&expBoxes)) {
            result->SetExpBoxes(expBoxes);
        } else {
            ok = false;
        }

        const NJson::TJsonValue& mainBody = response["all"]["CONTEXT"]["MAIN"];

        using namespace NAlice::NCuttlefish::NExpFlags;
        if (!ParseExperiments(mainBody["VOICE"]["flags"], *(result->MutableVoiceFlags()))) {
            ok = false;
        }

        if (TString buffer; MakeFlagsList(&buffer, mainBody, expBoxes, "ASR")) {
            result->SetAsrFlagsJson(std::move(buffer));
        }
        if (TString buffer; MakeFlagsList(&buffer, mainBody, expBoxes, "BIO")) {
            result->SetBioFlagsJson(std::move(buffer));
        }

        {
            const NJson::TJsonValue& testIds = response["all"]["TESTID"];
            for (const auto& testId : testIds.GetArray()) {
                result->AddAllTestIds(testId.GetString());
            }
        }
        {
            const NJson::TJsonValue& testIds = response["ids"];
            for (const auto& testId : testIds.GetArray()) {
                result->AddExperimentalTestIds(testId.GetString());
            }
        }

        return ok;
    }

    bool ParseFlagsInfoFromRawResponse(NAliceProtocol::TFlagsInfo* result, TStringBuf rawFjResponse) {
        if (NJson::TJsonValue response; NJson::ReadJsonTree(rawFjResponse, &response, /*throw = */ false)) {
            return ParseFlagsInfoFromJsonResponse(result, response);
        }
        return false;
    }

    const NAliceProtocol::TFlagsInfo* GetFlagsInfoPtrFromSessionContextProto(
        const NAliceProtocol::TSessionContext& sessionContext
    ) {
        if (!(
            sessionContext.HasExperiments() &&
            sessionContext.GetExperiments().HasFlagsJsonData() &&
            sessionContext.GetExperiments().GetFlagsJsonData().HasFlagsInfo()
        )) {
            return nullptr;
        }
        return &sessionContext.GetExperiments().GetFlagsJsonData().GetFlagsInfo();
    }

    TFlagsJsonFlagsPtr MakeFlagsConstRefFromSessionContextProto(const NAliceProtocol::TSessionContext& sessionContext) {
        if (const NAliceProtocol::TFlagsInfo* infoPtr = GetFlagsInfoPtrFromSessionContextProto(sessionContext)) {
            // Here we just call constructor of TFlagsJsonFlagsConstRef. We do not own proto-pointer.
            return MakeIntrusive<TFlagsJsonFlagsConstRef>(infoPtr);
        }
        return nullptr;
    }

    TFlagsJsonFlagsPtr MakeFlagsHolderFromSessionContextProto(const NAliceProtocol::TSessionContext& sessionContext) {
        if (const NAliceProtocol::TFlagsInfo* infoPtr = GetFlagsInfoPtrFromSessionContextProto(sessionContext)) {
            return MakeIntrusive<TFlagsJsonFlagsHolder>(*infoPtr);
        }
        return nullptr;
    }

    NAlice::TExperimentsProto::TValue TFlagsJsonFlagsBase::ExperimentFlagValue(TStringBuf expName) const {
        const auto& voiceFlags = GetFlagsInfoProto()->GetVoiceFlags().GetStorage();
        const auto& pos = voiceFlags.find(TString(expName));
        if (pos != voiceFlags.end()) {
            return pos->second;
        }
        return {};
    }


    bool TFlagsJsonFlagsBase::ConductingExperiment(TStringBuf expName) const {
        const auto& value = ExperimentFlagValue(expName);
        switch (value.GetValueCase()) {
            case NAlice::TExperimentsProto::TValue::ValueCase::kBoolean: {
                return value.GetBoolean();
            }
            case NAlice::TExperimentsProto::TValue::ValueCase::kInteger: {
                return value.GetInteger() != 0;
            }
            case NAlice::TExperimentsProto::TValue::ValueCase::kNumber: {
                return !FuzzyEquals(value.GetNumber(), 0.0);
            }
            case NAlice::TExperimentsProto::TValue::ValueCase::kString: {
                return value.GetString().size() > 0;
            }
            default: {
                return false;
            }
        }
        return false;
    }

    #define EXP_VAL_GETTER(type, type_name, getter_name)                                                  \
        TMaybe<type> TFlagsJsonFlagsBase::GetExperimentValue##getter_name(const TString& expName) const { \
            const auto& value = ExperimentFlagValue(expName);                                             \
            if (value.GetValueCase() == NAlice::TExperimentsProto::TValue::ValueCase::k##type_name) {     \
                return value.Get##type_name();                                                            \
            }                                                                                             \
            return Nothing();                                                                             \
        }

    EXP_VAL_GETTER(bool, Boolean, Boolean)
    EXP_VAL_GETTER(int32_t, Integer, Integer)
    EXP_VAL_GETTER(double, Number, Float)
    EXP_VAL_GETTER(TString, String, String)

    #undef EXP_VAL_GETTER

    NJson::TJsonValue TFlagsJsonFlagsBase::GetVoiceFlagsJson() const {
        NJson::TJsonValue json;
        for (const auto& [key, value] : GetFlagsInfoProto()->GetVoiceFlags().GetStorage()) {
            switch (value.GetValueCase()) {
                case NAlice::TExperimentsProto::TValue::ValueCase::kBoolean: {
                    json[key] = value.GetBoolean();
                    break;
                }
                case NAlice::TExperimentsProto::TValue::ValueCase::kInteger: {
                    json[key] = value.GetInteger();
                    break;
                }
                case NAlice::TExperimentsProto::TValue::ValueCase::kNumber: {
                    json[key] = value.GetNumber();
                    break;
                }
                case NAlice::TExperimentsProto::TValue::ValueCase::kString: {
                    json[key] = value.GetString();
                    break;
                }
                default: {
                    break;
                }
            }
        }
        return json;
    }

    #define GETTER(type, variable_name, method_name)                                               \
        TMaybe<type> TFlagsJsonFlagsBase::method_name() const {                                    \
            if (auto flagsInfoProto = GetFlagsInfoProto(); flagsInfoProto->Has##variable_name()) { \
                return flagsInfoProto->Get##variable_name();                                       \
            }                                                                                      \
            return Nothing();                                                                      \
        }

    GETTER(int32_t, ExpConfigVersion, GetExpConfigVersion)
    GETTER(TString, AsrFlagsJson, GetAsrAbFlagsSerializedJson)
    GETTER(TString, BioFlagsJson, GetBioAbFlagsSerializedJson)
    GETTER(TString, ExpBoxes, GetExpBoxes)
    GETTER(TString, RequestId, GetRequestId)

    #undef GETTER

    TMaybe<TString> TFlagsJsonFlagsBase::GetValueFromName(TStringBuf prefixExpName) const {
        const auto& voiceFlags = GetFlagsInfoProto()->GetVoiceFlags().GetStorage();
        for (auto& [k, v] : voiceFlags) {
            if (k.StartsWith(prefixExpName)) {
                TString valueFromName{k.begin()+prefixExpName.size(), k.end()};
                return valueFromName;
            }
        }
        return {};
    }

    TString TFlagsJsonFlagsBase::ToString() const {
        return TStringBuilder() << *GetFlagsInfoProto();
    }


    TFlagsJsonFlagsConstRef::TFlagsJsonFlagsConstRef(const NAliceProtocol::TFlagsInfo* ptr)
        : FlagsInfoProto(ptr)
    {
    }

    const NAliceProtocol::TFlagsInfo* TFlagsJsonFlagsConstRef::GetFlagsInfoProto() const {
        return FlagsInfoProto;
    }


    TFlagsJsonFlagsHolder::TFlagsJsonFlagsHolder(NAliceProtocol::TFlagsInfo value)
        : FlagsInfoProto(std::move(value))
    {
    }

    const NAliceProtocol::TFlagsInfo* TFlagsJsonFlagsHolder::GetFlagsInfoProto() const {
        return &FlagsInfoProto;
    }

} // namespace NVoice::NExperiments
