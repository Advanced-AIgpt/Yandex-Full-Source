#include "tokenizer.h"

#include <util/generic/xrange.h>
#include <util/string/vector.h>

using namespace NDict::NUtil;
using namespace NDict::NSegmenter;

namespace NAlice::NBeggins::NBertTfApplier {

TTokenizer::TTokenizer(std::unique_ptr<TTrie> startTrie, std::unique_ptr<TTrie> continuationTrie)
    : StartTrie(std::move(startTrie))
    , ContiuationTrie(std::move(continuationTrie))
    , BaseSegmenter(CreateCharBasedSegmenter()) {
    Y_ENSURE(StartTrie);
    Y_ENSURE(ContiuationTrie);
}

TTokenizer::TResult TTokenizer::Tokenize(const TUtf32String& text) const {
    const auto wordsUTF8 = SplitString(WideToUTF8(text), " ");
    TVector<TUtf32String> words(Reserve(wordsUTF8.size()));
    Transform(wordsUTF8.begin(), wordsUTF8.end(), std::back_inserter(words),
              [](const TString& word) { return TUtf32String::FromUtf8(word); });

    TResult result;
    result.Tokens.push_back(CLS);
    result.IsContinuation.push_back(0);
    for (const TUtf32String& word : words) {
        auto utf16word = UTF32ToWide(word.data(), word.size());
        auto segments = BaseSegmenter->Split(utf16word);
        const auto segments_end = segments.cend();

        bool is_start = 1;
        for (auto it = segments.cbegin(); it != segments_end; is_start = 0) {
            const auto match_length = (is_start ? StartTrie : ContiuationTrie)->MaxMatchingPrefixLen(it, segments_end);
            TUtf16String token = is_start ? u"" : u"##";
            for (size_t i = 0; i < Max(match_length, (size_t)1); i++) {
                token.append(*(it++));
            }
            result.Tokens.push_back(TUtf32String::FromUtf16(token));
            result.IsContinuation.push_back(!is_start);
        }
    }
    result.Tokens.push_back(SEP);
    result.IsContinuation.push_back(0);
    return result;
}
} // namespace NAlice::NBeggins::NBertTfApplier
