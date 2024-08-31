#include "runtime.h"

#include <alice/library/json/json.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>
#include <type_traits>

using namespace NAlice::NJsonSchemaBuilder::NRuntime;

TBuilder BUILDER;
static_assert(std::is_same_v<const NJson::TJsonValue&,
                             decltype(BUILDER.ValueWithoutValidation())>);
static_assert(std::is_same_v<NJson::TJsonValue&&,
                             decltype(std::move(BUILDER).ValueWithoutValidation())>);

Y_UNIT_TEST_SUITE(Util) {
    Y_UNIT_TEST(GetNumUtf16CharsInUtf8String) {
#define UTF_STRINGS(x) { x, u ## x}
        const std::pair<TString, TUtf16String> positiveCases[] = {
            UTF_STRINGS(""),
            UTF_STRINGS("Hello, world!"),
            UTF_STRINGS("Привет, мир!"),
            UTF_STRINGS("So I set the 🔥, isn't it good, 🇳🇴🌳"),
        };
#undef UTF_STRINGS

        for (const auto& [utf8, utf16] : positiveCases) {
            UNIT_ASSERT_VALUES_EQUAL_C(GetNumUtf16CharsInUtf8String(utf8), utf16.size(), "utf8 = " << utf8);
        }

        UNIT_ASSERT_EXCEPTION(GetNumUtf16CharsInUtf8String("\xd1\xd1"), TValidationError);
    }
}

Y_UNIT_TEST_SUITE(Validation) {
    Y_UNIT_TEST(Color) {
        UNIT_ASSERT_NO_EXCEPTION(ValidateColor("#333333"));
        UNIT_ASSERT_NO_EXCEPTION(ValidateColor("#333333aa"));
        UNIT_ASSERT_EXCEPTION(ValidateColor("#333333a"), TValidationError);
        UNIT_ASSERT_EXCEPTION(ValidateColor("#333333aaa"), TValidationError);
        UNIT_ASSERT_EXCEPTION(ValidateColor("#333333tt"), TValidationError);
    }

    Y_UNIT_TEST(JsonPayload) {
        UNIT_ASSERT_NO_EXCEPTION(ValidateJsonPayload(NAlice::JsonFromString("{}")));
        UNIT_ASSERT_NO_EXCEPTION(ValidateJsonPayload(NAlice::JsonFromString("{\"foo\": 123}")));
        UNIT_ASSERT_EXCEPTION(ValidateJsonPayload(NAlice::JsonFromString("123")), TValidationError);
        UNIT_ASSERT_EXCEPTION(ValidateJsonPayload(NAlice::JsonFromString("[123]")), TValidationError);
        UNIT_ASSERT_EXCEPTION(ValidateJsonPayload(NAlice::JsonFromString("null")), TValidationError);
    }

    Y_UNIT_TEST(IntegerConstraint) {
        const TStringBuf constraint = "number >= 5 && number <= 10";
        auto validator = [](const i64 number) {
            return number >= 5 && number <= 10;
        };

        // numbers in [5, 10] are valid
        UNIT_ASSERT_NO_EXCEPTION(ValidateIntegerConstraint(constraint, validator, 5 /* value */));
        UNIT_ASSERT_NO_EXCEPTION(ValidateIntegerConstraint(constraint, validator, 7 /* value */));
        UNIT_ASSERT_NO_EXCEPTION(ValidateIntegerConstraint(constraint, validator, 10 /* value */));

        // numbers outside [5, 10] are in valid
        UNIT_ASSERT_EXCEPTION(ValidateIntegerConstraint(constraint, validator, 11 /* value */), TValidationError);
        UNIT_ASSERT_EXCEPTION(ValidateIntegerConstraint(constraint, validator, -11 /* value */), TValidationError);
    }

    Y_UNIT_TEST(DoubleConstraint) {
        const TStringBuf constraint = "number >= 0.0 && number <= 1.0";
        auto validator = [](const double number) {
            return number >= 0.0 && number <= 1.0;
        };

        // numbers in [0.0, 1.0] are valid
        UNIT_ASSERT_NO_EXCEPTION(ValidateDoubleConstraint(constraint, validator, 0.0 /* value */));
        UNIT_ASSERT_NO_EXCEPTION(ValidateDoubleConstraint(constraint, validator, 0.666 /* value */));
        UNIT_ASSERT_NO_EXCEPTION(ValidateDoubleConstraint(constraint, validator, 1.0 /* value */));

        // numbers outside [0.0, 1.0] are invalid
        UNIT_ASSERT_EXCEPTION(ValidateDoubleConstraint(constraint, validator, 2.0 /* value */), TValidationError);
        UNIT_ASSERT_EXCEPTION(ValidateDoubleConstraint(constraint, validator, -2.0 /* value */), TValidationError);
    }

    Y_UNIT_TEST(MinLength) {
        UNIT_ASSERT_NO_EXCEPTION(ValidateMinLength(3 /* length */, 1 /* minLength */));
        UNIT_ASSERT_NO_EXCEPTION(ValidateMinLength(2 /* length */, 1 /* minLength */));
        UNIT_ASSERT_NO_EXCEPTION(ValidateMinLength(1 /* length */, 1 /* minLength */));
        UNIT_ASSERT_EXCEPTION(ValidateMinLength(0 /* length */, 1 /* minLength */), TValidationError);
    }

    Y_UNIT_TEST(MaxLength) {
        UNIT_ASSERT_NO_EXCEPTION(ValidateMaxLength(3 /* length */, 200 /* maxLength */));
        UNIT_ASSERT_NO_EXCEPTION(ValidateMaxLength(199 /* length */, 200 /* maxLength */));
        UNIT_ASSERT_NO_EXCEPTION(ValidateMaxLength(200 /* length */, 200 /* maxLength */));
        UNIT_ASSERT_EXCEPTION(ValidateMaxLength(201 /* length */, 200 /* maxLength */), TValidationError);
    }

    Y_UNIT_TEST(Pattern) {
        UNIT_ASSERT_NO_EXCEPTION(ValidatePattern(re2::RE2("a+"), "aaaaa"));
        UNIT_ASSERT_EXCEPTION(ValidatePattern(re2::RE2("a+"), "aaaaa bbbb"), TValidationError);
    }
}
