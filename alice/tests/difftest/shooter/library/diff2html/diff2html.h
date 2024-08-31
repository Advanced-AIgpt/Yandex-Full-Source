#pragma once

#include <util/folder/path.h>
#include <util/system/mutex.h>
#include <util/thread/pool.h>

namespace NAlice::NShooter {

class TDiff2Html : public NNonCopyable::TNonCopyable {
public:
    TDiff2Html(int threads, int diffsPerFile, const TString& mode, const TFsPath& oldResponsesPath,
               const TFsPath& newResponsesPath, const TFsPath& outputPath, const TMaybe<TFsPath>& statsPath);
    void ConstructDiff();

private:
    struct TDiffInfo {
        TString ReqId;
        TString OldResponse;
        TString NewResponse;
    };
    using NormalizerFunc = std::function<TString(const TString&)>;

private:
    void WorkWithFile(const TString& reqId, const TString& outputFolder, const TString& fileName, NormalizerFunc normalizer);
    void FlushDiffs(const TString& folder);

private:
    const int ThreadCount_;
    const int DiffsPerFile_;
    const TString& Mode_;
    const TFsPath& OldResponsesPath_;
    const TFsPath& NewResponsesPath_;
    const TFsPath& OutputPath_;
    const TMaybe<TFsPath>& StatsPath_;

    TVector<TDiffInfo> DiffInfos_;
    TMutex Lock_;
    int WrittenHtmlFiles_;
    int DiffsCount_;

    int ResponsesDiffsSummary_;
};

} // namespace NAlice::NShooter
