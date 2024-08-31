#pragma once

#include "vins_config.h"
#include <alice/nlu/granet/lib/sample/tag.h>
#include <util/generic/map.h>
#include <util/generic/noncopyable.h>
#include <util/generic/set.h>
#include <util/folder/path.h>

namespace NGranet {

// ~~~~ TVinsConverter ~~~~

class TVinsConverter : TNonCopyable {
public:
    TVinsConverter(const NVinsConfig::TVinsConfig& vinsConfig, const TString& projectName,
        const TString& intentName, const TString& resultDir);

    void SetFillersDictionary(const TString& elementName, const TString& dictionaryPath);
    void AddTagDictionary(const TString& tagName, const TString& dictionaryPath);

    void Process();

private:
    struct TElementData {
        TString ElementName;
        TString TagName;
        TString DictionaryRelativePath;
        bool IsRoot = false;
        bool IsCustomTemplateLoaded = false;
        TMap<TString, double> Dictionary;
    };

private:
    void ReadNluFiles();
    void ReadNluLine(const TString& line);
    static TString GetNormalizedTagName(const TTag& tag, const TVector<TTag>& tags);
    TElementData& EnsureElement(const TString& elementName, const TString& tagName, bool isRoot);
    TElementData& EnsureCustomTemplateElement(const TString& name);
    void LoadSimpleDictionary(const TString& path, TMap<TString, double>* dictionary) const;
    void WriteDictionaries();
    void WriteDictionary(const TElementData& element) const;
    void WriteGrammar() const;
    void Dump() const;

private:
    const NVinsConfig::TVinsConfig& VinsConfig;
    const TString ProjectName;
    const TString IntentName;
    const TFsPath ResultDir;
    TString FillersElementName;

    TMap<TString, TElementData> Elements;
};

} // namespace NGranet
