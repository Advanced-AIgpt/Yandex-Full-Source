#pragma once

#include <library/cpp/getopt/last_getopt.h>
#include <alice/nlu/granet/lib/test/dataset.h>
#include <alice/nlu/granet/lib/test/dataset_processor.h>
#include <alice/nlu/granet/lib/test/sample_creator.h>

namespace NGranet {

NLastGetopt::TOpts CreateOptions();

void AddLanguageOption(NLastGetopt::TOpts* opts, ELanguage* result);
void AddDomainOption(NLastGetopt::TOpts* opts, TGranetDomain* domain);
void AddGrammarPathOption(NLastGetopt::TOpts* opts, TFsPath* result);
void AddSourceDirsOption(NLastGetopt::TOpts* opts, TVector<TFsPath>* result);
void AddFormNameOption(NLastGetopt::TOpts* opts, TString* result);
void AddEntityNameOption(NLastGetopt::TOpts* opts, TString* result);
void AddOutputColumnFilterOptions(NLastGetopt::TOpts* opts, ESampleComponentFlags* result);
void AddSlotComponentsOptions(NLastGetopt::TOpts* opts, TDatasetProcessorOptions* result);
void AddEntitySourceOptions(NLastGetopt::TOpts* opts, EEntitySourceTypes* result);
void AddDisableParserOptimizationOption(NLastGetopt::TOpts* opts, bool* result);
void AddEnableParserLogOption(NLastGetopt::TOpts* opts, bool* result);
void AddSampleCacheSizeOption(NLastGetopt::TOpts* opts, TMaybe<size_t>* result);
void AddCollectBlockersOption(NLastGetopt::TOpts* opts, bool* result);
void AddReportDirOption(NLastGetopt::TOpts* opts, TFsPath* result);
void AddTextOption(NLastGetopt::TOpts* opts, TString* result);
void AddWizextraOption(NLastGetopt::TOpts* opts, TString* result);
void AddBaseDatasetOption(NLastGetopt::TOpts* opts, TVector<TFsPath>* result);

} // namespace NGranet
