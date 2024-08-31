#include "simple_tokenizer.h"
#include <library/cpp/testing/unittest/registar.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(TSimpleTokenizer) {

    Y_UNIT_TEST(Test) {
        TSimpleTokenizer tokenizer;
        UNIT_ASSERT_STRINGS_EQUAL("Call of Duty Modern Warfare 3 Начало № 1",
            tokenizer.Tokenize("Call of Duty: Modern Warfare 3-Начало!-№1\n"));
        UNIT_ASSERT_STRINGS_EQUAL("Алиса включи ка мне четвёртый номер",
            tokenizer.Tokenize("Алиса, включи-ка мне четвёртый номер."));
    };
}
