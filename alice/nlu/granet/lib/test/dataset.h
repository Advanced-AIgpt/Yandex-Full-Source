#pragma once

#include "tsv.h"
#include "simple_cache.h"
#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <alice/nlu/granet/lib/sample/sample.h>
#include <alice/nlu/granet/lib/sample/tag.h>
#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>
#include <library/cpp/langs/langs.h>
#include <util/folder/path.h>
#include <util/generic/deque.h>
#include <util/generic/noncopyable.h>
#include <util/memory/blob.h>

namespace NGranet {

// ~~~~ TTsvSampleKey ~~~~

struct TTsvSampleKey {
    TString Reqid;
    TString CleanText;

    DECLARE_TUPLE_LIKE_TYPE(TTsvSampleKey, Reqid, CleanText);
};

} // namespace NGranet

// Global namespace
template <>
struct THash<NGranet::TTsvSampleKey>: public TTupleLikeTypeHash {
};

namespace NGranet {

// ~~~~ ESampleColumnId ~~~~

enum class ESampleColumnId {
    Weight,
    Reqid,
    Text,
    Context,
    Wizextra,
    Mock,
    Embeddings,
};

inline const TMap<ESampleColumnId, TString> SampleColumnIdToName = {
    {ESampleColumnId::Weight,      "weight"},
    {ESampleColumnId::Reqid,       "reqid"},
    {ESampleColumnId::Text,        "text"},
    {ESampleColumnId::Context,     "context"},
    {ESampleColumnId::Wizextra,    "wizextra"},
    {ESampleColumnId::Mock,        "mock"},
    {ESampleColumnId::Embeddings,  "embeddings"},
};

template <>
struct TTsvTraits<ESampleColumnId> {
    static const TMap<ESampleColumnId, TString>& GetIdNames() {
        return SampleColumnIdToName;
    };
};

inline const TString& GetColumnName(ESampleColumnId id) {
    return SampleColumnIdToName.at(id);
}

// ~~~~ ESampleComponentFlag ~~~~

enum ESampleComponentFlag : ui32 {
    SCF_WEIGHT      = FLAG32(0), // column "weight"
    SCF_REQID       = FLAG32(1), // column "reqid"
    SCF_TEXT        = FLAG32(2), // column "text"
    SCF_CONTEXT     = FLAG32(3), // column "context"
    SCF_WIZEXTRA    = FLAG32(4), // column "wizextra"
    SCF_MOCK        = FLAG32(5), // column "mock"
    SCF_EMBEDDINGS  = FLAG32(6), // column "embeddings"
    SCF_EXTRA       = FLAG32(7), // other columns
};

Y_DECLARE_FLAGS(ESampleComponentFlags, ESampleComponentFlag);
Y_DECLARE_OPERATORS_FOR_FLAGS(ESampleComponentFlags)

inline const ESampleComponentFlags SCF_KEY_COLUMNS = SCF_REQID | SCF_TEXT;
inline const ESampleComponentFlags SCF_ALL_COLUMNS = SCF_WEIGHT | SCF_REQID | SCF_TEXT |
    SCF_CONTEXT | SCF_WIZEXTRA | SCF_MOCK | SCF_EMBEDDINGS | SCF_EXTRA;

bool IsColumnInComponentSet(const ESampleComponentFlags& components, TStringBuf column);

// ~~~~ TSampleTsvLine ~~~~

using TSampleTsvLine = TTsvLine<ESampleColumnId>;
using TSampleTsvHeader = TTsvHeader<ESampleColumnId>;

// ~~~~ TTsvSample ~~~~

struct TTsvSample {
    TSampleTsvLine TsvLine;
    TString WeightStr;
    TString Reqid;
    TString TaggedText;
    TString CleanText;
    TString Context;
    TString Wizextra;
    TString Mock;
    TString Embeddings;
    double Weight = 1.;

    static TTsvSample Read(const TSampleTsvLine& line);

    TTsvSampleKey MakeKey() const;
};

// ~~~~ TTsvSampleDataset ~~~~

class TTsvSampleDataset : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TTsvSampleDataset>;
    using TConstRef = TIntrusiveConstPtr<TTsvSampleDataset>;

public:
    TTsvSampleDataset() = default;
    explicit TTsvSampleDataset(const TFsPath& path);

    void Load(const TFsPath& path);

    const TSampleTsvHeader::TConstRef& GetHeader() const {
        return Tsv.GetHeader();
    }

    const TFsPath& GetPath() const {
        return Tsv.GetHeader()->GetPath();
    }

    size_t Size() const {
        return Tsv.GetLines().size();
    }

    size_t FindSample(const TTsvSampleKey& key) const;

    TTsvSample ReadSample(size_t index) const;

    double CalculateWeightTotal() const;

private:
    void UpdateKeyToLineIndex();

private:
    TTsv<ESampleColumnId> Tsv;
    THashMap<TTsvSampleKey, size_t> KeyToLineIndex;
};

// ~~~~ TTsvSampleDatasetCachedLoader ~~~~

const size_t DATASET_CACHE_DEFAULT_LIMIT = 20;

class TTsvSampleDatasetCachedLoader : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TTsvSampleDatasetCachedLoader>;
    using TConstRef = TIntrusiveConstPtr<TTsvSampleDatasetCachedLoader>;

public:
    static TTsvSampleDatasetCachedLoader::TRef Create(size_t cacheLimit = DATASET_CACHE_DEFAULT_LIMIT);

    TTsvSampleDataset::TConstRef LoadDataset(const TFsPath& path);

private:
    TTsvSampleDatasetCachedLoader(size_t cacheLimit);

private:
    // Path to dataset
    TSimpleCache<TString, TTsvSampleDataset::TConstRef> Cache;
};

} // namespace NGranet
