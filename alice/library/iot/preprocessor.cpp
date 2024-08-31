#include "preprocessor.h"
#include "utils.h"

#include <library/cpp/regex/pcre/regexp.h>
#include <library/cpp/resource/resource.h>

#include <util/charset/utf8.h>
#include <util/generic/string.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/subst.h>


namespace NAlice::NIot {

namespace {

TString LoadResource(const TStringBuf name, const TStringBuf defaultValue = "") {
    TString result;
    return NResource::FindExact(name, &result) ? result : ToString(defaultValue);
}


class TCategorizedVariationsBuilder;

class TCategorizedVariations : public ICategorizedVariations {
public:
    const TVariations& GetVariations(const TStringBuf category) const override {
        if (const auto variations = CategoryToVariations_.FindPtr(category)) {
            return *variations;
        }
        return CategoryToVariations_.at(CATEGORY_UNKNOWN);
    };

    ~TCategorizedVariations() = default;

private:
    THashMap<TString, TVariations> CategoryToVariations_;

    friend class TCategorizedVariationsBuilder;
};

class TCategorizedVariationsBuilder {
public:
    TCategorizedVariationsBuilder(const TStringBuf categorizedVariationsConfigFile)
        : RawData_(categorizedVariationsConfigFile)
    {
    }

    TCategorizedVariations Build() && {
        SplitRawDataIntoCategorizedLines();
        FillResult();

        return std::move(Result_);
    }

private:
    void SplitRawDataIntoCategorizedLines() {
        TString currentCategoriesNames;
        TVector<TString> currentCategoriesLines;

        const auto submitCategoriesLinesIfPresent = [&](){
            if (!currentCategoriesLines.empty() && !currentCategoriesNames.empty()) {
                const auto categoriesNames = StringSplitter(ToLowerUTF8(currentCategoriesNames)).Split('|').ToList<TString>();
                for (const auto& categoryName : categoriesNames) {
                    Copy(currentCategoriesLines.begin(),
                         currentCategoriesLines.end(),
                         back_inserter(CategoryToLines_[categoryName]));
                }
            }
            currentCategoriesLines.clear();
        };

        for (const auto& line : StringSplitter(RawData_).Split('\n').ToList<TString>()) {
            if (CategoryLine.Match(line.c_str())) {
                submitCategoriesLinesIfPresent();
                currentCategoriesNames = line.substr(1, static_cast<int>(line.size()) - 2);
            } else if (!line.empty()) {
                currentCategoriesLines.push_back(line);
            }
        }
        submitCategoriesLinesIfPresent();

        if (CategoryToLines_.contains(CATEGORY_ALL_TYPES)) {
            AllTypesCategoryLines_ = CategoryToLines_.at(CATEGORY_ALL_TYPES);
            CategoryToLines_.erase(CATEGORY_ALL_TYPES);
        }
    }

    void FillResult() {
        const auto allTypeCategoryVariations = ComputeVariations(AllTypesCategoryLines_);

        for (const auto& [category, lines] : CategoryToLines_) {
            const auto theseLinesVaritaions = ComputeVariations(lines);
            AddVariationsToCategory(category, theseLinesVaritaions);
            AddVariationsToCategory(category, allTypeCategoryVariations);
        }

        AddVariationsToCategory(TCategorizedVariations::CATEGORY_UNKNOWN, allTypeCategoryVariations);
    }

    void AddVariationsToCategory(const TStringBuf category, const TVariations& variations) {
        auto& thisCategoryVariations = Result_.CategoryToVariations_[category];
        for (const auto& [key, variationsForKey] : variations) {
            thisCategoryVariations[key] = variationsForKey;
        }
    }

    static TVariations ComputeVariations(const TVector<TString>& lines) {
        TVariations result;
        for (const auto& line : lines) {
            if (line.empty()) {
                continue;
            }

            // We don't add separate entries for variations starting with '!'. It
            // allows to specify non-symmetric synonym relations.
            auto variations = StringSplitter(line).SplitByString(", ").ToList<TString>();
            TVector<bool> skipFlags(variations.size(), false);
            for (size_t i = 0; i < variations.size(); ++i) {
                if (variations[i].StartsWith("!")) {
                    variations[i] = variations[i].substr(1);
                    skipFlags[i] = true;
                }
            }

            for (size_t i = 0; i < variations.size(); ++i) {
                if (skipFlags[i]) {
                    continue;
                }

                if (result.find(variations[i]) != result.end()) {
                    ythrow yexception() << "Found collision in variations in line \"" << line << "\"";
                }

                TVector<TString> allExceptIth;
                allExceptIth.insert(allExceptIth.end(), variations.begin(), variations.begin() + i);
                allExceptIth.insert(allExceptIth.end(), variations.begin() + i + 1, variations.end());
                result[variations[i]] = allExceptIth;
            }
        }
        return result;
    }

private:
    const TStringBuf RawData_;
    THashMap<TString, TVector<TString>> CategoryToLines_;
    TVector<TString> AllTypesCategoryLines_;
    TCategorizedVariations Result_;

    static constexpr TStringBuf CATEGORY_ALL_TYPES = "all";
    static const TRegExMatch CategoryLine;
};

const TRegExMatch TCategorizedVariationsBuilder::CategoryLine = {"^\\[[A-Z_|]+\\]$"};


class TPreprocessorBuilder {
public:
    TPreprocessorBuilder(const NSc::TValue& entities, const NSc::TValue& templates, const TString& spellingVariations = "",
                         const TString& synonyms = "", const TString& subSynonyms = "")
        : Templates_(templates)
        , Entities_(entities)
        , SpellingVariations_(TCategorizedVariationsBuilder(spellingVariations).Build())
        , Synonyms_(TCategorizedVariationsBuilder(synonyms).Build())
        , SubSynonyms_(ParseSubSynonyms(subSynonyms))
    {
    }

    TPreprocessor Build() && {
        Result_ = {};

        Result_.Entities = Entities_.Clone();
        Result_.Synonyms = MakeHolder<TCategorizedVariations>(Synonyms_);
        Result_.SpellingVariations = MakeHolder<TCategorizedVariations>(SpellingVariations_);
        Result_.SubSynonyms = SubSynonyms_;
        Result_.TypesSuitableForSubSynonyms = {"device", "device_type", "room", "group"};

        ParsePrepositions();
        ParseUnits();
        BuildBOWIndex();
        ComputeAndSetSubSynonymsMaxNumTokens();

        return std::move(Result_);
    }

private:
    void ParsePrepositions() {
        for (const auto& preposition : Entities_.TrySelect("preposition/[0]/exact_forms").GetArray()) {
            Result_.Prepositions.insert(ToString(preposition.GetString()));
        }
    }

    void ParseUnits() {
        for (const auto& unit : Entities_.TrySelect("unit").GetArray()) {
            for (const auto& form : unit.TrySelect("forms").GetArray()) {
                Result_.UnitWords.insert(ToString(form.GetString()));
            }
        }
    }

    void BuildBOWIndex() {
        for (const auto& [type, entries] : Entities_.GetDict()) {
            for (const auto& entry : entries.GetArray()) {
                const auto& value = entry.TrySelect("value");

                for (const auto& form : entry.TrySelect("bow_forms").GetArray()) {
                    AddBOWTokens(ApplyTemplates(form.GetString()), type, value);
                }
            }
        }
    }

    void AddBOWTokens(const TStringBuf bowStr, const TStringBuf type, const NSc::TValue& value) {
        for (const auto& bf : ParseBowForms(bowStr)) {
            for (const auto& t : bf.Tokens) {
                Result_.TypeToBowIndexTokens[type].insert(t.Form);
            }
            Result_.BOWIndex.Add(type, bf.Tokens, value);
        }
    }

    /*
     * The templates assumed to be topologically ordered, from the least dependent
     * to the most dependent.
     */
    TString ApplyTemplates(TStringBuf bowStr) {
        TString result = ToString(bowStr);
        const auto& templatesArray = Templates_.GetArray();
        for (auto it = templatesArray.crbegin(); it != templatesArray.crend(); ++it) {
            const auto& templateId = it->TrySelect("id").GetString();
            const auto& templateBody = it->TrySelect("template").GetString();
            SubstGlobal(result, TStringBuilder() << '{' << templateId << '}', templateBody);
        }
        return result;
    }

    void ComputeAndSetSubSynonymsMaxNumTokens() {
        Result_.SubSynonymsMaxNumTokens = 0;
        for (const auto& subSynonym : SubSynonyms_) {
            const auto numTokens = StringSplitter(subSynonym).SplitByString(" ").ToList<TString>().size();
            Result_.SubSynonymsMaxNumTokens = Max(Result_.SubSynonymsMaxNumTokens, numTokens);
        }
    }

    static THashSet<TString> ParseSubSynonyms(const TString& subSynonyms) {
        auto subSynonymsVector = StringSplitter(subSynonyms).Split('\n').ToList<TString>();
        EraseIf(subSynonymsVector, [](const TString& element) { return element.empty(); });
        return THashSet<TString>(subSynonymsVector.cbegin(), subSynonymsVector.cend());
    }

private:
    const NSc::TValue Templates_;
    const NSc::TValue Entities_;
    const TCategorizedVariations SpellingVariations_;
    const TCategorizedVariations Synonyms_;
    const THashSet<TString> SubSynonyms_;

    TPreprocessor Result_;
};


class TPreprocessorLoader {
public:
    static const TPreprocessor& GetPreprocessor(const ELanguage language) {
        return Singleton<TPreprocessorLoader>()->Storage_.at(language);
    }

private:
    TPreprocessorLoader() {
        for (const auto language : SupportedLanguages_) {
            LoadPreprocessor(language);
        }
    }

    void LoadPreprocessor(ELanguage language) {
        const TString languageStr = NameByLanguage(language);
        const auto entities = LoadAndParseResource(languageStr + "_entities.json");
        const auto templates = LoadAndParseResource(languageStr + "_templates.json");
        const auto spellingVariations = LoadResource(languageStr + "_spelling_variations.txt");
        const auto synonyms = LoadResource(languageStr + "_synonyms.txt");
        const auto subSynonyms = LoadResource(languageStr + "_subsynonyms.txt");
        Storage_[language] = TPreprocessorBuilder(entities, templates, spellingVariations, synonyms, subSynonyms).Build();
    }

private:
    THashMap<ELanguage, TPreprocessor> Storage_;

    static const TVector<ELanguage> SupportedLanguages_;

    Y_DECLARE_SINGLETON_FRIEND();
};

const TVector<ELanguage> TPreprocessorLoader::SupportedLanguages_ = {LANG_RUS, LANG_ARA};

} // namespace


const TPreprocessor& GetPreprocessor(ELanguage language) {
    return TPreprocessorLoader::GetPreprocessor(language);
}


} // namespace NAlice::NIot
