#include "dataset.h"
#include "context_storage.h"
#include "fetcher.h"
#include <alice/nlu/granet/lib/sample/tag.h>
#include <alice/nlu/granet/lib/utils/json_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <dict/nerutil/tstimer.h>
#include <library/cpp/map_text_file/map_text_file.h>
#include <util/string/split.h>

namespace NGranet {

using namespace NJson;

// ~~~~ ESampleComponentFlag ~~~~

static const TMap<TStringBuf, ESampleComponentFlags> ColumnToComponent = {
    {"weight",      SCF_WEIGHT},
    {"reqid",       SCF_REQID},
    {"text",        SCF_TEXT},
    {"context",     SCF_CONTEXT},
    {"wizextra",    SCF_WIZEXTRA},
    {"mock",        SCF_MOCK},
    {"embeddings",  SCF_EMBEDDINGS},
};

bool IsColumnInComponentSet(const ESampleComponentFlags& components, TStringBuf column) {
    return components & ColumnToComponent.Value(column, SCF_EXTRA);
}

// ~~~~ TTsvSample ~~~~

// static
TTsvSample TTsvSample::Read(const TSampleTsvLine& line) {
    if (!line.IsDefined()) {
        return {};
    }
    TTsvSample sample;
    sample.TsvLine = line;
    sample.WeightStr = line.Value(ESampleColumnId::Weight, TString());
    sample.Reqid = line.Value(ESampleColumnId::Reqid, TString());
    sample.TaggedText = line[ESampleColumnId::Text];
    sample.CleanText = RemoveTaggerMarkup(sample.TaggedText);
    sample.Context = line.Value(ESampleColumnId::Context, TString());
    sample.Wizextra = line.Value(ESampleColumnId::Wizextra, TString());
    sample.Mock = line.Value(ESampleColumnId::Mock, TString());
    sample.Embeddings = line.Value(ESampleColumnId::Embeddings, TString());
    sample.Weight = line.Value(ESampleColumnId::Weight, 1.);
    return sample;
}

TTsvSampleKey TTsvSample::MakeKey() const {
    return {Reqid, CleanText};
}

// ~~~~ TTsvSampleDataset ~~~~

TTsvSampleDataset::TTsvSampleDataset(const TFsPath& path) {
    Load(path);
}

void TTsvSampleDataset::Load(const TFsPath& path) {
    Tsv.Read(path);
    UpdateKeyToLineIndex();
}

void TTsvSampleDataset::UpdateKeyToLineIndex() {
    KeyToLineIndex.clear();
    for (const auto& [lineIndex, line] : Enumerate(Tsv.GetLines())) {
        TTsvSampleKey key = {
            .Reqid = line.Value<TString>(ESampleColumnId::Reqid, TString()),
            .CleanText = RemoveTaggerMarkup(line[ESampleColumnId::Text]),
        };
        KeyToLineIndex[std::move(key)] = lineIndex;
    }
}

size_t TTsvSampleDataset::FindSample(const TTsvSampleKey& key) const {
    return KeyToLineIndex.Value(key, NPOS);
}

TTsvSample TTsvSampleDataset::ReadSample(size_t index) const {
    Y_ENSURE(index < Size());
    return TTsvSample::Read(Tsv.GetLines().at(index));
}

double TTsvSampleDataset::CalculateWeightTotal() const {
    double total = 0;
    for (const TSampleTsvLine& line : Tsv.GetLines()) {
        total += TTsvSample::Read(line).Weight;
    }
    return total;
}

// ~~~~ TTsvSampleDatasetCachedLoader ~~~~

TTsvSampleDatasetCachedLoader::TRef TTsvSampleDatasetCachedLoader::Create(size_t cacheLimit) {
    return new TTsvSampleDatasetCachedLoader(cacheLimit);
}

TTsvSampleDatasetCachedLoader::TTsvSampleDatasetCachedLoader(size_t cacheLimit)
    : Cache(cacheLimit)
{
}

TTsvSampleDataset::TConstRef TTsvSampleDatasetCachedLoader::LoadDataset(const TFsPath& path) {
    DEBUG_TIMER("NGranet::TTsvSampleDatasetCachedLoader::LoadDataset");

    if (const TMaybe<TTsvSampleDataset::TConstRef> cached = Cache.Find(path.GetPath()); cached.Defined()) {
        Y_ENSURE(*cached != nullptr);
        return *cached;
    }
    TTsvSampleDataset::TConstRef created = MakeIntrusive<TTsvSampleDataset>(path);
    Cache.Insert(path.GetPath(), created);
    return created;
}

} // namespace NGranet
