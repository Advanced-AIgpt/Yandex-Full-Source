#include "metrics.h"
#include "table_formatter.h"
#include <util/generic/ymath.h>
#include <util/stream/format.h>
#include <util/string/printf.h>

namespace NGranet {

// ~~~~ TErrorMetrics ~~~~

double& TErrorMetrics::CounterRW(bool isExpectedPositive, bool isResultPositive) {
    if (isExpectedPositive) {
        if (isResultPositive) {
            return TruePositive;
        } else {
            return FalseNegative;
        }
    } else {
        if (isResultPositive) {
            return FalsePositive;
        } else {
            return TrueNegative;
        }
    }
}

void TErrorMetrics::AddSample(double weight, bool isExpectedPositive, bool isResultPositive, bool isTaggerError) {
    CounterRW(isExpectedPositive, isResultPositive) += weight;
    if (isTaggerError) {
        TaggerMismatch += weight;
    }
}

void TErrorMetrics::AddOther(const TErrorMetrics& other) {
    FalsePositive += other.FalsePositive;
    FalseNegative += other.FalseNegative;
    TruePositive += other.TruePositive;
    TrueNegative += other.TrueNegative;
    TaggerMismatch += other.TaggerMismatch;
}

// ~~~~ TIntentMetrics ~~~~

const TErrorMetrics& TIntentMetrics::Errors(bool byWeight) const {
    return byWeight ? ErrorsByWeight : ErrorsByCount;
}

bool TIntentMetrics::HasAnyError() const {
    return ErrorsByCount.HasAnyError() || ErrorsByWeight.HasAnyError();
}

double TIntentMetrics::TimeAvg() const {
    return SafeRatio(TimeSum, SampleCount, 0);
}

void TIntentMetrics::AddSample(const TSampleProcessingInfo& sample) {
    const bool isTaggerError = sample.Expected.IsPositive
        && sample.Actual.IsPositive
        && !sample.Expected.CheckResult(sample.Actual, sample.CompareSlotsByTop);
    SampleCount++;
    ErrorsByCount.AddSample(1., sample.Expected.IsPositive, sample.Actual.IsPositive, isTaggerError);
    ErrorsByWeight.AddSample(sample.Weight, sample.Expected.IsPositive, sample.Actual.IsPositive, isTaggerError);
    TimeSum += sample.Time;
    TimeMax = Max(TimeMax, sample.Time);
}

void TIntentMetrics::AddOther(const TIntentMetrics& other) {
    SampleCount += other.SampleCount;
    ErrorsByCount.AddOther(other.ErrorsByCount);
    ErrorsByWeight.AddOther(other.ErrorsByWeight);
    TimeSum += other.TimeSum;
    TimeMax = Max(TimeMax, other.TimeMax);
}

// ~~~~ TBatchMetrics ~~~~

void TBatchMetrics::AddIntent(const TString& name, const TIntentMetrics& metrics) {
    Metrics[name].AddOther(metrics);
}

void TBatchMetrics::AddOther(const TBatchMetrics& other) {
    for (const auto& [name, values] : other.Metrics) {
        Metrics[name].AddOther(values);
    }
}

// ~~~~ Selection report formatting ~~~~

static void AddSelectionReportRow(TStringBuf name, const TErrorMetrics& metrics, TTableFormatter *table) {
    Y_ENSURE(table);
    const double positive = metrics.ResultPositive();
    const double negative = metrics.ResultNegative();
    const double total = metrics.Total();
    table->BeginRow();
    table->AddCell(name);
    table->AddCell(Sprintf("%9g", positive));
    table->AddCell(Sprintf("%5.2f%%", SafeRatio(positive, total) * 100));
    table->AddCell(Sprintf("%9g", negative));
    table->AddCell(Sprintf("%5.2f%%", SafeRatio(negative, total) * 100));
    table->AddCell(Sprintf("%9g", total));
}

void PrintSelectionReport(const TIntentMetrics& metrics, TStringBuf name, IOutputStream* out,
    const TString& indent) {
    Y_ENSURE(out);
    TTableFormatter table;
    table.SetIndent(indent).DisableHorSeparators();
    table.AddColumn(name, 10, false);
    table.AddColumn("Pos", 9, true);
    table.AddColumn("Pos%", 6, true);
    table.AddColumn("Neg", 9, true);
    table.AddColumn("Neg%", 6, true);
    table.AddColumn("Total", 9, true);
    AddSelectionReportRow("By count:", metrics.ErrorsByCount, &table);
    AddSelectionReportRow("By weight:", metrics.ErrorsByWeight, &table);
    *out << table;
    *out << indent << Sprintf("Time avg: %4.0f µs", metrics.TimeAvg()) << Endl;
    *out << indent << Sprintf("Time max: %4.0f µs", metrics.TimeMax) << Endl;
}

// ~~~~ Testing report formatting ~~~~

static void PrintTestingReportHeader(TStringBuf name, bool needTimings, TTableFormatter* table) {
    Y_ENSURE(table);
    table->AddColumn(name, 30, false);
    table->AddColumn("Precis", 6, true);
    table->AddColumn("Recall", 6, true);
    table->AddColumn("Tagger", 6, true);
    table->AddColumn("Excess", 10, true);
    table->AddColumn("Lost", 10, true);
    table->AddColumn("Target", 6, true);
    table->AddColumn("Total", 6, true);
    if (needTimings) {
        table->AddColumn("TimeAvg", 7, true);
        table->AddColumn("TimeMax", 7, true);
    }
}

void PrintTestingReportLine(TStringBuf name, const TIntentMetrics& metrics, bool byWeight, bool needTimings,
    TTableFormatter* table)
{
    Y_ENSURE(table);
    const TErrorMetrics& errors = metrics.Errors(byWeight);
    table->BeginRow();
    table->AddCell(name);
    table->AddCell(Sprintf("%.3f", errors.Precision()));
    table->AddCell(Sprintf("%.3f", errors.Recall()));
    table->AddCell(Sprintf("%.3f", errors.TaggerAccuracy()));
    table->AddCell(Sprintf("%.0f %3.0f%%", errors.FalsePositive, 100 * (1 - errors.Precision())));
    table->AddCell(Sprintf("%.0f %3.0f%%", errors.FalseNegative, 100 * (1 - errors.Recall())));
    table->AddCell(Sprintf("%.0f", errors.ExpectedPositive()));
    table->AddCell(Sprintf("%.0f", errors.Total()));
    if (needTimings) {
        table->AddCell(Sprintf("%.0f µs", metrics.TimeAvg()));
        table->AddCell(Sprintf("%.0f µs", metrics.TimeMax));
    }
}

void PrintTestingReport(const TIntentMetrics& metrics, TStringBuf name, bool needTimings,
    IOutputStream* out, const TString& indent)
{
    Y_ENSURE(out);
    TTableFormatter table;
    table.SetIndent(indent).DisableHorSeparators();
    PrintTestingReportHeader(name, needTimings, &table);
    PrintTestingReportLine("By count:", metrics, false, needTimings, &table);
    PrintTestingReportLine("By weight:", metrics, true, needTimings, &table);
    *out << table;
}

void PrintTestingReport(const TBatchMetrics& metrics, bool byWeight, bool needTimings,
    IOutputStream* out, const TString& indent)
{
    TTableFormatter table;
    table.SetIndent(indent);
    const TString name = byWeight ? "By weight" : "By count (unique samples)";
    PrintTestingReportHeader(name, needTimings, &table);
    for (const auto& [name, intentMetrics] : metrics.Metrics) {
        PrintTestingReportLine(name, intentMetrics, byWeight, needTimings, &table);
    }
    *out << table;
}

void PrintTestingReport(const TBatchMetrics& metrics, IOutputStream* out, const TString& indent) {
    Y_ENSURE(out);
    PrintTestingReport(metrics, false, true, out, indent);
    PrintTestingReport(metrics, true, true, out, indent);
}

} // namespace NGranet
