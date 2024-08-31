#include "update_dialog_info_directive_model.h"

#include <utility>

namespace NAlice::NMegamind {

// TUpdateDialogInfoDirectiveStyleModel::TStyle -----------------------------
TUpdateDialogInfoDirectiveStyleModel::TUpdateDialogInfoDirectiveStyleModel(
    const TString& oknyxLogo, const TString& suggestBorderColor, const TString& suggestFillColor,
    const TString& suggestTextColor, const TString& skillActionsTextColor, const TString& skillBubbleFillColor,
    const TString& skillBubbleTextColor, const TString& userBubbleFillColor, const TString& userBubbleTextColor,
    TVector<TString> oknyxErrorColors, TVector<TString> oknyxNormalColors)
    : OknyxLogo(oknyxLogo)
    , SuggestBorderColor(suggestBorderColor)
    , SuggestFillColor(suggestFillColor)
    , SuggestTextColor(suggestTextColor)
    , SkillActionsTextColor(skillActionsTextColor)
    , SkillBubbleFillColor(skillBubbleFillColor)
    , SkillBubbleTextColor(skillBubbleTextColor)
    , UserBubbleFillColor(userBubbleFillColor)
    , UserBubbleTextColor(userBubbleTextColor)
    , OknyxErrorColors(std::move(oknyxErrorColors))
    , OknyxNormalColors(std::move(oknyxNormalColors)) {
}

void TUpdateDialogInfoDirectiveStyleModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TUpdateDialogInfoDirectiveStyleModel::GetOknyxLogo() const {
    return OknyxLogo;
}

const TString& TUpdateDialogInfoDirectiveStyleModel::GetSuggestBorderColor() const {
    return SuggestBorderColor;
}

const TString& TUpdateDialogInfoDirectiveStyleModel::GetSuggestFillColor() const {
    return SuggestFillColor;
}

const TString& TUpdateDialogInfoDirectiveStyleModel::GetSuggestTextColor() const {
    return SuggestTextColor;
}

const TString& TUpdateDialogInfoDirectiveStyleModel::GetSkillActionsTextColor() const {
    return SkillActionsTextColor;
}

const TString& TUpdateDialogInfoDirectiveStyleModel::GetSkillBubbleFillColor() const {
    return SkillBubbleFillColor;
}

const TString& TUpdateDialogInfoDirectiveStyleModel::GetSkillBubbleTextColor() const {
    return SkillBubbleTextColor;
}

const TString& TUpdateDialogInfoDirectiveStyleModel::GetUserBubbleFillColor() const {
    return UserBubbleFillColor;
}

const TString& TUpdateDialogInfoDirectiveStyleModel::GetUserBubbleTextColor() const {
    return UserBubbleTextColor;
}

const TVector<TString>& TUpdateDialogInfoDirectiveStyleModel::GetOknyxErrorColors() const {
    return OknyxErrorColors;
}

const TVector<TString>& TUpdateDialogInfoDirectiveStyleModel::GetOknyxNormalColors() const {
    return OknyxNormalColors;
}

// TUpdateDialogInfoDirectiveMenuItemModel ----------------------------------
TUpdateDialogInfoDirectiveMenuItemModel::TUpdateDialogInfoDirectiveMenuItemModel(const TString& title,
                                                                                 const TString& url)
    : Title(title)
    , Url(url) {
}

void TUpdateDialogInfoDirectiveMenuItemModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TUpdateDialogInfoDirectiveMenuItemModel::GetTitle() const {
    return Title;
}

const TString& TUpdateDialogInfoDirectiveMenuItemModel::GetUrl() const {
    return Url;
}

// TUpdateDialogInfoDirectiveModel ---------------------------------------------
TUpdateDialogInfoDirectiveModel::TUpdateDialogInfoDirectiveModel(
    const TString& analyticsType, const TString& title, const TString& url, const TString& imageUrl,
    TUpdateDialogInfoDirectiveStyleModel style, TUpdateDialogInfoDirectiveStyleModel darkStyle,
    TVector<TUpdateDialogInfoDirectiveMenuItemModel> menuItems, const TString& adBlockId)
    : TClientDirectiveModel("update_dialog_info", analyticsType)
    , ImageUrl(imageUrl)
    , Title(title)
    , Url(url)
    , AdBlockId(adBlockId)
    , Style(std::move(style))
    , DarkStyle(std::move(darkStyle))
    , MenuItems(std::move(menuItems)) {
}

void TUpdateDialogInfoDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TUpdateDialogInfoDirectiveModel::GetTitle() const {
    return Title;
}

const TString& TUpdateDialogInfoDirectiveModel::GetUrl() const {
    return Url;
}

const TString& TUpdateDialogInfoDirectiveModel::GetImageUrl() const {
    return ImageUrl;
}

const TString& TUpdateDialogInfoDirectiveModel::GetAdBlockId() const {
    return AdBlockId;
}

const TUpdateDialogInfoDirectiveStyleModel& TUpdateDialogInfoDirectiveModel::GetStyle() const {
    return Style;
}

const TUpdateDialogInfoDirectiveStyleModel& TUpdateDialogInfoDirectiveModel::GetDarkStyle() const {
    return DarkStyle;
}

const TVector<TUpdateDialogInfoDirectiveMenuItemModel>& TUpdateDialogInfoDirectiveModel::GetMenuItems() const {
    return MenuItems;
}

// TUpdateDialogInfoDirectiveStyleModel -------------------------------------
TUpdateDialogInfoDirectiveStyleModel TUpdateDialogInfoDirectiveStyleModelBuilder::Build() const {
    return TUpdateDialogInfoDirectiveStyleModel(
        OknyxLogo, SuggestBorderColor, SuggestFillColor, SuggestTextColor, SkillActionsTextColor, SkillBubbleFillColor,
        SkillBubbleTextColor, UserBubbleFillColor, UserBubbleTextColor, OknyxErrorColors, OknyxNormalColors);
}

TUpdateDialogInfoDirectiveStyleModelBuilder&
TUpdateDialogInfoDirectiveStyleModelBuilder::SetOknyxLogo(const TString& oknyxLogo) {
    OknyxLogo = oknyxLogo;
    return *this;
}

TUpdateDialogInfoDirectiveStyleModelBuilder&
TUpdateDialogInfoDirectiveStyleModelBuilder::SetSuggestBorderColor(const TString& suggestBorderColor) {
    SuggestBorderColor = suggestBorderColor;
    return *this;
}

TUpdateDialogInfoDirectiveStyleModelBuilder&
TUpdateDialogInfoDirectiveStyleModelBuilder::SetSuggestFillColor(const TString& suggestFillColor) {
    SuggestFillColor = suggestFillColor;
    return *this;
}

TUpdateDialogInfoDirectiveStyleModelBuilder&
TUpdateDialogInfoDirectiveStyleModelBuilder::SetSuggestTextColor(const TString& suggestTextColor) {
    SuggestTextColor = suggestTextColor;
    return *this;
}

TUpdateDialogInfoDirectiveStyleModelBuilder&
TUpdateDialogInfoDirectiveStyleModelBuilder::SetSkillActionsTextColor(const TString& skillActionsTextColor) {
    SkillActionsTextColor = skillActionsTextColor;
    return *this;
}

TUpdateDialogInfoDirectiveStyleModelBuilder&
TUpdateDialogInfoDirectiveStyleModelBuilder::SetSkillBubbleFillColor(const TString& skillBubbleFillColor) {
    SkillBubbleFillColor = skillBubbleFillColor;
    return *this;
}

TUpdateDialogInfoDirectiveStyleModelBuilder&
TUpdateDialogInfoDirectiveStyleModelBuilder::SetSkillBubbleTextColor(const TString& skillBubbleTextColor) {
    SkillBubbleTextColor = skillBubbleTextColor;
    return *this;
}

TUpdateDialogInfoDirectiveStyleModelBuilder&
TUpdateDialogInfoDirectiveStyleModelBuilder::SetUserBubbleFillColor(const TString& userBubbleFillColor) {
    UserBubbleFillColor = userBubbleFillColor;
    return *this;
}

TUpdateDialogInfoDirectiveStyleModelBuilder&
TUpdateDialogInfoDirectiveStyleModelBuilder::SetUserBubbleTextColor(const TString& userBubbleTextColor) {
    UserBubbleTextColor = userBubbleTextColor;
    return *this;
}

TUpdateDialogInfoDirectiveStyleModelBuilder&
TUpdateDialogInfoDirectiveStyleModelBuilder::AddOknyxErrorColor(const TString& oknyxErrorColor) {
    OknyxErrorColors.push_back(oknyxErrorColor);
    return *this;
}

TUpdateDialogInfoDirectiveStyleModelBuilder&
TUpdateDialogInfoDirectiveStyleModelBuilder::AddOknyxNormalColor(const TString& oknyxNormalColors) {
    OknyxNormalColors.push_back(oknyxNormalColors);
    return *this;
}

} // namespace NAlice::NMegamind
