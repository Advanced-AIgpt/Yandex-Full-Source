#pragma once

#include <alice/megamind/library/models/cards/card_model.h>

#include <util/generic/string.h>

namespace NAlice::NMegamind {

class TTextCardModel final : public virtual TCardModel {
public:
    explicit TTextCardModel(const TString& text);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TString& GetText() const;

private:
    const TString Text;
};

} // namespace NAlice::NMegamind
