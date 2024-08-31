#pragma once

#include <alice/megamind/library/models/directives/client_directive_model.h>

namespace NAlice::NMegamind {

class TCloseDialogDirectiveModel final : public virtual TClientDirectiveModel {
public:
    explicit TCloseDialogDirectiveModel(const TString& analyticsType, const TString& dialogId,
                                        TMaybe<TString>&& screenId);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TString& GetDialogId() const;

    [[nodiscard]] const TMaybe<TString>& GetScreenId() const;

private:
    const TString DialogId;
    const TMaybe<TString> ScreenId;
};

} // namespace NAlice::NMegamind
