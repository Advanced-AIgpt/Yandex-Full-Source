#include <alice/library/json/json.h>
#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/occurrence_searcher/occurrence_searcher.h>
#include <alice/nlu/libs/rnn_tagger/rnn_tagger.h>

#include <search/begemot/rules/occurrences/custom_entities/rule/proto/custom_entities.pb.h>

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/json/json_reader.h>

#include <util/folder/path.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>
#include <util/string/join.h>
#include <util/string/split.h>

namespace {

using TCustomEntitiesSearcher = NAlice::NNlu::TOccurrenceSearcher<NAlice::NNlu::TCustomEntityValues>;
using TIntentToSamples = THashMap<TString, TVector<NVins::TSampleFeatures>>;

constexpr TStringBuf TAGGERS_DIR = "personal_assistant_model_directory/tagger/tagger.data";
static const TVector<TString> SPARSE_SEQ_FEATURES = {"postag", "ner", "wizard"};

struct TApplierOptions {
    TString DataPath;
    TString OutputPath;
    TString TaggersDir;
    TString EmbeddingsDir;
    TString CustomEntitiesPath;
};

TApplierOptions ReadOptions(int argc, const char** argv) {
    TApplierOptions result;

    auto parser = NLastGetopt::TOpts();

    parser.AddLongOption("data-path")
          .StoreResult(&result.DataPath)
          .Required()
          .Help("Path to the file with canonized features");

    parser.AddLongOption("output-path")
          .StoreResult(&result.OutputPath)
          .Required()
          .Help("Predictions path");

    parser.AddLongOption("taggers-dir")
          .StoreResult(&result.TaggersDir)
          .Required()
          .Help("Path to the dir with the tagger models");

    parser.AddLongOption("embeddings-dir")
          .StoreResult(&result.EmbeddingsDir)
          .Required()
          .Help("Path to the dir with the embeddings");

    parser.AddLongOption("custom-entities-path")
          .StoreResult(&result.CustomEntitiesPath)
          .Required()
          .Help("Path to the custom entities");

    parser.AddHelpOption();
    parser.SetFreeArgsNum(0);

    NLastGetopt::TOptsParseResult parserResult{&parser, argc, argv};

    return result;
}

NAlice::TTokenEmbedder GetTokenEmbedder(const TFsPath& path) {
    return NAlice::TTokenEmbedder(TBlob::PrechargedFromFile(path / "embeddings"),
                                  TBlob::PrechargedFromFile(path / "embeddings_dictionary.trie"));
}

void ReadStringVector(const NJson::TJsonValue& stringsJson, TVector<TString>& strings) {
    for (const auto& tag : stringsJson.GetArraySafe()) {
        strings.push_back(tag.GetStringSafe());
    }
}

void ReadSparseSeqFeatures(const NJson::TJsonValue& sparseSeqFeaturesJson,
                           TVector<TVector<NVins::TSparseFeature>>& sparseSeqFeatures)
{
    for (const auto& tokenFeaturesJson : sparseSeqFeaturesJson.GetArraySafe()) {
        TVector<NVins::TSparseFeature> tokenFeatures;
        for (const auto& tokenFeature : tokenFeaturesJson.GetArraySafe()) {
            tokenFeatures.emplace_back(tokenFeature["value"].GetStringSafe(),
                                       tokenFeature["weight"].GetDoubleSafe(1.));
        }
        sparseSeqFeatures.push_back(std::move(tokenFeatures));
    }
}

void ParseLine(const TString& line, const TVector<TString>& header,
               TString& markup, NJson::TJsonValue& mock, TMaybe<TString>& intent)
{
    TVector<TString> fields = StringSplitter(line).Split('\t').ToList<TString>();

    Y_ENSURE(header.size() == fields.size());
    for (size_t fieldIndex = 0; fieldIndex < header.size(); ++fieldIndex) {
        if (header[fieldIndex] == "markup") {
            markup = fields[fieldIndex];
        } else if (header[fieldIndex] == "intent") {
            intent = fields[fieldIndex];
        } else if (header[fieldIndex] == "mock") {
            mock = NAlice::JsonFromString(fields[fieldIndex]);
        } else {
            Y_ENSURE(false, "Unexpected field " << header[fieldIndex]);
        }
    }
    Y_ENSURE(markup);
    Y_ENSURE(mock.IsDefined());
}

TIntentToSamples ReadSampleFeatures(const TString& path) {
    TFileInput dataStream(path);

    TIntentToSamples intentToSamples;
    TString line;

    dataStream.ReadLine(line);
    TVector<TString> header = StringSplitter(line).Split('\t').ToList<TString>();
    while (dataStream.ReadLine(line)) {
        TString markup;
        NJson::TJsonValue mock;
        TMaybe<TString> intent;
        ParseLine(line, header, markup, mock, intent);

        NVins::TSampleFeatures sample;

        ReadStringVector(mock["sample"]["tokens"], sample.Sample.Tokens);
        if (mock["sample"].Has("tags")) {
            ReadStringVector(mock["sample"]["tags"], sample.Sample.Tags);
        }

        for (const auto& feature : SPARSE_SEQ_FEATURES) {
            ReadSparseSeqFeatures(mock["sparse_seq_features"][feature], sample.SparseSeq[feature]);
        }

        intentToSamples[intent.GetOrElse("")].push_back(sample);
    }

    return intentToSamples;
}

void GetIndicesMappings(const TVector<TString>& tokens, THashMap<size_t, size_t>& beginByteToTokenIndex,
                        THashMap<size_t, size_t>& endByteToTokenIndex)
{
    size_t beginBytePos = 0;
    for (size_t tokenIndex = 0; tokenIndex < tokens.size(); ++tokenIndex) {
        const auto& tokenText = tokens[tokenIndex];

        beginByteToTokenIndex[beginBytePos] = tokenIndex;
        endByteToTokenIndex[beginBytePos + tokenText.length()] = tokenIndex + 1;
        beginBytePos += 1 + tokenText.length();
    }
}

void AddCustomEntities(const TCustomEntitiesSearcher& searcher, TIntentToSamples& intentToSamples) {
    for (auto& [intent, samples] : intentToSamples) {
        for (auto& sample : samples) {
            auto& features = sample.SparseSeq["ner"];

            THashMap<size_t, size_t> beginByteToTokenIndex;
            THashMap<size_t, size_t> endByteToTokenIndex;
            GetIndicesMappings(sample.Sample.Tokens, beginByteToTokenIndex, endByteToTokenIndex);

            const auto text = JoinSeq(" ", sample.Sample.Tokens);
            for (const auto& occurrence : searcher.Search(text)) {
                const size_t beginToken = beginByteToTokenIndex.at(occurrence.Begin);
                const size_t endToken = endByteToTokenIndex.at(occurrence.End);

                for (const auto& value : occurrence.Value.GetCustomEntityValues()) {
                    TString featureName = value.GetType();
                    featureName.to_upper();

                    features[beginToken].emplace_back("B-" + featureName, 1.);
                    for (size_t i = occurrence.Begin + 1; i < endToken; ++i) {
                        features[i].emplace_back("I-" + featureName, 1.);
                    }
                }
            }
        }
    }
}

void AddEmbeddings(const NAlice::TTokenEmbedder& embedder, TIntentToSamples& samples) {
    for (auto& [intent, intentSamples] : samples) {
        for (auto& sample : intentSamples) {
            sample.DenseSeq["alice_requests_emb"] = embedder.EmbedSequence(sample.Sample.Tokens);
        }
    }
}

THashMap<TString, NVins::TRnnTagger> LoadTaggers(const TFsPath& taggersDir) {
    THashMap<TString, NVins::TRnnTagger> intentToTagger;

    TVector<TString> taggerNames;
    (taggersDir / TAGGERS_DIR).ListNames(taggerNames);

    for (const auto& taggerName : taggerNames) {
        const TFsPath modelDir = taggersDir / TAGGERS_DIR / taggerName;

        if (!NVins::TRnnTagger::CanBeLoadedFrom(modelDir)) {
            // TODO(vl-trifonov) make it work with language specific models
            continue;
        }

        NVins::TRnnTagger tagger(modelDir);
        tagger.EstablishSession();

        intentToTagger.emplace(taggerName, tagger);
    }

    return intentToTagger;
}

void DumpTokenList(const TVector<TString>& tokens, IOutputStream& output) {
    for (const auto& token : tokens) {
        output << token << " ";
    }
}

void ProcessSamples(const NVins::TRnnTagger& tagger, const TVector<NVins::TSampleFeatures>& samples,
                    IOutputStream& output)
{
    for (const auto& sample : samples) {
        const auto prediction = tagger.PredictTop(sample, /* topSize= */ 1, /* beamWidth= */ 10)[0];

        DumpTokenList(sample.Sample.Tokens, output);
        output << "\t";
        if (sample.Sample.Tags) {
            DumpTokenList(sample.Sample.Tags, output);
            output << "\t";
        }
        DumpTokenList(prediction.Tags, output);
        output << Endl;
    }
}

} // namespace

int main(int argc, const char** argv) {
    const TApplierOptions options = ReadOptions(argc, argv);

    auto intentToSamples = ReadSampleFeatures(options.DataPath);

    const TCustomEntitiesSearcher customEntitySearcher(TBlob::PrechargedFromFile(options.CustomEntitiesPath));
    AddCustomEntities(customEntitySearcher, intentToSamples);

    const auto embedder = GetTokenEmbedder(options.EmbeddingsDir);
    AddEmbeddings(embedder, intentToSamples);

    const THashMap<TString, NVins::TRnnTagger> intentToTaggers = LoadTaggers(options.TaggersDir);
    TFileOutput outputFile(options.OutputPath);
    for (const auto& [intent, samples] : intentToSamples) {
        if (intent) {
            if (const auto* taggerPtr = intentToTaggers.FindPtr(intent)) {
                outputFile << intent << Endl;
                ProcessSamples(*taggerPtr, samples, outputFile);
            }
        } else {
            for (const auto& [intent, tagger] : intentToTaggers) {
                outputFile << intent << Endl;
                ProcessSamples(tagger, samples, outputFile);
            }
        }
    }

    return 0;
}
