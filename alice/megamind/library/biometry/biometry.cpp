#include "biometry.h"

namespace NAlice::NMegamind {
namespace {

using NScenarios::TUserClassification;

} // namespace

TUserClassification ParseUserClassification(const TBiometryClassification& biometryClassification) {
    TUserClassification::EAge age = TUserClassification::Adult;
    TUserClassification::EGender gender = TUserClassification::Unknown;
    for (const auto& classification : biometryClassification.GetSimple()) {
        if (classification.GetTag() == BIOMETRY_AGE_TAG) {
            TUserClassification::EAge_Parse(to_title(classification.GetClassName()), &age);
        } else if (classification.GetTag() == BIOMETRY_GENDER_TAG) {
            TUserClassification::EGender_Parse(to_title(classification.GetClassName()), &gender);
        }
    }

    TUserClassification userClassification;
    userClassification.SetAge(age);
    userClassification.SetGender(gender);
    return userClassification;
}

} // namespace NAlice::NMegamind
