#include "common_options.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/granet/lib/test/dataset.h>
#include <dict/dictutil/last_getopt.h>
#include <library/cpp/getopt/last_getopt.h>

namespace NGranet {

NLastGetopt::TOpts CreateOptions() {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts.AddHelpOption('h');
    return opts;
}

void AddLanguageOption(NLastGetopt::TOpts* opts, ELanguage* result) {
    Y_ENSURE(opts);
    opts->AddLongOption("lang", "Language of utterances and grammar")
        .DefaultValue("ru")
        .RequiredArgument("LANG")
        .StoreMappedResultT<TString>(result, TLanguageArgParser());
}

void AddDomainOption(NLastGetopt::TOpts* opts, TGranetDomain* domain) {
    Y_ENSURE(domain);

    AddLanguageOption(opts, &domain->Lang);

    opts->AddLongOption("paskills", "Is for paskills")
        .StoreTrue(&domain->IsPASkills);

    opts->AddLongOption("wizard", "Is for wizard")
        .StoreTrue(&domain->IsWizard);

    opts->AddLongOption("snezhana", "Is for snezhana")
        .StoreTrue(&domain->IsSnezhana);
}

void AddGrammarPathOption(NLastGetopt::TOpts* opts, TFsPath* result) {
    Y_ENSURE(opts);
    opts->AddLongOption('g', "grammar", "Path to grammar (grnt-file)")
        .Required()
        .RequiredArgument("GRAMMAR")
        .StoreResult(result);
}

void AddSourceDirsOption(NLastGetopt::TOpts* opts, TVector<TFsPath>* result) {
    Y_ENSURE(opts);
    opts->AddLongOption("source-dir", "Additional directories to search for imported files")
        .Optional()
        .RequiredArgument()
        .AppendTo(result);
}

void AddFormNameOption(NLastGetopt::TOpts* opts, TString* result) {
    Y_ENSURE(opts);
    opts->AddLongOption("form", "Name of matched form")
        .Optional()
        .RequiredArgument("FORM")
        .StoreResult(result);
}

void AddEntityNameOption(NLastGetopt::TOpts* opts, TString* result) {
    Y_ENSURE(opts);
    opts->AddLongOption("entity", "Name of matched entity")
        .Optional()
        .RequiredArgument("ENTITY")
        .StoreResult(result);
}

template <class TEnum>
static NLastGetopt::TOpt& AddFlagOption(NLastGetopt::TOpts* opts, const TString& name, TFlags<TEnum> flag, TFlags<TEnum>* result) {
    Y_ENSURE(opts);
    Y_ENSURE(result);
    return opts->AddLongOption(name)
        .Optional()
        .DefaultValue(result->HasFlags(flag) ? "YES" : "NO")
        .Handler1T(true, [=](bool value) { SetFlags(result, flag, value); });
}

template <class TEnum>
static NLastGetopt::TOpt& AddFlagOption(NLastGetopt::TOpts* opts, const TString& name, TEnum flag, TFlags<TEnum>* result) {
    return AddFlagOption(opts, name, static_cast<TFlags<TEnum>>(flag), result);
}

void AddOutputColumnFilterOptions(NLastGetopt::TOpts* opts, ESampleComponentFlags* result) {
    AddFlagOption(opts, "keep-weight", SCF_WEIGHT, result);
    AddFlagOption(opts, "keep-reqid", SCF_REQID, result);
    AddFlagOption(opts, "keep-context", SCF_CONTEXT, result);
    AddFlagOption(opts, "keep-wizextra", SCF_WIZEXTRA, result);
    AddFlagOption(opts, "keep-mock", SCF_MOCK, result);
    AddFlagOption(opts, "keep-embeddings", SCF_EMBEDDINGS, result);
    AddFlagOption(opts, "keep-extra", SCF_EXTRA, result);
}

void AddSlotComponentsOptions(NLastGetopt::TOpts* opts, TDatasetProcessorOptions* result) {
    opts->AddLongOption("print-slots", "Print tagger result into positive dataset").StoreTrue(&result->NeedSlots);
    AddFlagOption(opts, "print-slot-values", SPO_NEED_VALUES, &result->SlotPrintingOptions);
    AddFlagOption(opts, "print-slot-types", SPO_NEED_TYPES, &result->SlotPrintingOptions);
    AddFlagOption(opts, "print-slot-value-variants", SPO_NEED_VALUES | SPO_NEED_VARIANTS, &result->SlotPrintingOptions);
}

void AddEntitySourceOptions(NLastGetopt::TOpts* opts, EEntitySourceTypes* result) {
    AddFlagOption(opts, "enable-tsv-entities", EST_TSV, result);
    AddFlagOption(opts, "enable-online-entities", EST_ONLINE, result).Help("Fetch missing entities from Begemot");
    AddFlagOption(opts, "enable-empty-entities", EST_EMPTY, result);
}

void AddDisableParserOptimizationOption(NLastGetopt::TOpts* opts, bool* result) {
    opts->AddLongOption("disable-parser-optimization", "Disable parser optimization")
        .StoreTrue(result);
}

void AddEnableParserLogOption(NLastGetopt::TOpts* opts, bool* result) {
    opts->AddLongOption("enable-parser-log", "Enable parser log")
        .StoreTrue(result);
}

void AddSampleCacheSizeOption(NLastGetopt::TOpts* opts, TMaybe<size_t>* result) {
    opts->AddLongOption("sample-cache-size", "Size of cache of preprocessed samples.")
        .StoreResult(result);
}

void AddCollectBlockersOption(NLastGetopt::TOpts* opts, bool* result) {
    opts->AddLongOption("collect-blockers", "Analyze reasons of negative results. Statistics is stored to REPORT/blockers_*.tsv")
        .StoreTrue(result);
}

void AddReportDirOption(NLastGetopt::TOpts* opts, TFsPath* result) {
    Y_ENSURE(opts);
    opts->AddLongOption("report", "Directory for storing additional processing info")
        .Optional()
        .RequiredArgument("REPORT")
        .StoreResult(result);
}

void AddTextOption(NLastGetopt::TOpts* opts, TString* result) {
    Y_ENSURE(opts);
    opts->AddLongOption('t', "text", "Text of sample")
        .Required()
        .RequiredArgument("TEXT")
        .StoreResult(result);
}

void AddWizextraOption(NLastGetopt::TOpts* opts, TString* result) {
    Y_ENSURE(opts);
    opts->AddLongOption("wizextra", "Additional parts of parameter wizextra sent to begemot while fetching of entities")
        .Optional()
        .RequiredArgument("WIZEXTRA")
        .StoreResult(result);
}

void AddBaseDatasetOption(NLastGetopt::TOpts* opts, TVector<TFsPath>* result) {
    Y_ENSURE(opts);
    opts->AddLongOption('b', "base", "Optional dataset used as source of entities")
        .Optional()
        .RequiredArgument()
        .AppendTo(result);
}

} // namespace NGranet
