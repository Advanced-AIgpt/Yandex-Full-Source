#pragma once

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/vector.h>


namespace NAlice {

struct TAsrHypothesis {
public:
    explicit TAsrHypothesis(TVector<TUtf16String> words)
        : Words(std::move(words))
    {
    }

    explicit TAsrHypothesis(const TVector<TString>& words) {
        for (const auto& word : words) {
            Words.push_back(UTF8ToWide(word));
        }
    }

    [[nodiscard]] TUtf16String Utterance() const {
        return JoinStrings(Words, u" ");
    };

    [[nodiscard]] TString ComputeUtf8Utterance() const {
        return WideToUTF8(Utterance());
    }

public:
    TVector<TUtf16String> Words;
};

}  // namespace NAlice
