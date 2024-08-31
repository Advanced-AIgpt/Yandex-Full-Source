#include "common.h"

#include <alice/nlu/libs/fst/prefix_data_loader.h>
#include <alice/nlu/libs/fst/fst_calc.h>
#include <alice/nlu/libs/fst/fst_date_time_range.h>
#include <alice/nlu/libs/fst/fst_date_time.h>
#include <alice/nlu/libs/fst/fst_custom_hierarchy.h>
#include <alice/nlu/libs/fst/fst_float.h>
#include <alice/nlu/libs/fst/fst_geo.h>
#include <alice/nlu/libs/fst/fst_units.h>
#include <alice/nlu/libs/fst/fst_num.h>
#include <alice/nlu/libs/fst/fst_time.h>
#include <alice/nlu/libs/fst/fst_weekdays.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/testing/unittest/env.h>

TFsPath GetDataRoot() {
    return BuildRoot() + "/alice/begemot/lib/fst/data";
}

TString GetFstDataPath(const TStringBuf& fstName, const ELanguage lang) {
    return GetDataRoot() / "fst" / fstName / "fst" / IsoNameByLanguage(lang) / fstName;
}

THolder<NAlice::TFstBase> MakeFst(const TString& fstName, const ELanguage language) {
    const NAlice::TPrefixDataLoader prefixDataLoader(GetFstDataPath(fstName, language));
    if (fstName == "calc") {
        return MakeHolder<NAlice::TFstCalc>(prefixDataLoader);
    } else if (fstName == "date") {
        return MakeHolder<NAlice::TFstDateTime>(prefixDataLoader);
    } else if (fstName == "datetime") {
        return MakeHolder<NAlice::TFstDateTime>(prefixDataLoader);
    } else if (fstName == "datetime_range") {
        return MakeHolder<NAlice::TFstDateTimeRange>(prefixDataLoader);
    } else if (fstName == "fio") {
        return MakeHolder<NAlice::TFstCustomHierarchy>(prefixDataLoader);
    } else if (fstName == "float") {
        return MakeHolder<NAlice::TFstFloat>(prefixDataLoader);
    } else if (fstName == "geo") {
        return MakeHolder<NAlice::TFstGeo>(prefixDataLoader);
    } else if (fstName == "num") {
        return MakeHolder<NAlice::TFstNum>(prefixDataLoader);
    } else if (fstName == "time") {
        return MakeHolder<NAlice::TFstTime>(prefixDataLoader);
    } else if (fstName == "units_time") {
        return MakeHolder<NAlice::TFstUnits>(prefixDataLoader);
    } else if (fstName == "weekdays") {
        return MakeHolder<NAlice::TFstWeekdays>(prefixDataLoader);
    }

    Y_ENSURE(false, "Unknown fst name: " << fstName);
}

NLastGetopt::TOpts CreateOptions() {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts.AddHelpOption('h');
    return opts;
}

void AddFstConfigOpts(NLastGetopt::TOpts& opts, TConfigFst& config) {
    opts.AddLongOption("fst-name", "name fst")
        .RequiredArgument("name")
        .Required()
        .StoreResult(&config.FstName);

    opts.AddLongOption("fst-type", "name of the entity given by the fst")
        .RequiredArgument("type")
        .Required()
        .StoreResult(&config.EntityTypeName);

    opts.AddLongOption("lang", "language")
        .RequiredArgument("lang")
        .DefaultValue("ru")
        .StoreResult(&config.Language);
}
