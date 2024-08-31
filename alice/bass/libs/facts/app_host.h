#pragma once

#include <alice/bass/libs/app_host/source.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/string.h>

namespace NBASS {

class TFactsAppHostSource : public IAppHostSource {
public:
    TFactsAppHostSource(TStringBuf part, TStringBuf tld, TStringBuf uil);

    NSc::TValue GetSourceInit() const override;

private:
    NSc::TValue Data;
};

} // namespace NBASS
