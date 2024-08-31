#pragma once

#include <alice/megamind/library/models/buttons/button_model.h>
#include <alice/megamind/library/models/interfaces/directive_model.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>

#include <utility>

namespace NAlice::NMegamind {

class TActionButtonModel final : public virtual TButtonModel {
public:
    class TTheme final {
    public:
        explicit TTheme(const TString& imageUrl)
            : ImageUrl(imageUrl) {
        }

        [[nodiscard]] const TString& GetImageUrl() const {
            return ImageUrl;
        }

    private:
        TString ImageUrl;
    };

public:
    TActionButtonModel(const TString& title, TVector<TIntrusivePtr<IDirectiveModel>> directives,
                       const TMaybe<TTheme>& theme = Nothing(), const TMaybe<TString>& text = Nothing());

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TVector<TIntrusivePtr<IDirectiveModel>>& GetDirectives() const;
    [[nodiscard]] const TMaybe<TTheme>& GetTheme() const {
        return Theme;
    }
    [[nodiscard]] const TMaybe<TString>& GetText() const {
        return Text;
    }

private:
    TVector<TIntrusivePtr<IDirectiveModel>> Directives;
    TMaybe<TTheme> Theme;
    TMaybe<TString> Text;
};

class TActionButtonModelBuilder final {
public:
    [[nodiscard]] TActionButtonModel Build() const;

    TActionButtonModelBuilder& SetTitle(const TString& title);

    template <typename TDirectiveModel>
    TActionButtonModelBuilder& AddDirective(TDirectiveModel&& directive) {
        Directives.push_back(std::move(MakeIntrusive<TDirectiveModel>(std::forward<TDirectiveModel>(directive))));
        return *this;
    }

private:
    TString Title;
    TVector<TIntrusivePtr<IDirectiveModel>> Directives;
};

} // namespace NAlice::NMegamind
