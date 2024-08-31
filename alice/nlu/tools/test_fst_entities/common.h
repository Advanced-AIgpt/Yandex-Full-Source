#pragma once

#include <alice/nlu/libs/fst/fst_base.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/getopt/small/last_getopt_opts.h>
#include <util/folder/path.h>
#include <util/generic/vector.h>

struct TConfigFst {
    TString FstName;
    TString EntityTypeName;
    TString Language;

    TFsPath DatasetPath;
    TFsPath PositiveTablePath;
};

THolder<NAlice::TFstBase> MakeFst(const TString& fstName, const ELanguage language);

NLastGetopt::TOpts CreateOptions();
void AddFstConfigOpts(NLastGetopt::TOpts& opts, TConfigFst& config);
