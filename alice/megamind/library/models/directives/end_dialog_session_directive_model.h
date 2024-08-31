#pragma once

#include <alice/megamind/library/models/directives/client_directive_model.h>

namespace NAlice::NMegamind {

class TEndDialogSessionDirectiveModel final : public virtual TClientDirectiveModel {
public:
    explicit TEndDialogSessionDirectiveModel(const TString& analyticsType, const TString& dialogId);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TString& GetDialogId() const;

private:
    TString DialogId;
};

} // namespace NAlice::NMegamind
