#include "dataset_builder.h"
#include "fetcher.h"
#include <alice/nlu/granet/lib/sample/tag.h>
#include <alice/nlu/granet/lib/user_entity/collect_from_context.h>
#include <alice/nlu/granet/lib/utils/json_utils.h>
#include <alice/nlu/granet/lib/utils/trace.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <dict/dictutil/dictutil.h>
#include <library/cpp/iterator/zip.h>
#include <library/cpp/json/json_writer.h>
#include <util/charset/wide.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/subst.h>

namespace NGranet {

using namespace NJson;

// ~~~~ TDatasetBuilder ~~~~

TDatasetBuilder::TDatasetBuilder(const TOptions& options, IOutputStream* log)
    : Options(options)
    , Log(log)
{
}

void TDatasetBuilder::BuildDataset() {
    TRACE_LINE(Log, LogPrefix() << "Creating dataset " << Options.OutputPath);

    const TString extension = Options.InputPath.GetExtension();
    if (extension == "tsv") {
        BuildFromTsv();
    } else if (extension == "ndjson") {
        BuildFromNdjson();
    } else {
        BuildFromTxt();
    }

    TRACE_LINE(Log, LogPrefix() << "  " << NonTextCount << " non text samples was skipped.");
    TRACE_LINE(Log, LogPrefix() << "  " << DuplicatedCount << " duplicated samples was skipped.");
    TRACE_LINE(Log, LogPrefix() << "  " << AddedCount << " samples was added.");
}

TVector<TString> TDatasetBuilder::GetColumnNamesOption() const {
    return StringSplitter(Options.Columns).SplitBySet(",; ").SkipEmpty().ToList<TString>();
}

void TDatasetBuilder::BuildFromTsv() {
    TTsvReader<> input(Options.InputPath);

    TVector<TString> columns = GetColumnNamesOption();
    if (columns.empty()) {
        columns = input.GetHeader()->GetNames();
    }
    TTsvWriter<ESampleColumnId> output(Options.OutputPath, columns);

    TTsvLine<> srcLine;
    while (!IsLimitReached() && input.ReadLine(&srcLine)) {
        TSampleTsvLine destLine(output.GetHeader());
        for (const TString& column : columns) {
            destLine[column] = srcLine[column];
        }
        WriteSample(&destLine, &output);
    }
}

void TDatasetBuilder::BuildFromNdjson() {
    TFileInput input(Options.InputPath);

    TVector<TString> columnNames = GetColumnNamesOption();
    if (columnNames.empty()) {
        if (Options.ContextStoragePath.IsDefined()) {
            columnNames.push_back(GetColumnName(ESampleColumnId::Context));
        }
        columnNames.push_back(GetColumnName(ESampleColumnId::Text));
    }

    TTsvWriter<ESampleColumnId> output(Options.OutputPath, columnNames);

    TString srcLine;
    while (!IsLimitReached() && input.ReadLine(srcLine)) {
        const TJsonValue srcJson = NJsonUtils::ReadJsonStringVerbose(srcLine, Options.InputPath);
        TSampleTsvLine destLine(output.GetHeader());

        for (const TString& columnName : destLine.GetNames()) {
            if (columnName == GetColumnName(ESampleColumnId::Context)) {
                destLine[columnName] = JoinSeq(",", ContextCollector.CollectFromSample(srcJson));
                continue;
            }
            const TJsonValue* value = srcJson.GetValueByPath(columnName);
            if (!value && columnName == GetColumnName(ESampleColumnId::Text)) {
                value = srcJson.GetValueByPath("request.utterance.text");
                if (!value) {
                    value = srcJson.GetValueByPath("utterance.text");
                }
                if (!value) {
                    value = srcJson.GetValueByPath("utterance_text");
                }
                Y_ENSURE(value, "Can't find text");
            }
            Y_ENSURE(value, "Column " + Cite(columnName) + " not found in sample");
            TString str = value->IsString()
                ? value->GetStringSafe()
                : WriteJson(value, false, true);
            SubstGlobal(str, '\n', ' ');
            SubstGlobal(str, '\t', ' ');
            destLine[columnName] = str;
        }

        WriteSample(&destLine, &output);
    }

    if (Options.ContextStoragePath.IsDefined()) {
        ContextCollector.CreateStorage().SaveToPath(Options.ContextStoragePath);
    }
}

void TDatasetBuilder::BuildFromTxt() {
    TFileInput input(Options.InputPath);
    TTsvWriter<ESampleColumnId> output(Options.OutputPath, {GetColumnName(ESampleColumnId::Text)});

    TString srcLine;
    while (!IsLimitReached() && input.ReadLine(srcLine)) {
        SubstGlobal(srcLine, '\t', ' ');
        TSampleTsvLine destLine(output.GetHeader());
        destLine[0] = srcLine;
        WriteSample(&destLine, &output);
    }
}

bool TDatasetBuilder::IsLimitReached() const {
    return Options.SampleCountLimit != NPOS && AddedCount >= Options.SampleCountLimit;
}

void TDatasetBuilder::WriteSample(TSampleTsvLine* line, TTsvWriter<ESampleColumnId>* output) {
    Y_ENSURE(line);

    // Make basic normalization.
    const TString original = (*line)[ESampleColumnId::Text];
    const TString normalized = NormalizeForDataset(original);
    (*line)[ESampleColumnId::Text] = normalized;

    // Filter samples
    if (!AnyOf(UTF8ToWide(normalized), IsAlnum)) {
        NonTextCount++;
        ReportProgress(NonTextCount, "skipped due not a text.", original);
        return;
    }

    const TString sampleKey = GetSampleKey(*line);
    const auto [it, isNew] = UniqueSamples.try_emplace(sampleKey, 0);
    it->second += line->Value(ESampleColumnId::Weight, 1.);

    if (Options.MergeDuplicated && !isNew) {
        DuplicatedCount++;
        ReportProgress(DuplicatedCount, "skipped due duplicate.", original);
        return;
    }

    // Add sample
    AddedCount++;
    ReportProgress(AddedCount, "processed.", original);
    output->WriteLine(*line);
}

// static
TString TDatasetBuilder::GetSampleKey(const TSampleTsvLine& line) {
    const size_t weightIndex = line.GetHeader()->TryGetColumnIndex(ESampleColumnId::Weight);
    if (weightIndex == NPOS) {
        return JoinSeq("\t", line.GetValues());
    }
    TVector<TString> values = line.GetValues();
    values[weightIndex].clear();
    return JoinSeq("\t", values);
}

TString TDatasetBuilder::NormalizeForDataset(TString text) const {
    if (Options.ShouldNormalize) {
        for (char c : TStringBuf("\t\r\n:;,.-*!?()'\"")) {
            SubstGlobal(text, c, ' ');
        }
    } else {
        // Remove symbols used in tsv
        for (char c : TStringBuf("\t\r\n")) {
            SubstGlobal(text, c, ' ');
        }
        // Remove "bad" symbols
        TStringBuilder out;
        int level = 0;
        for (const char c : text) {
            if (c == '(') {
                level++;
            } else if (c == ')') {
                level = Max(0, level - 1);
            }
            if (level > 0 || c == ')' || c == '\'') {
                if (Options.KeepTags) {
                    // Keep any symbols inside tag description (except those used in tsv format).
                    // They will be removed later in RemoveTaggerMarkup.
                    out << c;
                }
            } else if (c == ';') {
                // used in cgi-parameter 'wizextra'
                out << ',';
            } else if (c == '"' ) {
                // can be used in tsv
                out << ' ';
            } else {
                out << c;
            }
        }
    }
    TUtf16String wide = UTF8ToWide(text);
    if (Options.ToLowerCase || Options.ShouldNormalize) {
        wide = ToLower(Options.Lang, wide);
    }
    Collapse(wide);
    Strip(wide);
    return WideToUTF8(wide);
}

void TDatasetBuilder::ReportProgress(size_t counter, TStringBuf message, TStringBuf current) {
    if (IsPowerOf10(counter)) {
        TRACE_LINE(Log, LogPrefix() << "  Progress..." << LeftPad(counter, 8)
            << " " << message << " Current: " << Cite(current));
    }
}

} // namespace NGranet
