#pragma once

#include <alice/nlu/granet/lib/grammar/domain.h>
#include <alice/nlu/granet/lib/sample/sample.h>
#include <alice/nlu/granet/lib/sample/sample_mock.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/noncopyable.h>
#include <util/system/env.h>

namespace NGranet {

// For TBegemotFetcherOptions::Url
inline const TString BEGEMOT_PRODUCTION_URL = "http://reqwizard.yandex.net:8891/wizard";
inline const TString BEGEMOT_HAMSTER_URL = "http://hamzard.yandex.net:8891/wizard";

inline const TString WIZCLIENT_MEGAMIND = "megamind";

inline TString GetBegemotDefaultUrl() {
    return GetEnv("GRANET_BEGEMOT", BEGEMOT_HAMSTER_URL);
}

struct TBegemotFetcherOptions {
    TString Url = GetBegemotDefaultUrl();
    TString WizClient = WIZCLIENT_MEGAMIND;
    TString Wizextra;
};

bool FetchSampleMock(TStringBuf text, const TGranetDomain& domain, TSampleMock* sampleMock,
    TEmbeddingsMock* embeddingsMock, TString* errorMessage, const TBegemotFetcherOptions& options = {});

// Fetch sample entities from Begemot and save them into the sample.
// In case of error returns false.
bool FetchSampleEntities(const TSample::TRef& sample, const TGranetDomain& domain,
    const TBegemotFetcherOptions& options = {});

} // namespace NGranet
