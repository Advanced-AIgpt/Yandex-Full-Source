#include "vins_converter.h"

#include <alice/nlu/granet/lib/compiler/nlu_line.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/libs/tokenization/tokenizer.h>

#include <library/cpp/dbg_output/auto.h>
#include <library/cpp/dbg_output/dump.h>
#include <library/cpp/json/writer/json.h>

#include <util/folder/path.h>
#include <util/generic/adaptor.h>
#include <util/generic/algorithm.h>
#include <util/generic/utility.h>
#include <util/stream/file.h>
#include <util/stream/labeled.h>
#include <util/stream/output.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/strip.h>

namespace NGranet {

using namespace NJson;

// ~~~~ TVinsConverter ~~~~

TVinsConverter::TVinsConverter(const NVinsConfig::TVinsConfig& vinsConfig, const TString& projectName,
        const TString& intentName, const TString& resultDir)
    : VinsConfig(vinsConfig)
    , ProjectName(projectName)
    , IntentName(intentName)
    , ResultDir(resultDir)
{
}

void TVinsConverter::SetFillersDictionary(const TString& elementName, const TString& dictionaryPath) {
    TElementData& element = EnsureElement(elementName, "", false);
    LoadSimpleDictionary(dictionaryPath, &element.Dictionary);
    FillersElementName = elementName;
}

void TVinsConverter::AddTagDictionary(const TString& tagName, const TString& dictionaryPath) {
    TElementData& element = EnsureElement("TAG." + tagName, tagName, false);
    LoadSimpleDictionary(dictionaryPath, &element.Dictionary);
}

void TVinsConverter::Process() {
    ReadNluFiles();
    WriteDictionaries();
    WriteGrammar();
    Dump();
}

void TVinsConverter::ReadNluFiles() {
    const NVinsConfig::TIntentConfig& intentConfig = VinsConfig.Projects.at(ProjectName).Intents.at(IntentName);
    for (const NVinsConfig::TNluConfig& nluConfig : intentConfig.Nlus) {
        if (!nluConfig.CanUseToTrainTagger) {
            continue;
        }
        TFileInput in(nluConfig.FullPath);
        TString line;
        while (in.ReadLine(line)) {
            ReadNluLine(line);
        }
    }
}

void TVinsConverter::ReadNluLine(const TString& line) {
    TVector<TNluLinePart> parts;
    TVector<TTag> tags;
    if (!TryParseNluTemplateLine(line, &parts, &tags)) {
        Cerr << "Error! Can't parse line " << Cite(line) << Endl;
        return;
    }
    if (parts.empty()) {
        return;
    }
    tags = AddPadding(tags, parts.size());
    Y_ENSURE(!tags.empty());
    Y_ENSURE(tags.front().Interval.Begin == 0);
    Y_ENSURE(tags.back().Interval.End == parts.size());

    TVector<TString> ruleParts;

    for (const TTag& tag : tags) {

        TVector<TString> tagParts;
        for (size_t pos = tag.Interval.Begin; pos < tag.Interval.End; ++pos) {
            const TNluLinePart& part = parts[pos];
            if (!part.StaticText.empty()) {
                Extend(NNlu::TSmartTokenizer(part.StaticText, LANG_RUS).GetNormalizedTokens(), &tagParts);
            } else if(!part.ElementName.empty()) {
                const TElementData& element = EnsureCustomTemplateElement(part.ElementName);
                tagParts.push_back("$" + element.ElementName);
            } else {
                Y_ENSURE(false, "Error in line: " << line << Endl);
            }
        }

        if (tag.Name.empty()) {
            Extend(tagParts, &ruleParts);
            continue;
        }

        const TString tagName = GetNormalizedTagName(tag, tags);
        TElementData& element = EnsureElement("TAG." + tagName, tagName, false);
        ruleParts.push_back("$" + element.ElementName);

        element.Dictionary[JoinSeq(" ", tagParts)]++;
    }

    TElementData& root = EnsureElement("root", "", true);
    root.Dictionary[JoinSeq("  ", ruleParts)]++;
}

// static
TString TVinsConverter::GetNormalizedTagName(const TTag& tag, const TVector<TTag>& tags) {
    TStringBuf withoutPlus = tag.Name;
    withoutPlus = StripString(withoutPlus);
    withoutPlus.SkipPrefix("+");
    if (withoutPlus.empty()) {
        return "";
    }
    const TString withPlus = "+" + TString(withoutPlus);
    bool isLongest = true;
    for (const TTag& other : tags) {
        if ((other.Name == withPlus || other.Name == withoutPlus)
            && other.Interval.Length() >= tag.Interval.Length()
            && other != tag)
        {
            isLongest = false;
            break;
        }
    }
    return TStringBuilder() << withoutPlus << (isLongest? "" : ".partial");
}

TVinsConverter::TElementData& TVinsConverter::EnsureCustomTemplateElement(const TString& name) {
    TElementData& element = EnsureElement("CUSTOM." + name, "", false);
    if (element.IsCustomTemplateLoaded) {
        return element;
    }
    const TString& path = VinsConfig.CustomTemplatesPaths.at(name);
    LoadSimpleDictionary(path, &element.Dictionary);
    element.IsCustomTemplateLoaded = true;
    return element;
}

TVinsConverter::TElementData& TVinsConverter::EnsureElement(const TString& elementName,
    const TString& tagName, bool isRoot)
{
    if (Elements.contains(elementName)) {
        return Elements.at(elementName);
    }
    TElementData& element = Elements[elementName];
    element.ElementName = elementName;
    element.DictionaryRelativePath += elementName + ".dict.txt";
    element.TagName = tagName;
    element.IsRoot = isRoot;
    return element;
}

void TVinsConverter::LoadSimpleDictionary(const TString& path, TMap<TString, double>* dictionary) const {
    Y_ENSURE(dictionary);
    TFileInput file(path);
    TString fullLine;
    while (file.ReadLine(fullLine)) {
        TStringBuf line = fullLine;
        line = JoinSeq(" ", NNlu::TSmartTokenizer(line.Before('#'), LANG_RUS).GetNormalizedTokens());
        if (line.find_first_of(TStringBuf("$@:'")) != TString::npos) {
            continue;
        }
        if (line.empty()) {
            continue;
        }
        (*dictionary)[TString(line)]++;
    }
}

void TVinsConverter::WriteDictionaries() {
    for (auto& item : Elements) {
        WriteDictionary(item.second);
    }
}

void TVinsConverter::WriteDictionary(const TElementData& element) const {
    const TFsPath path = ResultDir / element.DictionaryRelativePath;
    path.Parent().MkDirs();
    TFileOutput out(path);

    if (element.ElementName.StartsWith("TAG.search_text")) {
        double weightSum = 0;
        for (auto& item : element.Dictionary) {
            weightSum += item.second;
        }
        out << ".+              : " << weightSum * 1000 << "\n";
        out << "песню .+        : " << weightSum * 10 << "\n";
        out << "песня .+        : " << weightSum * 10 << "\n";
        out << "песни .+        : " << weightSum * 10 << "\n";
        out << "песенка .+      : " << weightSum * 10 << "\n";
        out << "песенки .+      : " << weightSum * 10 << "\n";
        out << "песенку .+      : " << weightSum * 10 << "\n";
        out << ".+ песни        : " << weightSum * 10 << "\n";
        out << "песню про .+    : " << weightSum << "\n";
        out << "музыку про .+   : " << weightSum << "\n";
        out << "музыку из .+    : " << weightSum << "\n";
    }

    // Sort dictionary items by weight.
    auto items = ToVector<std::pair<TString, double>>(element.Dictionary);
    SortBy(items, [](const auto& item) {return std::make_pair(-item.second, item.first);});

    for (const auto& item : items) {
        out << item.first << " : " << item.second << "\n";
    }
}

void TVinsConverter::WriteGrammar() const {
    NJsonWriter::TBuf json;
    json.SetIndentSpaces(2);
    json.BeginObject();

    // Form
    json.WriteKey("form").BeginObject();
    json.WriteKey("root").WriteString("root");
    json.WriteKey("slots").BeginList();
    for (const auto& item : Elements) {
        const TElementData& element = item.second;
        if (element.TagName.empty()) {
            continue;
        }
        json.BeginObject();
        json.SetIndentSpaces(0);
        json.WriteKey("name").WriteString(element.TagName);
        json.WriteKey("sources").BeginObject();
        json.WriteKey("element").WriteString(element.ElementName);
        json.EndObject(); // sources
        json.EndObject(); // slot
        json.SetIndentSpaces(2);
    }
    json.EndList(); // slots
    json.EndObject(); // form

    // Elements
    json.WriteKey("elements").BeginList();

    for (const auto& item : Elements) {
        const TElementData& element = item.second;
        json.BeginObject();
        json.SetIndentSpaces(0);
        json.WriteKey("element").WriteString(element.ElementName);
        json.WriteKey("rules_file").WriteString(element.DictionaryRelativePath);
        if (element.IsRoot && !FillersElementName.empty()) {
            json.WriteKey("filler").WriteString(FillersElementName);
        }
        json.EndObject();
        json.SetIndentSpaces(2);
    }
    json.EndList(); // elements
    json.EndObject(); // root

    TFileOutput file(ResultDir / "grammar.json");
    file << json.Str();
}

void TVinsConverter::Dump() const {
    Cout << Endl;
    for (const auto& item : Elements) {
        const TElementData& element = item.second;
        Cout << element.ElementName << ": " << element.Dictionary.size() << Endl;
    }
    Cout << Endl;
}

} // namespace NGranet
