#pragma once

#include "context_storage.h"
#include "dataset.h"
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/langs/langs.h>
#include <util/folder/path.h>
#include <util/generic/noncopyable.h>

namespace NGranet {

// ~~~~ TDatasetBuilder ~~~~

class TDatasetBuilder : public TMoveOnly {
public:
    struct TOptions {
        TFsPath InputPath;
        TFsPath OutputPath;
        TFsPath ContextStoragePath;
        bool MergeDuplicated = false;
        bool KeepTags = false;
        bool ShouldNormalize = false;
        bool ToLowerCase = false;
        ELanguage Lang = LANG_RUS;
        TString Columns;
        size_t SampleCountLimit = NPOS;
    };

public:
    explicit TDatasetBuilder(const TOptions& options, IOutputStream* log = nullptr);

    void BuildDataset();

private:
    TVector<TString> GetColumnNamesOption() const;
    void BuildFromTsv();
    void BuildFromNdjson();
    void BuildFromTxt();
    bool IsLimitReached() const;
    void WriteSample(TSampleTsvLine* line, TTsvWriter<ESampleColumnId>* output);
    static TString GetSampleKey(const TSampleTsvLine& line);
    TString NormalizeForDataset(TString text) const;
    void ReportProgress(size_t counter, TStringBuf message, TStringBuf current);

private:
    TOptions Options;
    IOutputStream* Log = nullptr;
    size_t AddedCount = 0;
    size_t NonTextCount = 0;
    size_t DuplicatedCount = 0;
    THashMap<TString, double> UniqueSamples;
    TContextPatchCollector ContextCollector;
};

} // namespace NGranet
