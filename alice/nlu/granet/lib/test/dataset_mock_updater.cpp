#include "dataset_mock_updater.h"
#include "fetcher.h"
#include <alice/nlu/granet/lib/sample/sample_mock.h>
#include <alice/nlu/granet/lib/utils/parallel_processor.h>
#include <alice/nlu/granet/lib/utils/trace.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/json/json_writer.h>
#include <util/generic/xrange.h>
#include <util/string/join.h>
#include <util/system/tempfile.h>

namespace NGranet {

// ~~~~ TDatasetMockUpdater ~~~~

TDatasetMockUpdater::TDatasetMockUpdater(const TOptions& options, IOutputStream* log)
    : Options(options)
    , Log(log)
{
}

static TFsPath MakeTempPathNear(TFsPath path) {
    do {
        path = path.GetPath() + ".tmp";
    } while (path.Exists());
    return path;
}

void TDatasetMockUpdater::Update() {
    if (Options.DestPath.IsDefined() && Options.DestPath != Options.SrcPath) {
        DoUpdate(Options.DestPath);
        return;
    }

    // Inplace
    Y_ENSURE(Options.SrcPath.IsFile());
    const TFsPath tempFile = MakeTempPathNear(Options.SrcPath);
    const TTempFile tempFileRemover(tempFile);

    DoUpdate(tempFile);

    Y_ENSURE(Options.SrcPath.IsFile());
    Y_ENSURE(tempFile.IsFile());
    tempFile.RenameTo(Options.SrcPath);
}

void TDatasetMockUpdater::DoUpdate(const TFsPath& destPath) {
    TRACE_LINE(Log, LogPrefix() << "Fetch mock for " << Options.SrcPath);

    TTsvSampleDataset reader(Options.SrcPath);
    const TVector<TString> destColumns = MakeDestColumns(reader.GetHeader()->GetNames());
    TTsvWriter<ESampleColumnId> writer(destPath, destColumns);

    THolder<IThreadPool> threadPool = CreateThreadPool(Options.ThreadCount);
    TParallelProcessorWithOrderedPostprocess processor(
        /* process= */ [this](TSampleData data) {
            ProcessSample(&data);
            return data;
        },
        /* postprocess= */ [this, &writer](const TSampleData& data) {
            if (!data.ShouldRemove) {
                writer.WriteLine(data.DestLine);
            }
            ReportProgress(data);
        },
        /* threadPool= */ threadPool.Get(),
        /* resultQueueLimit= */ 1000
    );

    for (const size_t i : xrange(reader.Size())) {
        TSampleData data;
        data.Sample = reader.ReadSample(i);
        data.DestLine = TSampleTsvLine(writer.GetHeader(), data.Sample.TsvLine);
        processor.Push(std::move(data));
    }
    processor.Finalize();
    ReportResults();
}

static void AppendUnique(const TString& item, TVector<TString>* items) {
    if (!IsIn(*items, item)) {
        items->push_back(item);
    }
}

TVector<TString> TDatasetMockUpdater::MakeDestColumns(const TVector<TString>& srcColumns) const {
    TVector<TString> destColumns = srcColumns;
    if (Options.ShouldUpdateMock) {
        AppendUnique(GetColumnName(ESampleColumnId::Mock), &destColumns);
    }
    if (Options.ShouldUpdateEmbeddings) {
        AppendUnique(GetColumnName(ESampleColumnId::Embeddings), &destColumns);
    }
    return destColumns;
}

void TDatasetMockUpdater::ProcessSample(TSampleData* data) const {
    Y_ENSURE(data);

    const bool needUpdateMock = Options.ShouldUpdateMock
        && (!Options.OnlyMissing || IsSampleMockStrGood(data->DestLine[ESampleColumnId::Mock]));
    const bool needUpdateEmbeddings = Options.ShouldUpdateEmbeddings
        && (!Options.OnlyMissing || IsEmbeddingsMockStrGood(data->DestLine[ESampleColumnId::Embeddings]));
    if (!needUpdateMock && !needUpdateEmbeddings) {
        data->Status = SS_UNCHANGED;
        return;
    }

    TBegemotFetcherOptions options;
    options.Wizextra = data->Sample.Wizextra;
    TSampleMock mock;
    TEmbeddingsMock embeddings;
    TString fetcherError;
    if (!FetchSampleMock(data->Sample.CleanText, Options.Domain, &mock, &embeddings, &fetcherError, options)) {
        data->Status = SS_OTHER_ERROR;
        data->ErrorMessage = "Fetcher error: " + fetcherError;
    } else if (mock.FstText.empty() || mock.Tokens.empty()) {
        data->Status = SS_EMPTY_REQUEST;
        data->ErrorMessage = "Sample with empty request.";
    } else {
        data->Status = SS_SUCCESS;
    }

    if (data->Status != SS_SUCCESS && Options.RemoveBadSamples) {
        data->ShouldRemove = true;
        return;
    }

    if (needUpdateMock) {
        NJson::TJsonValue mockJson = mock.ToJson();
        if (!fetcherError.empty()) {
            mockJson["FetcherError"] = fetcherError;
        }
        data->DestLine[ESampleColumnId::Mock] = NJson::WriteJson(mockJson, false, true);
    }
    if (needUpdateEmbeddings) {
        NJson::TJsonValue embeddingsJson = embeddings.ToJson();
        if (!fetcherError.empty()) {
            embeddingsJson["FetcherError"] = fetcherError;
        }
        data->DestLine[ESampleColumnId::Embeddings] = NJson::WriteJson(embeddingsJson, false, true);
    }
}

void TDatasetMockUpdater::ReportProgress(const TSampleData& data) {
    if (data.Status == SS_UNCHANGED) {
        return;
    } else if (data.Status == SS_SUCCESS) {
        SuccessCount++;
    } else if (data.Status == SS_EMPTY_REQUEST) {
        EmptyCount++;
    } else if (data.Status == SS_OTHER_ERROR) {
        ErrorCount++;
    }
    size_t processedCount = SuccessCount + EmptyCount + ErrorCount;
    if (IsPowerOf10(processedCount)) {
        TRACE_LINE(Log, LogPrefix() << "  Progress..." << LeftPad(processedCount, 8)
            << " processed. Current: " << Cite(data.Sample.TaggedText));
    }
    if (data.Status != SS_SUCCESS) {
        TRACE_LINE(Log, LogPrefix() << "  " << data.ErrorMessage);
        TRACE_LINE(Log, "    Sample index: " << data.Sample.TsvLine.GetIndex());
        TRACE_LINE(Log, "    Sample text: " << data.Sample.TaggedText);
    }
    if (data.ShouldRemove) {
        TRACE_LINE(Log, "    Sample has been removed");
    }
}

void TDatasetMockUpdater::ReportResults() {
    const size_t badCount = ErrorCount + EmptyCount;
    if (badCount > 0) {
        TRACE_LINE(Log, LogPrefix() << "  Done. But there was errors!");
        TRACE_LINE(Log, "      " << ErrorCount << " failed samples.");
        TRACE_LINE(Log, "      " << EmptyCount << " samples with empty request.");
        if (Options.RemoveBadSamples) {
            TRACE_LINE(Log, "      These samples have been removed.");
        } else {
            TRACE_LINE(Log, "      Use --remove-bad-samples option to remove these samples.");
        }
    }
    TRACE_LINE(Log, LogPrefix() << "  " << SuccessCount << " samples were updated.");
}

} // namespace NGranet
