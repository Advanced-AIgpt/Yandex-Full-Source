#include "aligner.h"
#include "joined_tokens.h"
#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/iterator_range.h>
#include <util/generic/reserve.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/subst.h>
#include <util/string/type.h>
#include <util/string/util.h>

namespace NNlu {

// ~~~~ TNormalizedTokenAligner ~~~~

static const size_t PRECISE_MATCH_LIMIT = 20;

TAlignmentData TNormalizedTokenAligner::Align(TWtringBuf joinedTokens1, TWtringBuf joinedTokens2) {
    InitWorkingTokens(joinedTokens1, &Tokens1);
    InitWorkingTokens(joinedTokens2, &Tokens2);

    const size_t initialTokenCount1 = Tokens1.size();
    const size_t initialTokenCount2 = Tokens2.size();

    Alignment = {};

    const size_t prefixLength = MatchCommonPrefix();
    const size_t suffixLength = MatchCommonSuffix();

    Alignment.AddOneToOneSegments(prefixLength, true);
    while (!Tokens1.empty() || !Tokens2.empty()) {
        MatchChunk();
    }
    Alignment.AddOneToOneSegments(suffixLength, true);

    Y_ENSURE(Alignment.IsValid());
    Y_ENSURE(Alignment.CountTokens1() == initialTokenCount1);
    Y_ENSURE(Alignment.CountTokens2() == initialTokenCount2);

    return std::move(Alignment);
}

// static
void TNormalizedTokenAligner::InitWorkingTokens(TWtringBuf joinedTokens, TDeque<TToken>* workingTokens) {
    Y_ENSURE(workingTokens);
    workingTokens->clear();
    while (!joinedTokens.empty()) {
        const TWtringBuf token = joinedTokens.NextTok(u' ');
        workingTokens->push_back({ComputeHash(token), token});
    }
}

size_t TNormalizedTokenAligner::MatchCommonPrefix() {
    size_t length = 0;
    while (!Tokens1.empty() && !Tokens2.empty() && Tokens1.front() == Tokens2.front()) {
        length++;
        Tokens1.pop_front();
        Tokens2.pop_front();
    }
    return length;
}

size_t TNormalizedTokenAligner::MatchCommonSuffix() {
    size_t length = 0;
    while (!Tokens1.empty() && !Tokens2.empty() && Tokens1.back() == Tokens2.back()) {
        length++;
        Tokens1.pop_back();
        Tokens2.pop_back();
    }
    return length;
}

void TNormalizedTokenAligner::MatchChunk() {
    // Trivial
    if (Tokens1.empty() && Tokens2.empty()) {
        return;
    }
    if (Tokens1.empty()) {
        Alignment.AddSegment(0, Tokens2.size(), false);
        Tokens2.clear();
        return;
    }
    if (Tokens2.empty()) {
        Alignment.AddSegment(Tokens1.size(), 0, false);
        Tokens1.clear();
        return;
    }

    if (Tokens1.size() * Tokens2.size() <= PRECISE_MATCH_LIMIT * PRECISE_MATCH_LIMIT) {
        NLevenshtein::TEditChain chain;
        NLevenshtein::GetEditChain(Tokens1, Tokens2, chain);
        ProcessEditChain(chain);
        Y_ENSURE(Tokens1.empty());
        Y_ENSURE(Tokens2.empty());
        return;
    }

    // Too long sequences. Complexity of GetEditChain is O(size1*size2). Perform GetEditChain on portion of tokens.
    auto range1 = MakeIteratorRange(Tokens1.begin(), Tokens1.begin() + Min(Tokens1.size(), PRECISE_MATCH_LIMIT));
    auto range2 = MakeIteratorRange(Tokens2.begin(), Tokens2.begin() + Min(Tokens2.size(), PRECISE_MATCH_LIMIT));
    NLevenshtein::TEditChain chain;
    NLevenshtein::GetEditChain(range1, range2, chain);
    chain.crop(PRECISE_MATCH_LIMIT / 2);
    ProcessEditChain(chain);
}

void TNormalizedTokenAligner::ProcessEditChain(const NLevenshtein::TEditChain& chain) {
    auto it = chain.begin();
    while (it != chain.end()) {
        if (*it == NLevenshtein::EMT_PRESERVE) {
            Alignment.AddSegment(1, 1, true);
            Tokens1.pop_front();
            Tokens2.pop_front();
            ++it;
            continue;
        }
        size_t length1 = 0;
        size_t length2 = 0;
        while (it != chain.end() && *it != NLevenshtein::EMT_PRESERVE) {
            if (*it == NLevenshtein::EMT_REPLACE) {
                length1++;
                Tokens1.pop_front();
                length2++;
                Tokens2.pop_front();
            } else if (*it == NLevenshtein::EMT_DELETE) {
                length1++;
                Tokens1.pop_front();
            } else if (*it == NLevenshtein::EMT_INSERT) {
                length2++;
                Tokens2.pop_front();
            } else {
                Y_ENSURE(false);
            }
            ++it;
        }
        if (length1 == length2) {
            Alignment.AddOneToOneSegments(length1, false);
        } else {
            Alignment.AddSegment(length1, length2, false);
        }
    }
}

// ~~~~ TTokenAligner ~~~~

// Simple normalization for tokens typically glued to numbers.
static const THashMap<TUtf16String, TUtf16String> TokenNormalizationTable = {
    {u"плюс",           u"+"},
    {u"минус",          u"-"},
    {u"равно",          u"="},
    {u"дробь",          u"/"},
    {u"доллар",         u"$"},
    {u"евро",           u"€"},

    {u"нол",            u"0"},
    {u"нул",            u"0"},
    {u"один",           u"1"},
    {u"перв",           u"1"},
    {u"одн",            u"1"},
    {u"два",            u"2"},
    {u"двум",           u"2"},
    {u"две",            u"2"},
    {u"втор",           u"2"},
    {u"три",            u"3"},
    {u"трем",           u"3"},
    {u"трет",           u"3"},
    {u"четыр",          u"4"},
    {u"четверт",        u"4"},
    {u"пят",            u"5"},
    {u"шест",           u"6"},
    {u"сем",            u"7"},
    {u"седьм",          u"7"},
    {u"восемь",         u"8"},
    {u"восьм",          u"8"},
    {u"девят",          u"9"},
    {u"десят",          u"10"},
    {u"одиннадцат",     u"11"},
    {u"двенадцат",      u"12"},
    {u"тринадцат",      u"13"},
    {u"четырнадцат",    u"14"},
    {u"пятнадцат",      u"15"},
    {u"шестнадцат",     u"16"},
    {u"семнадцат",      u"17"},
    {u"восемнадцат",    u"18"},
    {u"девятнадцат",    u"19"},
    {u"двадцат",        u"20"},
    {u"тридцат",        u"30"},
    {u"сорок",          u"40"},
    {u"сороков",        u"40"},
    {u"пятьдесят",      u"50"},
    {u"пятидесят",      u"50"},
    {u"шестьдесят",     u"60"},
    {u"шестидесят",     u"60"},
    {u"семьдесят",      u"70"},
    {u"семидесят",      u"70"},
    {u"восемьдесят",    u"80"},
    {u"восьмидесят",    u"80"},
    {u"девяност",       u"90"},
    {u"сто",            u"100"},
    {u"ста",            u"100"},
    {u"двести",         u"200"},
    {u"двухсот",        u"200"},
    {u"двумстам",       u"200"},
    {u"двухстах",       u"200"},
    {u"триста",         u"300"},
    {u"трехсот",        u"300"},
    {u"тремстам",       u"300"},
    {u"трехстах",       u"300"},
    {u"четырста",       u"400"},
    {u"четырехсот",     u"400"},
    {u"четыремстам",    u"400"},
    {u"четырехстах",    u"400"},
    {u"пятьсот",        u"500"},
    {u"пятисот",        u"500"},
    {u"пятистам",       u"500"},
    {u"пятистах",       u"500"},
    {u"шестьсот",       u"600"},
    {u"семьсот",        u"700"},
    {u"восемьсот",      u"800"},
    {u"девятьсот",      u"900"},
    {u"тысяч",          u"1000"},
    {u"миллион",        u"1000000"},
    {u"миллиард",       u"1000000000"},
    {u"триллион",       u"1000000000000"},
};

TAlignment TTokenAligner::Align(const TVector<TString>& tokens1, const TVector<TString>& tokens2) {
    return Align(NJoinedTokens::JoinTokens(tokens1), NJoinedTokens::JoinTokens(tokens2));
}

TAlignment TTokenAligner::Align(const TVector<TStringBuf>& tokens1, const TVector<TStringBuf>& tokens2) {
    return Align(NJoinedTokens::JoinTokens(tokens1), NJoinedTokens::JoinTokens(tokens2));
}

TAlignment TTokenAligner::Align(TStringBuf tokens1, TStringBuf tokens2) {
    return Align(UTF8ToWide(tokens1), UTF8ToWide(tokens2));
}

TAlignment TTokenAligner::Align(TWtringBuf tokensRaw1, TWtringBuf tokensRaw2) {
    const TUtf16String tokens1 = NJoinedTokens::Collapse(tokensRaw1);
    const TUtf16String tokens2 = NJoinedTokens::Collapse(tokensRaw2);

    const size_t tokenCount1 = NJoinedTokens::CountTokens(tokens1);
    const size_t tokenCount2 = NJoinedTokens::CountTokens(tokens2);

    // Optimization for trivial cases.
    if (tokens1 == tokens2) {
        Y_ENSURE(tokenCount1 == tokenCount2);
        return TAlignment::CreateTrivial(tokenCount1);
    }

    // Function NormalizeTokens can split some tokens into subtokens. Keep information about that
    // spliting in arryas partitionsX. partitionsX[i] is number of subtokens in i-th token.
    TVector<size_t> partitions1;
    TVector<size_t> partitions2;
    const TUtf16String subTokens1 = NormalizeTokens(tokens1, &partitions1);
    const TUtf16String subTokens2 = NormalizeTokens(tokens2, &partitions2);
    const size_t subTokenCount1 = NJoinedTokens::CountTokens(subTokens1);
    const size_t subTokenCount2 = NJoinedTokens::CountTokens(subTokens2);
    Y_ENSURE(partitions1.size() == tokenCount1);
    Y_ENSURE(partitions2.size() == tokenCount2);
    Y_ENSURE(Accumulate(partitions1, 0u) == subTokenCount1);
    Y_ENSURE(Accumulate(partitions2, 0u) == subTokenCount2);

    // Align subtokens.
    TAlignmentData alignment = TNormalizedTokenAligner().Align(subTokens1, subTokens2);
    Y_ENSURE(alignment.CountTokens1() == subTokenCount1);
    Y_ENSURE(alignment.CountTokens2() == subTokenCount2);

    // Convert alignment of subtokens to alignment of original tokens.
    alignment.MergeTokens1(partitions1);
    alignment.MergeTokens2(partitions2);

    Y_ENSURE(alignment.IsValid());
    Y_ENSURE(alignment.CountTokens1() == tokenCount1);
    Y_ENSURE(alignment.CountTokens2() == tokenCount2);

    return TAlignment(std::move(alignment));
}

// static
TUtf16String TTokenAligner::NormalizeTokens(TWtringBuf tokensRaw, TVector<size_t>* partitions) {
    Y_ENSURE(partitions);

    TUtf16String tokens = ToLowerRet(tokensRaw);
    SubstGlobal(tokens, u'ё', u'е');

    TUtf16String result(Reserve(tokens.length()));
    for (const auto token : StringSplitter(tokens).Split(u' ').SkipEmpty()) {
        TUtf16String normalized = NormalizeToken(token);
        partitions->push_back(NJoinedTokens::CountTokens(normalized));
        if (!result.empty()) {
            result.append(u' ');
        }
        result.append(normalized);
    }
    return result;
}

// static
TUtf16String TTokenAligner::NormalizeToken(TWtringBuf tokenOriginal) {
    TUtf16String result;
    for (const auto subTokenOriginal : StringSplitter(tokenOriginal).SplitBySet(u"-'").SkipEmpty()) {
        TUtf16String subToken(subTokenOriginal.Token());
        NormalizeByTable(&subToken);
        SplitNumber(&subToken);
        if (subToken.empty()) {
            continue;
        }
        if (!result.empty()) {
            result.append(u' ');
        }
        result.append(subToken);
    }
    if (result.empty()) {
        return TUtf16String(tokenOriginal);
    }
    return result;
}

// static
void TTokenAligner::NormalizeByTable(TUtf16String* token) {
    Y_ENSURE(token);
    TWtringBuf prefix = *token;
    for (size_t i = 0; i < 4; ++i) {
        if (const TUtf16String* normalized = TokenNormalizationTable.FindPtr(prefix)) {
            *token = *normalized;
            return;
        }
        prefix.Chop(1);
    }
}

// Examples:
//   "123456" -> "100 20 3 1000 400 50 6"
//   "100000" -> "100 1000"
//   "1012"   -> "1 1000 10 2"
//   "12"     -> "10 2"
//   "0"      -> "0"
// static
void TTokenAligner::SplitNumber(TUtf16String* token) {
    Y_ENSURE(token);
    if (token->length() > 6
        || !IsNumber(*token)
        || token->empty()
        || token->front() == u'0')
    {
        return;
    }
    TUtf16String result;
    for (size_t i = 0; i < token->length(); ++i) {
        const wchar16 c = (*token)[i];
        size_t power = token->length() - i - 1;
        Y_ASSERT(power < 6);
        if (power == 3) {
            if (c != u'0') {
                result.append(c);
                result.append(u' ');
            }
            result.append(TWtringBuf(u"1000 "));
            continue;
        }
        if (c == u'0') {
            continue;
        }
        if (power >= 3) {
            power -= 3;
        }
        result.append(c);
        result.append(power, u'0');
        result.append(u' ');
    }
    if (result.empty()) {
        result.append('0');
    } else {
        result.pop_back(); // space
    }
    *token = result;
}

// ~~~~ TTokenCachedAligner ~~~~

TTokenCachedAligner::TTokenCachedAligner(size_t cacheLimit)
    : CacheLimit(cacheLimit)
{
}

const TAlignment& TTokenCachedAligner::Align(const TVector<TString>& tokens1, const TVector<TString>& tokens2) {
    return Align(NJoinedTokens::JoinTokens(tokens1), NJoinedTokens::JoinTokens(tokens2));
}

const TAlignment& TTokenCachedAligner::Align(const TVector<TStringBuf>& tokens1, const TVector<TStringBuf>& tokens2) {
    return Align(NJoinedTokens::JoinTokens(tokens1), NJoinedTokens::JoinTokens(tokens2));
}

const TAlignment& TTokenCachedAligner::Align(const TString& joinedTokens1, const TString& joinedTokens2) {
    if (Cache.size() >= CacheLimit) {
        Cache.clear();
    }
    auto [it, isNew] = Cache.try_emplace(TAlignerInput{joinedTokens1, joinedTokens2});
    if (isNew) {
        it->second = TTokenAligner().Align(joinedTokens1, joinedTokens2);
    }
    return it->second;
}

} // namespace NNlu
