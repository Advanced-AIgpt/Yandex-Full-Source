#pragma once

#include <alice/megamind/library/classifiers/formulas/protos/formulas_description.pb.h>

#include <alice/megamind/library/config/protos/classification_config.pb.h>
#include <alice/megamind/library/config/protos/config.pb.h>

#include <alice/library/client/client_features.h>
#include <alice/library/logger/logger.h>

#include <library/cpp/langs/langs.h>

#include <google/protobuf/util/message_differencer.h>

#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/xrange.h>

template <>
struct THash<NAlice::TFormulaKey> {
    ui64 operator()(const NAlice::TFormulaKey& formulaKey) const {
        ui64 res = 0;
        const auto* descriptor = formulaKey.GetDescriptor();
        const auto* reflection = formulaKey.GetReflection();
        for (const ui32 i : xrange(descriptor->field_count())) {
            const auto* fieldDescriptor = descriptor->field(i);
            if (!reflection->HasField(formulaKey, fieldDescriptor)) {
                continue;
            }

            ui64 value = 0;
            switch (fieldDescriptor->cpp_type()) {
                case NProtoBuf::FieldDescriptor::CPPTYPE_ENUM:
                    value = THash<int>{}(reflection->GetEnumValue(formulaKey, fieldDescriptor));
                    break;
                case NProtoBuf::FieldDescriptor::CPPTYPE_STRING:
                    value = THash<TString>{}(reflection->GetString(formulaKey, fieldDescriptor));
                    break;
                default:
                    ythrow yexception{}
                        << "Unsupported field " << fieldDescriptor->name().Quote()
                        << " of type " << TString{fieldDescriptor->cpp_type_name()}.Quote();
            }
            res = CombineHashes(res, value);
        }

        return res;
    }
};

template <>
struct TEqualTo<NAlice::TFormulaKey> {
    bool operator()(const NAlice::TFormulaKey& lhs, const NAlice::TFormulaKey& rhs) const {
        return google::protobuf::util::MessageDifferencer::Equivalent(lhs, rhs);
    }
};

namespace NAlice {

class TFormulasDescription {
public:
    TFormulasDescription() = default;
    TFormulasDescription(
        const google::protobuf::Map<TString, NMegamind::TClassificationConfig::TScenarioConfig>& scenarioClassificationConfigs,
        const TFsPath& formulasFolder,
        TRTLogger& logger
    );

    bool AddFormula(TFormulaDescription formulaDescription);

    void AddFormulasFromResource(const TFsPath& formulasPath, TRTLogger& logger);

    const TFormulaDescription& Lookup(
        const TStringBuf scenarioName,
        const EMmClassificationStage classificationStage,
        const EClientType clientType,
        const TStringBuf experiment,
        const ELanguage language
    ) const noexcept;

private:
    THashMap<TFormulaKey, TFormulaDescription> FormulasDescriptionMap_;

    void FillFormulasDescriptionMapByScenarioClassificationConfigs(
        const google::protobuf::Map<TString, NMegamind::TClassificationConfig::TScenarioConfig>& scenarioClassificationConfigs
    );
};

inline EClientType GetClientType(const TClientFeatures& clientFeatures) {
    if (clientFeatures.IsSmartSpeaker()) {
        return ECT_SMART_SPEAKER;
    } else if (clientFeatures.IsTvDevice() || clientFeatures.IsLegatus()) {
        return ECT_SMART_TV;
    } else if (clientFeatures.IsElariWatch()) {
        return ECT_ELARI_WATCH;
    } else if (clientFeatures.IsDesktop()) {
        return ECT_DESKTOP;
    } else if (clientFeatures.SupportsNavigator()) {
        return ECT_NAVIGATION; // Navigator is touch too, so we check ECT_TOUCH after it.
    } else if (clientFeatures.IsTouch()) {
        return ECT_TOUCH;
    } else {
        return ECT_UNKNOWN;
    }
}

} // namespace NAlice
