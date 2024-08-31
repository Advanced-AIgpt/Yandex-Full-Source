#pragma once

#include <alice/megamind/library/models/directives/client_directive_model.h>

#include <util/generic/vector.h>

namespace NAlice::NMegamind {

class TUpdateDialogInfoDirectiveStyleModel final : public virtual IModel {
public:
    TUpdateDialogInfoDirectiveStyleModel(const TString& oknyxLogo, const TString& suggestBorderColor,
                                         const TString& suggestFillColor, const TString& suggestTextColor,
                                         const TString& skillActionsTextColor, const TString& skillBubbleFillColor,
                                         const TString& skillBubbleTextColor, const TString& userBubbleFillColor,
                                         const TString& userBubbleTextColor, TVector<TString> oknyxErrorColors,
                                         TVector<TString> oknyxNormalColors);

    void Accept(IModelSerializer& serializer) const override;

    [[nodiscard]] const TString& GetOknyxLogo() const;
    [[nodiscard]] const TString& GetUserBubbleFillColor() const;
    [[nodiscard]] const TString& GetSuggestBorderColor() const;
    [[nodiscard]] const TString& GetSuggestFillColor() const;
    [[nodiscard]] const TString& GetSuggestTextColor() const;
    [[nodiscard]] const TString& GetSkillActionsTextColor() const;
    [[nodiscard]] const TString& GetSkillBubbleFillColor() const;
    [[nodiscard]] const TString& GetSkillBubbleTextColor() const;
    [[nodiscard]] const TString& GetUserBubbleTextColor() const;

    [[nodiscard]] const TVector<TString>& GetOknyxErrorColors() const;
    [[nodiscard]] const TVector<TString>& GetOknyxNormalColors() const;

private:
    const TString OknyxLogo;
    const TString SuggestBorderColor;
    const TString SuggestFillColor;
    const TString SuggestTextColor;
    const TString SkillActionsTextColor;
    const TString SkillBubbleFillColor;
    const TString SkillBubbleTextColor;
    const TString UserBubbleFillColor;
    const TString UserBubbleTextColor;

    const TVector<TString> OknyxErrorColors;
    const TVector<TString> OknyxNormalColors;
};

class TUpdateDialogInfoDirectiveStyleModelBuilder final {
public:
    [[nodiscard]] TUpdateDialogInfoDirectiveStyleModel Build() const;

    TUpdateDialogInfoDirectiveStyleModelBuilder& SetOknyxLogo(const TString& oknyxLogo);
    TUpdateDialogInfoDirectiveStyleModelBuilder& SetSuggestBorderColor(const TString& suggestBorderColor);
    TUpdateDialogInfoDirectiveStyleModelBuilder& SetSuggestFillColor(const TString& suggestFillColor);
    TUpdateDialogInfoDirectiveStyleModelBuilder& SetSuggestTextColor(const TString& suggestTextColor);
    TUpdateDialogInfoDirectiveStyleModelBuilder& SetSkillActionsTextColor(const TString& skillActionsTextColor);
    TUpdateDialogInfoDirectiveStyleModelBuilder& SetSkillBubbleFillColor(const TString& skillBubbleFillColor);
    TUpdateDialogInfoDirectiveStyleModelBuilder& SetSkillBubbleTextColor(const TString& skillBubbleTextColor);
    TUpdateDialogInfoDirectiveStyleModelBuilder& SetUserBubbleFillColor(const TString& userBubbleFillColor);
    TUpdateDialogInfoDirectiveStyleModelBuilder& SetUserBubbleTextColor(const TString& userBubbleTextColor);

    TUpdateDialogInfoDirectiveStyleModelBuilder& AddOknyxErrorColor(const TString& oknyxErrorColor);
    TUpdateDialogInfoDirectiveStyleModelBuilder& AddOknyxNormalColor(const TString& oknyxNormalColors);

private:
    TString OknyxLogo;
    TString SuggestBorderColor;
    TString SuggestFillColor;
    TString SuggestTextColor;
    TString SkillActionsTextColor;
    TString SkillBubbleFillColor;
    TString SkillBubbleTextColor;
    TString UserBubbleFillColor;
    TString UserBubbleTextColor;

    TVector<TString> OknyxErrorColors;
    TVector<TString> OknyxNormalColors;
};

class TUpdateDialogInfoDirectiveMenuItemModel final : public virtual IModel {
public:
    TUpdateDialogInfoDirectiveMenuItemModel(const TString& title, const TString& url);

    void Accept(IModelSerializer& serializer) const override;

    [[nodiscard]] const TString& GetTitle() const;
    [[nodiscard]] const TString& GetUrl() const;

private:
    const TString Title;
    const TString Url;
};

class TUpdateDialogInfoDirectiveModel final : public virtual TClientDirectiveModel {
public:
public:
    explicit TUpdateDialogInfoDirectiveModel(const TString& analyticsType, const TString& title, const TString& url,
                                             const TString& imageUrl, TUpdateDialogInfoDirectiveStyleModel style,
                                             TUpdateDialogInfoDirectiveStyleModel darkStyle, TVector<TUpdateDialogInfoDirectiveMenuItemModel> menuItems,
                                             const TString& adBlockId);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TString& GetTitle() const;
    [[nodiscard]] const TString& GetUrl() const;
    [[nodiscard]] const TString& GetImageUrl() const;
    [[nodiscard]] const TString& GetAdBlockId() const;

    [[nodiscard]] const TUpdateDialogInfoDirectiveStyleModel& GetStyle() const;
    [[nodiscard]] const TUpdateDialogInfoDirectiveStyleModel& GetDarkStyle() const;

    [[nodiscard]] const TVector<TUpdateDialogInfoDirectiveMenuItemModel>& GetMenuItems() const;

private:
    const TString ImageUrl;
    const TString Title;
    const TString Url;
    const TString AdBlockId;

    const TUpdateDialogInfoDirectiveStyleModel Style;
    const TUpdateDialogInfoDirectiveStyleModel DarkStyle;

    const TVector<TUpdateDialogInfoDirectiveMenuItemModel> MenuItems;
};

} // namespace NAlice::NMegamind
