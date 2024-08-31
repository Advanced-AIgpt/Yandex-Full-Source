#include "biometry.h"

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/string.h>

using namespace NAlice;
using namespace NAlice::NMegamind;
using NScenarios::TUserClassification;

namespace {

Y_UNIT_TEST_SUITE(Biometry) {
    Y_UNIT_TEST(TestLoad) {
        for (const auto& gender : {BIOMETRY_GENDER_UNKNOWN, BIOMETRY_GENDER_MALE, BIOMETRY_GENDER_FEMALE}) {
            for (const auto& age : {BIOMETRY_AGE_ADULT, BIOMETRY_AGE_CHILD}) {
                NJson::TJsonValue biometryClassificationJson;

                auto& genderClassification = biometryClassificationJson["simple"][0];
                genderClassification["classname"] = TStringBuf(gender);
                genderClassification["tag"] = TStringBuf(BIOMETRY_GENDER_TAG);

                auto& ageClassification = biometryClassificationJson["simple"][1];
                ageClassification["classname"] = TStringBuf(age);
                ageClassification["tag"] = TStringBuf(BIOMETRY_AGE_TAG);

                const auto biometryClassification = JsonToProto<TBiometryClassification>(biometryClassificationJson);

                TUserClassification userClassification = ParseUserClassification(biometryClassification);

                TUserClassification::EAge expectedAge;
                UNIT_ASSERT(TUserClassification::EAge_Parse(to_title(TString(age)), &expectedAge));
                UNIT_ASSERT_EQUAL(userClassification.GetAge(), expectedAge);

                TUserClassification::EGender expectedGender;
                UNIT_ASSERT(TUserClassification::EGender_Parse(to_title(TString(gender)), &expectedGender));
                UNIT_ASSERT_EQUAL(userClassification.GetGender(), expectedGender);
            }
        }
    }
    Y_UNIT_TEST(TestDefaults) {
        const TBiometryClassification biometryClassification;
        TUserClassification userClassification = ParseUserClassification(biometryClassification);
        UNIT_ASSERT_EQUAL(userClassification.GetAge(), TUserClassification::Adult);
        UNIT_ASSERT_EQUAL(userClassification.GetGender(), TUserClassification::Unknown);
    }
    Y_UNIT_TEST(TestInvalidData) {
        NJson::TJsonValue biometryClassificationJson;

        auto& classification = biometryClassificationJson["simple"][0];
        classification["classname"] = TStringBuf("teen");
        classification["tag"] = TStringBuf(BIOMETRY_AGE_TAG);

        const auto biometryClassification = JsonToProto<TBiometryClassification>(biometryClassificationJson);
        UNIT_ASSERT_EQUAL(ParseUserClassification(biometryClassification).GetAge(), TUserClassification::Adult);
    }
}

} // namespace
