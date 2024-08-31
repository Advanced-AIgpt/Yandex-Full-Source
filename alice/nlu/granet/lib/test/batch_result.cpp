#include "batch_result.h"
#include <alice/nlu/granet/lib/utils/trace.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/generic/hash_set.h>
#include <util/stream/file.h>
#include <util/string/printf.h>

namespace NGranet {

namespace {

const int HISTORY_SIZE = 5;

// Forward declaration;
int FindLastResultNumber(const TFsPath& historyDir);
void DeleteOldResults(const TFsPath& historyDir, int lastResultNum, IOutputStream* log);
TFsPath FindLastResultDir(const TFsPath& historyDir, int lastResultNum);
void WriteDatasetDiff(const TFsPath& prevFilePath, const TFsPath& currFilePath,
    const TFsPath& diffDir, const TString& diffName, IOutputStream* log);

void PrepareResultDir(const TFsPath& batchResultsDir, TFsPath* prevResultDir,
    TFsPath* currResultDir, IOutputStream* log)
{
    Y_ENSURE(prevResultDir);
    Y_ENSURE(currResultDir);
    if (!batchResultsDir.IsDefined()) {
        return;
    }
    const TFsPath historyDir = batchResultsDir / "history";
    int lastResultNum = FindLastResultNumber(historyDir);
    DeleteOldResults(historyDir, lastResultNum, log);
    *prevResultDir = FindLastResultDir(historyDir, lastResultNum);
    *currResultDir = historyDir / ToString(lastResultNum + 1);
    currResultDir->MkDirs();
}

int FindLastResultNumber(const TFsPath& historyDir) {
    if (!historyDir.IsDirectory()) {
        return 0;
    }
    TVector<TString> names;
    historyDir.ListNames(names);
    int maxNum = 0;
    for (const TString& name : names) {
        int num = 0;
        if (TryFromString<int>(name, num) && num > maxNum) {
            maxNum = num;
        }
    }
    return maxNum;
}

void DeleteOldResults(const TFsPath& historyDir, int lastResultNum, IOutputStream* log) {
    if (!historyDir.IsDirectory()) {
        return;
    }
    try {
        TVector<TString> names;
        historyDir.ListNames(names);
        for (const TString& name : names) {
            int num = 0;
            if (TryFromString<int>(name, num) && num + HISTORY_SIZE <= lastResultNum + 1) {
                const TFsPath dir = historyDir / name;
                dir.ForceDelete();
            }
        }
    } catch (...) {
        TRACE_LINE(log, LogPrefix() << "Failed to delete old results: " << CurrentExceptionMessage());
    }
}

TFsPath FindLastResultDir(const TFsPath& historyDir, int lastResultNum) {
    if (!historyDir.IsDirectory()) {
        return {};
    }
    TVector<TString> names;
    historyDir.ListNames(names);
    for (const TString& name : names) {
        int num = 0;
        if (TryFromString<int>(name, num) && num == lastResultNum) {
            return historyDir / name;
        }
    }
    return {};
}

void WriteResultDiff(const TFsPath& prevResultDir, const TFsPath& currResultDir) {
    if (!prevResultDir.IsDirectory() || !currResultDir.IsDirectory()) {
        return;
    }
    TVector<TString> testCaseNames;
    currResultDir.ListNames(testCaseNames);
    for (const TString& testCaseName : testCaseNames) {
        const TFsPath currTestCaseDir = currResultDir / testCaseName;
        const TFsPath prevTestCaseDir = prevResultDir / testCaseName;
        if (!currTestCaseDir.IsDirectory() || !prevTestCaseDir.IsDirectory()) {
            continue;
        }
        const TFsPath diffDir = currTestCaseDir / "diff";
        diffDir.MkDirs();

        TFileOutput log(diffDir / "summary.txt");
        log << "Time: " << Now().FormatLocalTime("%F %T") << Endl;
        log << "Prev: " << prevResultDir << Endl;
        log << "Curr: " << currResultDir << Endl;
        log << Endl << Sprintf("%-16s %5s %5s", "", "-", "+") << Endl;

        TVector<TString> datasetNames;
        currTestCaseDir.ListNames(datasetNames);
        for (const TString& datasetName : datasetNames) {
            TStringBuf shortName = datasetName;
            if (!shortName.ChopSuffix(".tsv")) {
                continue;
            }
            const TFsPath currDatasetPath = currTestCaseDir / datasetName;
            const TFsPath prevDatasetPath = prevTestCaseDir / datasetName;
            WriteDatasetDiff(prevDatasetPath, currDatasetPath, diffDir, TString(shortName), &log);
        }
    }
}

void WriteDatasetDiff(const TFsPath& prevFilePath, const TFsPath& currFilePath,
    const TFsPath& diffDir, const TString& diffName, IOutputStream* log)
{
    if (!prevFilePath.IsFile() || !currFilePath.IsFile()) {
        return;
    }
    TVector<TString> prevLines = LoadLines(prevFilePath);
    TVector<TString> currLines = LoadLines(currFilePath);

    // Skip header
    if (prevLines.empty() || currLines.empty() || prevLines[0] != currLines[0]) {
        TRACE_LINE(log, diffName << " - incompatible tsv headers");
        return;
    }
    const TString header = currLines[0];
    prevLines.erase(prevLines.begin());
    currLines.erase(currLines.begin());

    const TFsPath separatedDiffDir = diffDir / "separated";
    separatedDiffDir.MkDirs();
    TFileOutput diffOutput(diffDir / (diffName + "_diff.tsv"));
    TFileOutput minusOutput(separatedDiffDir / (diffName + "_minus.tsv"));
    TFileOutput plusOutput(separatedDiffDir / (diffName + "_plus.tsv"));

    diffOutput << "diff\t" << header << '\n';
    minusOutput << header << '\n';
    plusOutput << header << '\n';

    const THashSet<TString> currLineSet(currLines.begin(), currLines.end());
    int minusCount = 0;
    for (const TString& line : prevLines) {
        if (!currLineSet.contains(line)) {
            diffOutput << "-\t" << line << '\n';
            minusOutput << line << '\n';
            minusCount++;
        }
    }

    const THashSet<TString> prevLineSet(prevLines.begin(), prevLines.end());
    int plusCount = 0;
    for (const TString& line : currLines) {
        if (!prevLineSet.contains(line)) {
            diffOutput << "+\t" << line << '\n';
            plusOutput << line << '\n';
            plusCount++;
        }
    }

    TRACE_LINE(log, Sprintf("%-16s %5d %5d", diffName.c_str(), minusCount, plusCount));
}

} // namespace

void PrepareBatchResultDir(const TFsPath& batchResultsDir, TFsPath* prevResultDir,
    TFsPath* currResultDir, IOutputStream* log)
{
    PrepareResultDir(batchResultsDir, prevResultDir, currResultDir, log);
}

void WriteBatchResultDiff(const TFsPath& prevResultDir, const TFsPath& currResultDir) {
    WriteResultDiff(prevResultDir, currResultDir);
}

} // namespace NGranet
