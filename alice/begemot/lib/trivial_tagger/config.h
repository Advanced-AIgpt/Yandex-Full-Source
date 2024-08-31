#pragma once

#include <alice/begemot/lib/trivial_tagger/proto/config.pb.h>
#include <alice/begemot/lib/fresh_options/proto/fresh_options.pb.h>

#include <search/begemot/core/filesystem.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/hash.h>

namespace NAlice {

TTrivialTaggerConfig ReadTrivialTaggerConfigFromJsonString(TStringBuf str, bool permissive = false);

THashMap<ELanguage, TTrivialTaggerConfig> LoadTrivialTaggerConfigs(const NBg::TFileSystem& fs, const TFsPath& dir);

class TTrivialTaggerConfigValidationException : public yexception {
};

void Validate(const TTrivialTaggerConfig& config);

TTrivialTaggerConfig MergeTrivialTaggerConfigs(
    const TTrivialTaggerConfig* staticConfig,
    const TTrivialTaggerConfig* freshConfig,
    const TVector<TTrivialTaggerConfig>& configPatches,
    const TFreshOptions& freshOptions);

} // namespace NAlice
