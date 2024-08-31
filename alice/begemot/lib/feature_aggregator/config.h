#pragma once

#include <alice/begemot/lib/feature_aggregator/proto/config.pb.h>

#include <search/begemot/core/filesystem.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/hash.h>
#include <util/generic/yexception.h>

namespace NAlice::NFeatureAggregator {

inline constexpr TStringBuf RESERVED_FEATURE_NAME = "RESERVED";

class TValidationException : public yexception {};

TFeatureAggregatorConfig ReadConfigFromProtoTxtString(TStringBuf str, bool ignoreUnknownRules = false);

THashMap<ELanguage, TFeatureAggregatorConfig> LoadConfigs(const NBg::TFileSystem& fs, bool loadFreshConfig = false);

} // namespace NAlice::NFeatureAggregator
