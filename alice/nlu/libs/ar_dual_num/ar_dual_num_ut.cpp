#include "ar_dual_num.h"

#include <alice/nlu/libs/normalization/normalize.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/split.h>

namespace NAlice::NNlu {

    struct TCase {
        TString Input;
        bool ExpectedOutput;
    };

    Y_UNIT_TEST_SUITE(TestArDualNum) {
        
        Y_UNIT_TEST(TestIsArDualNum) {
            const TVector<TCase> testCases = {
                {"الكتاب", false},
                {"كتابين", true},
                {"نافذتين", true},
                {"", false},
            };
            for (size_t i = 0; i < testCases.size(); ++i) {
                UNIT_ASSERT_EQUAL_C(IsArDualNum(testCases[i].Input), testCases[i].ExpectedOutput, i);
            }
        }

        Y_UNIT_TEST(TestGetArDualNumEntities) {
            const TVector<TString> tokens = {"الكتاب", "كتابين", "نافذتين", "ы"};
            const TVector<bool> expectedDualNums = {false, true, true, false};
            TFstResult result = GetArDualNumEntities(tokens);

            UNIT_ASSERT_EQUAL(result.EntitiesSize(), expectedDualNums.size());
            for (size_t i = 0; i < result.EntitiesSize(); ++i) {
                UNIT_ASSERT_EQUAL_C(result.GetEntities()[i].GetStart(), i, i);
                UNIT_ASSERT_EQUAL_C(result.GetEntities()[i].GetEnd(), i + 1, i);
                UNIT_ASSERT_EQUAL_C(result.GetEntities()[i].GetStringValue(), tokens[i], i);
                
                if (expectedDualNums[i]) {
                    UNIT_ASSERT_EQUAL_C(result.GetEntities()[i].GetType(), "NUM", i);
                    UNIT_ASSERT_EQUAL_C(result.GetEntities()[i].GetValue(), "2", i);
                } else {
                    UNIT_ASSERT_EQUAL_C(result.GetEntities()[i].GetType(), "", i);
                    UNIT_ASSERT_EQUAL_C(result.GetEntities()[i].GetValue(), '"' + tokens[i] + '"', i);
                }
            }
        }
    }
} // namespace NAlice::NNlu
