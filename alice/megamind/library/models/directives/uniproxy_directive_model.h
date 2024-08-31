#pragma once

#include <alice/megamind/library/models/interfaces/directive_model.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>

namespace NAlice::NMegamind {

class TUniproxyDirectiveModel : public IDirectiveModel {
public:
    explicit TUniproxyDirectiveModel(const TString& name);

    [[nodiscard]] const TString& GetName() const final;

    [[nodiscard]] EDirectiveType GetType() const final;

    void SetUniproxyDirectiveMeta(NSpeechKit::TUniproxyDirectiveMeta uniproxyDirectiveMeta);
    [[nodiscard]] const NSpeechKit::TUniproxyDirectiveMeta* GetUniproxyDirectiveMeta() const;

private:
    TString Name;
    TMaybe<NSpeechKit::TUniproxyDirectiveMeta> UniproxyDirectiveMeta;
};

} // namespace NAlice::NMegamind
