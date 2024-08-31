#include "translations.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/resource/resource.h>

namespace NAlice::NHollywood::NWeather {

namespace {

THashMap<TString, TString> GenerateTranslationsMap(const TStringBuf lang) {
    TString rawTranslations = NResource::Find("translations.json");
    NJson::TJsonValue map;
    NJson::ReadJsonFastTree(rawTranslations, &map);

    // Убираем уровень вложенности посередине
    // вместо "язык -> кейсет -> ключ" оставляем "язык -> ключ"
    THashMap<TString, TString> result;
    const auto& langMap = map[lang];
    for (const auto& subMap: langMap.GetMap()) {
        for (const auto& pair: subMap.second.GetMap()) {
            result[pair.first] = pair.second.GetString();
        }
    }
    return result;
}

const THashMap<TString, TString> AR_TRANSLATIONS_MAP = GenerateTranslationsMap("ar");
const THashMap<TString, TString> RU_TRANSLATIONS_MAP = GenerateTranslationsMap("ru");

const THashMap<TString, TString>& SelectTranslationsMapByLanguage(const ELanguage language) {
    switch(language) {
    case ELanguage::LANG_ARA:
        return AR_TRANSLATIONS_MAP;
    default:
        return RU_TRANSLATIONS_MAP;
    }
}

}

TTranslations::TTranslations(TRTLogger& logger, const ELanguage language)
    : Logger_{logger}
    , TranslationsMap_(SelectTranslationsMapByLanguage(language))
{
}

TString TTranslations::Translate(const TString& key) const {
    const auto iter = TranslationsMap_.find(key);
    if (iter.IsEnd()) {
        LOG_ERROR(Logger_) << "Unknown weather translation key " << key.Quote();
        return key;
    }
    return iter->second;
}

} // namespace NAlice::NHollywood::NWeather
