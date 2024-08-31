#pragma once

#include <alice/megamind/library/models/interfaces/model.h>

#include <util/generic/ptr.h>

namespace NAlice::NMegamind {

enum ECardType {
    SimpleText /* "simple_text" */,
    TextWithButton /* "text_with_button" */,
    DivCard /* "div_card" */,
    Div2Card /* "div2_card" */,
};

class ICardModel : public IModel, public virtual TThrRefBase {
public:
    [[nodiscard]] virtual ECardType GetType() const = 0;
};

} // namespace NAlice::NMegamind
