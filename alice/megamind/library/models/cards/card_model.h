#pragma once

#include <alice/megamind/library/models/interfaces/card_model.h>

namespace NAlice::NMegamind {

class TCardModel : public virtual ICardModel {
public:
    explicit TCardModel(const ECardType type);

    [[nodiscard]] ECardType GetType() const final;

private:
    const ECardType Type;
};

} // namespace NAlice::NMegamind
