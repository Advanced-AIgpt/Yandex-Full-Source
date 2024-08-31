#include <alice/nlu/libs/item_selector/catboost_item_selector/catboost_item_selector.h>
#include <alice/nlu/libs/item_selector/catboost_item_selector/loader.h>
#include <alice/nlu/libs/item_selector/catboost_item_selector/utils.h>

#include <util/folder/dirut.h>
#include <util/folder/path.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/string/builder.h>
#include <util/string/subst.h>
#include <util/thread/pool.h>

#include <library/cpp/getopt/last_getopt.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <library/cpp/resource/resource.h>


using namespace NAlice;
using namespace NAlice::NItemSelector;


const TString defaultTaggerPath = JoinFsPaths(
    GetHomeDir(), "arcadia/alice/nlu/data/ru/models/alice_item_selector/video_gallery/tagger/tagger.data/personal_assistant.scenarios.video_play"
);

const TString defaultSpecPath = JoinFsPaths(
    GetHomeDir(), "arcadia/alice/nlu/data/ru/models/alice_item_selector/video_gallery/model/spec.json"
);

const TString defaultEmbeddingsPath = JoinFsPaths(GetHomeDir(), "arcadia/alice/nlu/data/ru/models/alice_token_embedder");


void WriteColumnDescription(TFileOutput& out) {
    out << 0 << "\t" << "Label" << Endl;
    out << 1 << "\t" << "GroupId" << Endl;
    out << 2 << "\t" << "Auxiliary" << Endl;
    out << 3 << "\t" << "Auxiliary" << Endl;
    out << 4 << "\t" << "Auxiliary" << Endl;
}


void WriteHeader(TFileOutput& out, const TCatboostItemSelectorSpec& spec) {
    TVector<TString> featureNames(spec.FeatureSpec.size());
    for (const auto& [key, value] : spec.FeatureSpec) {
        featureNames[value] = key;
    }

    out << "Label" << "\t" << "GroupId" << "\t" << "RequestId"<< "\t" << "ItemName" << "\t" << "UtteranceText";
    for (const auto& name : featureNames) {
        out << "\t" << name;
    }
    out << Endl;
}


int main(int argc, const char** argv) {
    NLastGetopt::TOpts opts;
    TString taggerPath, embeddingsPath, dictionaryPath, specPath, inputPath, poolPath, descPath;

    opts.AddLongOption("tagger_path").Help("path to tagger directory").DefaultValue(defaultTaggerPath).StoreResult(&taggerPath);

    opts.AddLongOption("embeddings_path").Help("path to embeddings").DefaultValue(
            defaultEmbeddingsPath + "/embeddings"
        ).StoreResult(&embeddingsPath);

    opts.AddLongOption("dictionary_path").Help("path to dictionary.trie").DefaultValue(
            defaultEmbeddingsPath + "/embeddings_dictionary.trie"
        ).StoreResult(&dictionaryPath);

    opts.AddLongOption("spec_path").Help("path to desired catboost model spec").DefaultValue(
            defaultSpecPath
        ).StoreResult(&specPath);

    opts.AddLongOption("in").Help("input file").DefaultValue(
            "data_with_select_video.json"
        ).StoreResult(&inputPath);

    opts.AddLongOption("pool").Help("output pool file").DefaultValue(
            "data_with_select_video.tsv"
        ).StoreResult(&poolPath);

    opts.AddLongOption("columns_description").Help("output columns description file").DefaultValue(
            "columns_description.tsv"
        ).StoreResult(&descPath);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    const TEasyTagger tagger = GetEasyTagger(taggerPath, embeddingsPath, dictionaryPath, 10);

    TCatboostItemSelectorSpec spec = ReadSpec(specPath);

    TFileInput in(inputPath);
    auto data = NJson::ReadJsonTree(&in, true);

    TFileOutput desc_out(descPath);
    WriteColumnDescription(desc_out);

    TMultiMap<TString, TString> groups;
    THolder<IThreadPool> queue = CreateThreadPool(16);

    for (const auto row : data.GetArray()) {
        queue->SafeAddFunc([&, row](){
            TCatboostSelectionRequest request;
            request.Phrase = {.Text = row["utterance_text"].GetStringSafe()};
            request.TaggerResult = tagger.Tag(request.Phrase.Text);

            TCatboostSelectionItem item;
            item.Synonyms = {{row["name"].GetStringSafe()}};
            item.TaggerResults = {tagger.Tag(item.Synonyms[0].Text)};

            item.Meta = TVideoItemMeta {
                .Duration = static_cast<uint32_t>(row["duration"].GetUInteger()),
                .Episode = static_cast<uint32_t>(row["episode"].GetUInteger()),
                .EpisodesCount = static_cast<uint32_t>(row["episodes_count"].GetUInteger()),
                .Genre = row["genre"].GetString(),
                .MinAge = static_cast<uint32_t>(row["min_age"].GetUInteger()),
                .ProviderName = row["provider_name"].GetString(),
                .Rating = row["release_year"].GetDouble(),
                .ReleaseYear = static_cast<uint32_t>(row["release_year"].GetUInteger()),
                .Season = static_cast<uint32_t>(row["season"].GetUInteger()),
                .SeasonsCount = static_cast<uint32_t>(row["seasons_count"].GetUInteger()),
                .Type = row["type"].GetString(),
                .ViewCount = row["view_count"].GetUInteger(),
                .Position = static_cast<uint32_t>(row["position"].GetUIntegerSafe())
            };

            item.PositionTaggerResult = tagger.Tag(GetPositionalText(row["position"].GetIntegerSafe()));

            auto features = ComputeFeatures(request, item, spec);

            TString featuresString = ToString(row["score"].GetDoubleSafe()) + "\t" + row["task_id"].GetStringSafe() +
                "\t" + row["request_id"].GetStringSafe() + "\t" + SubstGlobalCopy(item.Synonyms[0].Text, "\t", " ") +
                "\t" + request.Phrase.Text;

            for (auto& feature : features) {
                featuresString += "\t" + ToString(feature);
            }
            featuresString += "\n";

            static TMutex lock;
            with_lock(lock) {
                groups.insert({row["task_id"].GetStringSafe(), featuresString});
            }
        });
    }
    queue->Stop();

    TFileOutput out(poolPath);

    WriteHeader(out, spec);

    for (auto& [key, value] : groups) {
        out << value;
    }
    return 0;
}
