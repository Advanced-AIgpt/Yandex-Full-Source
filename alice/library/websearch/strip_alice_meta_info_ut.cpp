#include "strip_alice_meta_info.h"

#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

void AddWords(TEvent& event, const TVector<TString>& words) {
    auto& asrResult = *event.AddAsrResult();
    for (const auto& word : words) {
        asrResult.AddWords()->SetValue(word);
    }
}

void AddHypothesis(TAliceMetaInfo::TCompressedAsr& asr, const TVector<int>& indices) {
    auto& hypothesis = *asr.AddHypotheses();
    for (const auto index : indices) {
        hypothesis.AddWordIndices(index);
    }
}

} // namespace

Y_UNIT_TEST_SUITE(StripAliceMetaInfo) {
    Y_UNIT_TEST(FillCompressedAsr) {
        TEvent event;
        AddWords(event, {"hello", "foo"});
        AddWords(event, {"hello", "bar"});

        TAliceMetaInfo::TCompressedAsr actualAsr;
        FillCompressedAsr(actualAsr, event.GetAsrResult());

        TAliceMetaInfo::TCompressedAsr expectedAsr;
        *expectedAsr.AddWords() = "hello";
        *expectedAsr.AddWords() = "foo";
        *expectedAsr.AddWords() = "bar";
        AddHypothesis(expectedAsr, {0, 1});
        AddHypothesis(expectedAsr, {0, 2});

        UNIT_ASSERT_MESSAGES_EQUAL(expectedAsr, actualAsr);
    }

    Y_UNIT_TEST(DecodeCompressedAsr) {
        TAliceMetaInfo::TCompressedAsr asr;
        *asr.AddWords() = "hello";
        *asr.AddWords() = "foo";
        *asr.AddWords() = "bar";
        AddHypothesis(asr, {0, 1});
        AddHypothesis(asr, {0, 2});

        UNIT_ASSERT_VALUES_EQUAL("hello foo", DecodeCompressedAsr(asr, 0));
        UNIT_ASSERT_VALUES_EQUAL("hello bar", DecodeCompressedAsr(asr, 1));
    }
}
