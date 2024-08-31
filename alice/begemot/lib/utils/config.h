#pragma once

#include <alice/library/proto/proto.h>

#include <search/begemot/core/filesystem.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/hash.h>

namespace NAlice {

template <typename TProtoMessage>
THashMap<ELanguage, TProtoMessage> LoadLanguageConfigs(const NBg::TFileSystem& fs, TStringBuf configFileName, bool permissive = false) {
    THashMap<ELanguage, TProtoMessage> configs;
    for (const TString& dir : fs.List()) {
        const ELanguage lang = LanguageByName(dir);
        if (lang == LANG_UNK) {
            continue;
        }
        const TFsPath path = TFsPath(dir) / configFileName;
        if (!fs.Exists(path)) {
            continue;
        }
        const TString configString = fs.LoadInputStream(path)->ReadAll();
        configs[lang] = ParseProtoText<TProtoMessage>(configString, permissive);
    }
    return configs;
}

bool IsEnabledByExperiments(const ::google::protobuf::RepeatedPtrField<TProtoStringType>& expressions,
    const THashSet<TString>& experiments);

} // namespace NAlice
