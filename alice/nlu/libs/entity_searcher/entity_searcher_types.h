#pragma once

#include <alice/nlu/libs/interval/interval.h>
#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>

#include <alice/nlu/proto/entities/custom.pb.h>

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/containers/comptrie/search_iterator.h>

#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/stream/output.h>

namespace NAlice::NNlu {

using TTokenId = ui32;
using TTrie = TCompactTrie<TTokenId, TVector<size_t>>;
using TTrieIterator = TSearchIterator<TTrie>;

const TTokenId UNKNOWN_TOKEN = UINT_MAX;

struct TSynonym {
    ::NNlu::TInterval Interval;
    TString Text;
    double LogProbability = 0.;
};

struct TEntityString {
    TString Sample;
    TString Type;
    TString Value;
    double LogProbability = 0.;
    double Quality = 0.;

    DECLARE_TUPLE_LIKE_TYPE(TEntityString, Sample, Type, Value, LogProbability);
};

IOutputStream& operator<<(IOutputStream& out, const TEntityString& entity);

struct TEntitySearcherData {
    TBlob TrieData;
    TVector<TEntityString> EntityStrings;
    TVector<TString> IdToString;
};

} // namespace NAlice::NNlu
