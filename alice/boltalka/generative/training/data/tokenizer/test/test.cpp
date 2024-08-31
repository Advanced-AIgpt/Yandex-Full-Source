#include <alice/boltalka/generative/training/data/tokenizer/tokenizer.h>

#include <library/unittest/registar.h>

Y_UNIT_TEST_SUITE(Suite) {
    Y_UNIT_TEST(Test) {
        TTokenizer tokenizer("bpe.voc");
        UNIT_ASSERT_VALUES_EQUAL(tokenizer.Tokenize("приветушки как дела ?"), "привет `уш `ки как дела ?");
    }
}
