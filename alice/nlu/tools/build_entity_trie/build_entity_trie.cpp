#include <alice/nlu/libs/entity_parsing/entity_parsing.h>
#include <alice/nlu/libs/entity_searcher/entity_searcher_builder.h>

#include <alice/library/json/json.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/json_value.h>

#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/split.h>

using namespace NAlice::NNlu;

namespace {

const TString OCCURRENCE_SEARCHER_TYPE = "TOccurrenceSearcher";
const TString ENTITY_SEARCHER_TYPE = "TEntitySearcher";

NJson::TJsonValue ReadJson(const TFsPath& path) {
    return NAlice::JsonFromString(TFileInput(path).ReadAll());
}

void ReadEntitiesFromFile(const TFsPath& pathToConfig, const TFsPath& pathToVinsFile,
                          TVector<TEntityConfig>* entities) {
    Y_ASSERT(entities);
    NJson::TJsonValue entitiesJson = ReadJson(pathToConfig / pathToVinsFile)["entities"];
    entitiesJson.SetType(NJson::JSON_ARRAY);
    for (NJson::TJsonValue& entity : entitiesJson.GetArraySafe()) {
        entity.InsertValue("values", ReadJson(pathToConfig / entity["path"].GetString()));
    }
    ReadEntitiesFromJson(entitiesJson, entities);
}

TVector<TEntityConfig> ReadEntitiesFromFiles(const TFsPath& pathToConfig, const TVector<TFsPath>& pathsToVinsFiles) {
    TVector<TEntityConfig> entities;
    for (const TFsPath& pathToVinsFile : pathsToVinsFiles) {
        ReadEntitiesFromFile(pathToConfig, pathToVinsFile, &entities);
    }
    return entities;
}

void WriteToFile(const TEntitySearcherData& builderResult, const TFsPath& idToStringPath,
                 const TFsPath& entitySearcherDataPath, const TFsPath& entityStringsPath) {
    TFileOutput outMap(idToStringPath);
    for (const TString& s : builderResult.IdToString) {
        outMap << s << "\n";
    }
    TFileOutput outTrie(entitySearcherDataPath);
    outTrie.Write(builderResult.TrieData.Data(), builderResult.TrieData.Length());
    TFileOutput outEntityStrings(entityStringsPath);
    for (const TEntityString& s : builderResult.EntityStrings) {
        outEntityStrings << s.Type << '\t' << s.Value << '\t' << s.LogProbability << '\n';
    }
}

struct TOptions {
    TFsPath PathToConfig;
    TVector<TFsPath> PathsToVinsFiles;
    TString CommaSeparatedEntityTypes;
    TString TrieType;
    TFsPath EntitySearcherDataPath;
    TFsPath IdToStringPath;
    TFsPath EntityStringsPath;
    bool Uniq;
};

TOptions Parse(int argc, const char** argv) {
    TOptions res;
    NLastGetopt::TOpts opts;
    opts.AddLongOption('c', "path-to-config", "Paths to config")
        .Required()
        .StoreResult(&res.PathToConfig);
    opts.AddLongOption('f', "paths-to-vins-files", "Paths to Vins files")
        .Required()
        .AppendTo(&res.PathsToVinsFiles);
    opts.AddLongOption('e', "entity-types",
                       "Use specified (comma-delimited) entity types only. Use all the types when not set.")
        .StoreResult(&res.CommaSeparatedEntityTypes);
    opts.AddLongOption('u', "uniq", "Use only unique entity tuples: (entity_type, entity_value, synonym)")
        .NoArgument()
        .DefaultValue(false)
        .StoreTrue(&res.Uniq);
    opts.AddLongOption('t', "trie-type",
                        "The type of trie to be built. "
                        "TOccurrenceSearcher and TEntitySearcher are supported now. "
                        "Data file for TOccurrenceSearcher basically stores (<STRING>, <VALUE>) pairs "
                        "where Value is a list of (<ENTITY_TYPE>, <ENTITY_VALUE>) pairs. "
                        "For TEntitySearcher there are two file: id-to-string file and data file. "
                        "Data file for TEntitySearcher stores (<UINT_32>, <VALUE>) pairs "
                        "where Value is a list of (<ENTITY_TYPE>, <ENTITY_VALUE>) pairs. "
                        "<UINT32_t> is converted to a string with id-to-string file")
        .DefaultValue("TOccurrenceSearcher")
        .StoreResult(&res.TrieType);
    opts.AddLongOption('n', "id-to-string-path", "Path to output id-to-string file")
        .DefaultValue("strings.txt")
        .StoreResult(&res.IdToStringPath);
    opts.AddLongOption('s', "entity-searcher-data-path", "Path to output data file for entity searcher")
        .DefaultValue("custom_entities.trie")
        .StoreResult(&res.EntitySearcherDataPath);
    opts.AddLongOption('p', "entity-strings-path", "Path to file with entityStrings, needed for entitySearcher")
        .DefaultValue("entity_strings.txt")
        .StoreResult(&res.EntityStringsPath);
    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);
    return res;
}

} // namespace

int main(int argc, const char** argv) {
    const TOptions opts = Parse(argc, argv);
    TMaybe<THashSet<TString>> entityTypes;
    if (!opts.CommaSeparatedEntityTypes.empty()) {
        entityTypes = StringSplitter(opts.CommaSeparatedEntityTypes).Split(',').SkipEmpty();
    }
    TVector<TEntityConfig> entities = ReadEntitiesFromFiles(opts.PathToConfig, opts.PathsToVinsFiles);
    TVector<TEntityString> entityStrings = MakeEntityStrings(entityTypes, entities);
    if (opts.Uniq) {
        SortUnique(entityStrings);
    }
    if (opts.TrieType == OCCURRENCE_SEARCHER_TYPE) {
        const TBlob builderResult = BuildOccurrenceSearcherData(entityStrings);
        Cout.Write(builderResult.Data(), builderResult.Length());
    } else if (opts.TrieType == ENTITY_SEARCHER_TYPE) {
        WriteToFile(TEntitySearcherDataBuilder().Build(entityStrings), opts.IdToStringPath,
                    opts.EntitySearcherDataPath, opts.EntityStringsPath);
    }
}
