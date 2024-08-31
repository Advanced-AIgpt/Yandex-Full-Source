#include "translations.h"
#include "exceptions.h"

#include <library/cpp/json/json_reader.h>
#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/stream/file.h>

namespace NAlice::NNlg {

class TTranslationsContainer : public ITranslationsContainer {
public:
    explicit TTranslationsContainer(THashMap<ELanguage, THashMap<TString, TString>> translations);
    const TString& GetTranslation(const ELanguage language, const TStringBuf key) const final;
private:
    THashMap<ELanguage, THashMap<TString, TString>> Translations_;
};

TTranslationsContainer::TTranslationsContainer(THashMap<ELanguage, THashMap<TString, TString>> translations)
    : Translations_(std::move(translations))
{
}

const TString& TTranslationsContainer::GetTranslation(const ELanguage language, const TStringBuf key) const {
    const auto* langMap = Translations_.FindPtr(language);
    Y_ENSURE_EX(langMap, TTranslationError() << "Failed to retrieve translation for key '" << key << "': unknown language " << IsoNameByLanguage(language));
    const auto* result = langMap->FindPtr(key);
    Y_ENSURE_EX(result, TTranslationError() << "Failed to retrieve translation for language "  << IsoNameByLanguage(language) << " and key '" << key << "'");
    return *result;
}

ITranslationsContainerPtr CreateTranslationsContainerFromFile(const TFsPath& nlgTranslationsJsonFilePath) {
    NJson::TJsonValue json;
    {
        auto fileInput = TFileInput(nlgTranslationsJsonFilePath);
        json = NJson::ReadJsonTree(&fileInput, true);
    }

    Y_ENSURE(json.IsMap(), "Failed to read translations map from file '" << nlgTranslationsJsonFilePath << "'");

    auto translations = THashMap<ELanguage, THashMap<TString, TString>>(json.GetMap().size());

    for (const auto& [langKey, langValue] : json.GetMap()) {
        const auto lang = LanguageByName(langKey);
        Y_ENSURE(lang != ELanguage::LANG_UNK, "Unknown translations language " << langKey);
        auto& langMap = translations[lang];

        Y_ENSURE(langValue.IsMap(), "Expected to get translations map for language " << langKey);
        langMap.reserve(langValue.GetMap().size());
        for (const auto& [key, value] : langValue.GetMap()) {
            Y_ENSURE(value.IsString(), "Expected to get string value for language " << langKey << " and key '" << key << "'");
            langMap.emplace(key, value.GetString());
        }
    }

    return std::make_shared<TTranslationsContainer>(std::move(translations));
}

}
