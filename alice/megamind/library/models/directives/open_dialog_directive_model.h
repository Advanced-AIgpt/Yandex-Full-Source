#pragma once

#include <alice/megamind/library/models/directives/client_directive_model.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <utility>

namespace NAlice::NMegamind {

class TOpenDialogDirectiveModel final : public TClientDirectiveModel {
public:
    explicit TOpenDialogDirectiveModel(const TString& analyticsType, const TString& dialogId,
                                       TVector<TIntrusivePtr<IDirectiveModel>> directives);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TString& GetDialogId() const;
    [[nodiscard]] const TVector<TIntrusivePtr<IDirectiveModel>>& GetDirectives() const;

private:
    const TString DialogId;
    const TVector<TIntrusivePtr<IDirectiveModel>> Directives;
};

class TOpenDialogDirectiveModelBuilder final {
public:
    [[nodiscard]] TOpenDialogDirectiveModel Build() const;

    TOpenDialogDirectiveModelBuilder& SetAnalyticsType(const TString& analyticsType);
    TOpenDialogDirectiveModelBuilder& SetDialogId(const TString& dialogId);

    template <typename TDirectiveModel>
    TOpenDialogDirectiveModelBuilder& AddDirective(TDirectiveModel&& directive) {
        Directives.push_back(std::move(MakeIntrusive<TDirectiveModel>(std::forward<TDirectiveModel>(directive))));
        return *this;
    }

private:
    TString AnalyticsType;
    TString DialogId;
    TVector<TIntrusivePtr<IDirectiveModel>> Directives;
};

} // namespace NAlice::NMegamind
