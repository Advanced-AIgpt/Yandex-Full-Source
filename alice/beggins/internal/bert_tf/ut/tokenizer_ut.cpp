#include <alice/beggins/internal/bert_tf/tokenizer.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/gtest/gtest.h>

#include <util/generic/xrange.h>

const TString dataPath = BuildRoot() + "/alice/beggins/internal/bert_tf/ut/data/";
const TString startTriePath = dataPath + "start.trie";
const TString continuationTriePath = dataPath + "cont.trie";
const TString vocabFilePath = dataPath + "vocab.txt";
const TString graphDefPath = dataPath + "model.pb";

using namespace NAlice::NBeggins::NBertTfApplier;

const TUtf32String knownWords = U"сколько стоит новый чайник";
const TTokenizer::TResult knownWordsTokenization{.Tokens = {CLS, U"сколько", U"стоит", U"новый", U"чайник", SEP},
                                                 .IsContinuation = {0, 0, 0, 0, 0, 0}};
const TVector<TUtf32String> knownWordsSplitted{U"сколько", U"стоит", U"новый", U"чайник"};

const TUtf32String unknownWords = U"сколько стоит новенькая 3080ti";
const TTokenizer::TResult unknownWordsTokenization{
    .Tokens = {CLS, U"сколько", U"стоит", U"нове", U"##нькая", U"308", U"##0t", U"##i", SEP},
    .IsContinuation = {0, 0, 0, 0, 1, 0, 1, 1, 0}};
const TVector<TUtf32String> unknownWordsSplitted{U"сколько", U"стоит", U"новенькая", U"3080ti"};

class TBertTfTokenizerTest : public testing::Test {
protected:
    void SetUp() override {
        Tokenizer.reset(new TTokenizer(TTrie::FromFile(startTriePath), TTrie::FromFile(continuationTriePath)));
    }

    std::unique_ptr<TTokenizer> Tokenizer;
};

TEST_F(TBertTfTokenizerTest, TestKnown) {
    const auto resultKnownWordsTokenization = Tokenizer->Tokenize(knownWords);
    ASSERT_TRUE(resultKnownWordsTokenization.Tokens == knownWordsTokenization.Tokens);
    ASSERT_EQ(resultKnownWordsTokenization.IsContinuation, knownWordsTokenization.IsContinuation);
}

TEST_F(TBertTfTokenizerTest, TestUnknown) {
    const auto resultUnknownWordsTokenization = Tokenizer->Tokenize(unknownWords);
    ASSERT_TRUE(resultUnknownWordsTokenization.Tokens == unknownWordsTokenization.Tokens);
    ASSERT_EQ(resultUnknownWordsTokenization.IsContinuation, unknownWordsTokenization.IsContinuation);
}
