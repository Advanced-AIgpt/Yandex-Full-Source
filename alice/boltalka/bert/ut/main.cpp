#include <library/cpp/testing/unittest/registar.h>
#include <dict/mt/libs/nn/ynmt/test_utils/test_backend.h>

#include <alice/boltalka/bert/lib/main.h>

constexpr double PRECISION = 1e-3;

Y_UNIT_TEST_SUITE(TestBoltalkaBert) {
    Y_UNIT_TEST(Test) {
        TVector<TString> samples = {
            "[CLS] [SEP] [SEP] привет [SEP] привет [SEP]",
            "[CLS] [SEP] привет [SEP] как дела [SEP] неплохо [SEP]",
            "[CLS] [SEP] что такое дебют [SEP] и что такое идея [SEP] я не знаю [SEP]",
            "[CLS] нет [SEP] ну ладно , как хотите [SEP] нет [SEP] а что ? [SEP]",
            "[CLS] нет [SEP] ну ладно , как хотите [SEP] нет [SEP] как же нет , когда да [SEP]",
            "[CLS] нет [SEP] ну ладно , как хотите [SEP] нет [SEP] яблоки вкусные [SEP]",
        };
        auto data = PreprocessSamples(
            samples,
            128,
            "start.trie",
            "cont.trie",
            "vocab.txt");

        TVector<float> results = Run<float>(data, samples, 32, 128, "boltalka-bert-mse.npz", NDict::NMT::NYNMT::NTesting::CreateTestBackend());
        UNIT_ASSERT_DOUBLES_EQUAL(1.81228, results[0], PRECISION);
        UNIT_ASSERT_DOUBLES_EQUAL(1.37753, results[1], PRECISION);
        UNIT_ASSERT_DOUBLES_EQUAL(1.05803, results[2], PRECISION);
        UNIT_ASSERT_DOUBLES_EQUAL(1.56769, results[3], PRECISION);
        UNIT_ASSERT_DOUBLES_EQUAL(1.96784, results[4], PRECISION);
        UNIT_ASSERT_DOUBLES_EQUAL(0.416829, results[5], PRECISION);
    }
}
