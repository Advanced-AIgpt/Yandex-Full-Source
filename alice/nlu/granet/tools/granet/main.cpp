#include "batch_applet.h"
#include "dataset_applet.h"
#include "debug_applet.h"
#include "normalizer.h"
#include "grammar_applet.h"
#include <alice/nlu/granet/lib/test/table_formatter.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <dict/nerutil/tstimer.h>
#include <library/cpp/getopt/modchooser.h>
#include <util/datetime/cputimer.h>
#include <util/string/printf.h>

using namespace NGranet;

// For enable timings build project with ENABLE_DEBUG_TIMERS macro:
//   ya make -r -DCFLAGS=-DENABLE_DEBUG_TIMERS
static void PrintTimings() {
    #ifdef ENABLE_DEBUG_TIMERS
    TVector<const NThreadSafeTimer::TTimer*> timers = NThreadSafeTimer::GetTimers();
    SortBy(timers, std::mem_fn(&NThreadSafeTimer::TTimer::Name));

    TTableFormatter table;
    table.AddColumn("Timer", 60);
    table.AddColumn("Hits", 7, true);
    table.AddColumn("Time", 11, true);
    for (const NThreadSafeTimer::TTimer* timer : timers) {
        table.BeginRow();
        table.AddCell(timer->Name);
        table.AddCell(timer->HitsCount);
        table.AddCell(Sprintf("%.3f s", CyclesToDuration(timer->CyclesCount).SecondsFloat()));
    }
    Cerr << Endl << table;
    #endif
}

int Run(int argc, const char** argv) {
    DEBUG_TIMER("Run");

    TModChooser modChooser;

    modChooser.AddMode("dataset", RunDatasetApplet,
        "Dataset tools: create dataset, fetch entities, select samples by grammar, etc");

    modChooser.AddMode("batch", RunBatchApplet,
        "Batch testing: test grammar using datasets specified in config file");

    modChooser.AddMode("debug", RunDebugApplet,
        "Debug sample parsing, explore entities, etc");

    modChooser.AddMode("normalize", RunNormalizeApplet,
        "Normalization applet: normalize strings from specified column of tsv-file "
        "by different types of normalization and save results as new column");

    modChooser.AddMode("grammar", RunGrammarApplet,
        "Tools for Granet source texts");

    return modChooser.Run(argc, argv);
}

int main(int argc, const char** argv) {
    try {
        const int error = Run(argc, argv);
        PrintTimings();
        return error;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
