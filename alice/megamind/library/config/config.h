#pragma once

#include <alice/megamind/library/config/protos/classification_config.pb.h>
#include <alice/megamind/library/config/protos/config.pb.h>

namespace NAlice {

inline ui32 GetMaxAttempts(const TConfig::TSource& config) {
    return (config.HasAliceFetcherNoRetry() && config.GetAliceFetcherNoRetry()) ? ui32{1} : config.GetMaxAttempts();
}

bool ValidateSource(TStringBuf sourceName, const TConfig::TSource& config, IOutputStream& output);
bool ValidateSources(TConfig& config, IOutputStream& output);

const TString* GetCurrentDC(const TConfig& config);

TConfig LoadConfig(int argc, const char** argv);
NMegamind::TClassificationConfig LoadClassificationConfig(const TString& path);

} // namespace NAlice
