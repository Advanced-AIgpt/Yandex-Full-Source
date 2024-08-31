#pragma once

#include <alice/megamind/library/models/cards/card_model.h>
#include <alice/megamind/library/models/interfaces/button_model.h>
#include <alice/megamind/library/models/interfaces/directive_model.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <utility>

namespace NAlice::NMegamind {

class TTextWithButtonCardModel final : public virtual TCardModel {
public:
    explicit TTextWithButtonCardModel(const TString& text, TVector<TIntrusivePtr<IButtonModel>> buttons);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TString& GetText() const;
    [[nodiscard]] const TVector<TIntrusivePtr<IButtonModel>>& GetButtons() const;

private:
    const TString Text;
    const TVector<TIntrusivePtr<IButtonModel>> Buttons;
};

class TTextWithButtonCardModelBuilder final {
public:
    [[nodiscard]] TTextWithButtonCardModel Build() const;

    TTextWithButtonCardModelBuilder& SetText(const TString& text);

    template <typename TButtonModel>
    TTextWithButtonCardModelBuilder& AddButton(TButtonModel&& directive) {
        Buttons.push_back(MakeIntrusive<TButtonModel>(std::forward<TButtonModel>(directive)));
        return *this;
    }

private:
    TString Text;
    TVector<TIntrusivePtr<IButtonModel>> Buttons;
};

} // namespace NAlice::NMegamind
