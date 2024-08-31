#pragma once

#include "dataset.h"
#include <alice/nlu/granet/lib/grammar/domain.h>
#include <library/cpp/langs/langs.h>
#include <util/folder/path.h>
#include <util/generic/noncopyable.h>

namespace NGranet {

// ~~~~ TDatasetMockUpdater ~~~~

class TDatasetMockUpdater : public TMoveOnly {
public:
    struct TOptions {
        TFsPath SrcPath;
        TFsPath DestPath;
        TGranetDomain Domain;
        bool ShouldUpdateMock = true;
        bool ShouldUpdateEmbeddings = false;
        bool OnlyMissing = false;
        bool RemoveBadSamples = false;
        size_t ThreadCount = 16;
    };

public:
    explicit TDatasetMockUpdater(const TOptions& options, IOutputStream* log = nullptr);

    void Update();

private:
    enum ESampleStatus {
        SS_UNCHANGED,
        SS_SUCCESS,
        SS_EMPTY_REQUEST,
        SS_OTHER_ERROR,
    };

    struct TSampleData {
        TTsvSample Sample;
        TSampleTsvLine DestLine;
        ESampleStatus Status = SS_UNCHANGED;
        TString ErrorMessage;
        bool ShouldRemove = false;
    };

private:
    void DoUpdate(const TFsPath& destPath);
    TVector<TString> MakeDestColumns(const TVector<TString>& srcColumns) const;
    void ProcessSample(TSampleData* data) const;
    void ReportProgress(const TSampleData& data);
    void ReportResults();

private:
    TOptions Options;
    IOutputStream* Log = nullptr;
    size_t SuccessCount = 0;
    size_t ErrorCount = 0;
    size_t EmptyCount = 0;
};

} // namespace NGranet
