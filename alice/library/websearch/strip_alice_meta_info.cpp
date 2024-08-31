#include "strip_alice_meta_info.h"

#include <library/cpp/iterator/mapped.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>
#include <util/string/join.h>

namespace NAlice {

namespace {

constexpr size_t NUM_MAX_HYPOTHESES = 5;

} // namespace

void FillCompressedAsr(TAliceMetaInfo::TCompressedAsr& asr, const google::protobuf::RepeatedPtrField<TAsrResult>& asrResults) {
    // replace each word with a unique index
    THashMap<TString, size_t> words;
    const auto numHypotheses = Min(NUM_MAX_HYPOTHESES, static_cast<size_t>(asrResults.size()));
    for (size_t i = 0; i < numHypotheses; ++i) {
        const auto& inputWords = asrResults[i].GetWords();

        auto& hypothesis = *asr.AddHypotheses()->MutableWordIndices();
        hypothesis.Reserve(inputWords.size());

        for (const auto& word : inputWords) {
            const auto [it, inserted] = words.insert({word.GetValue(), words.size()});
            hypothesis.Add(it->second);
        }
    }

    // store index to word correspondence for decoding
    TVector<TString> sortedWords(words.size());
    for (const auto& [word, index] : words) {
        sortedWords[index] = word;
    }

    auto& asrWords = *asr.MutableWords();
    asrWords.Reserve(words.size());
    for (auto& word : sortedWords) {
        asrWords.Add(std::move(word));
    }
}

TString DecodeCompressedAsr(const TAliceMetaInfo::TCompressedAsr& asr, const size_t index) {
    const auto& words = asr.GetWords();
    const auto& hypotheses = asr.GetHypotheses();

    Y_ENSURE(index < static_cast<size_t>(hypotheses.size()));

    const auto range = MakeMappedRange(
        hypotheses[index].GetWordIndices(),
        [&words](const size_t wordIndex) -> TStringBuf {
            return words[wordIndex];
        }
    );
    return JoinSeq(TStringBuf(" "), range);
}

} // namespace NAlice
