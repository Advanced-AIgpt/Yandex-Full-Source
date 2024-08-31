#pragma once

#include <alice/megamind/library/models/interfaces/directive_model.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>

#include <variant>

namespace NAlice::NMegamind {

class TProtobufUniproxyDirectiveModel : public IDirectiveModel {
public:
    using TDirectives = std::variant<
        NScenarios::TCancelScheduledActionDirective,
        NScenarios::TEnlistScheduledActionDirective
    >;

public:
    explicit TProtobufUniproxyDirectiveModel(const NScenarios::TCancelScheduledActionDirective& directive);
    explicit TProtobufUniproxyDirectiveModel(const NScenarios::TEnlistScheduledActionDirective& directive);

    // IDirectiveModel overrides.
    const TString& GetName() const override;
    EDirectiveType GetType() const override;

    // IModel overrides.
    void Accept(IModelSerializer& serializer) const override;

    const TDirectives& Directives() const {
        return Directives_;
    }

    void SetUniproxyDirectiveMeta(NSpeechKit::TUniproxyDirectiveMeta uniproxyDirectiveMeta);
    [[nodiscard]] const NSpeechKit::TUniproxyDirectiveMeta* GetUniproxyDirectiveMeta() const;

private:
    TString Name_;
    TDirectives Directives_;
    TMaybe<NSpeechKit::TUniproxyDirectiveMeta> UniproxyDirectiveMeta;
};

} // namespace NAlice::NMegamind
