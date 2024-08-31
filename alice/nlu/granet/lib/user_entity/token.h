#pragma once

#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <alice/nlu/granet/lib/lang/simple_tokenizer.h>
#include <alice/nlu/libs/interval/interval.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/tokenizer/tokenizer.h>
#include <util/generic/array_ref.h>
#include <util/generic/hash.h>

namespace NGranet::NUserEntity {

// ~~~~ ETokenFlag ~~~~

enum ETokenFlag : ui32 {
    TF_STRONG = FLAG32(0),
};

Y_DECLARE_FLAGS(ETokenFlags, ETokenFlag);
Y_DECLARE_OPERATORS_FOR_FLAGS(ETokenFlags);

// ~~~~ TTokenInfo ~~~~

struct TTokenInfo {
    ui32 Original = 0;
    ui32 Normalized = 0;
    ui32 Lemma = 0;
    ETokenFlags Flags = 0;

    bool IsStrong() const {
        return Flags.HasFlags(TF_STRONG);
    }
};

// ~~~~ TTokenRange ~~~~

class TTokenRange {
public:
    TArrayRef<const TTokenInfo> Range;
    NNlu::TInterval Interval;
    size_t WholeLength = 0;

public:
    TTokenRange(const TVector<TTokenInfo>& whole, size_t offset, size_t count)
        : Range(whole)
        , Interval{offset, offset + count}
        , WholeLength(whole.size())
    {
        Range = Range.subspan(offset, count);
    }

    bool HasLemma(const TTokenInfo& token) const {
        for (const TTokenInfo& other : Range) {
            if (token.Lemma == other.Lemma) {
                return true;
            }
        }
        return false;
    }
};

// ~~~~ TTokenBuilder ~~~~

class TTokenBuilder : public TMoveOnly {
public:
    explicit TTokenBuilder(const TLangMask& languages);

    // Params:
    //  text - source text.
    //  originalTokens - (optional) result tokens without normalization joined by space into string.
    TVector<TTokenInfo> Tokenize(TWtringBuf text, TUtf16String* originalTokens = nullptr);

    const TTokenInfo& MakeToken(TWtringBuf word);

    ui32 GetIdCounterValue() const;

private:
    ui32 MakeId(TUtf16String&& word);

private:
    TLangMask Languages;
    TSimpleTokenizer Tokenizer;
    THashMap<TUtf16String, ui32> WordToId;
    TVector<const TUtf16String*> IdToWord;
    THashMap<TUtf16String, TTokenInfo> TokenInfoCache;
};

} // namespace NGranet::NUserEntity
