#include "catboost_item_selector.h"
#include "loader.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/stream/file.h>

using namespace NAlice::NItemSelector;

namespace {

const TString MODEL_PATH = "model/model.cbm";
const TString MODEL_SPEC_PATH = "model/spec.json";

constexpr TStringBuf EMBEDDINGS_PATH = "embeddings";
constexpr TStringBuf DICTIONARY_PATH = "embeddings_dictionary.trie";

constexpr TStringBuf MODEL_PROTOBUF_FILE = "model.pb";
constexpr TStringBuf MODEL_DESCRIPTION_FILE = "model_description";

NVins::TRnnTagger LoadRnnTagger() {
    TStringStream graphStream(NResource::Find(MODEL_PROTOBUF_FILE));
    TStringStream descriptionStream(NResource::Find(MODEL_DESCRIPTION_FILE));

    return NVins::TRnnTagger(&graphStream, &descriptionStream);
}

NAlice::TTokenEmbedder LoadTokenEmbedder() {
    return NAlice::TTokenEmbedder(TBlob::FromString(NResource::Find(EMBEDDINGS_PATH)),
                                  TBlob::FromString(NResource::Find(DICTIONARY_PATH)));
}

TEasyTagger GetEasyTagger() {
    return {LoadRnnTagger(), LoadTokenEmbedder(), /* topSize = */ 1};
}

}; // anonymous namespace

Y_UNIT_TEST_SUITE(TCatboostItemSelectorTestSuite) {
    Y_UNIT_TEST(CheckFeatures) {
        TCatboostSelectionRequest request;
        request.Phrase = {"включи про пингвинов", ELanguage::LANG_RUS};
        request.TaggerResult = {{
            /* Tokens */ {{"включи", "O"}, {"про", "O"}, {"пингвинов", "O"}},
            /* Probability */ 1
        }};

        TCatboostSelectionItem item;
        item.Synonyms = {{"веселые пингвины едят слона", ELanguage::LANG_RUS}};
        item.TaggerResults = {{{
            /* Tokens */ {{"веселые", "O"}, {"пингвины", "O"}, {"едят", "O"}, {"слона", "O"}},
            /* Probability */ 1
        }}};
        item.PositionTaggerResult = {{
            /* Tokens */ {{"1", "O"}},
            /* Probability */ 1
        }};
        item.Meta = TVideoItemMeta{
            .Duration = 10,
            .Genre = "food_show",
            .MinAge = 18,
            .ProviderName = "",
            .Type = "video",
            .ViewCount = 9001,
            .Position = 1
        };

        const TFeatureMap expected {
            {"item_length", 4},
            {"lemma_jaccard_O", 0.166667},
            {"position_edit12_O", 18},
            {"lemma_edit12_O", 9},
            {"no_lemma_jaccard_O", 0},
            {"no_lemma_edit12_O", 10},
            {"no_lemma_edit21_O", 16},
            {"item_non_russian_words_count", 0},
            {"position_tagging_top_probability", 1},
            {"lemma_edit21_O", 14},
            {"utterance_non_russian_words_count", 0},
            {"utterance_length", 3},
            {"item_tagging_top_probability", 1},
            {"position_edit21_O", 1},
            {"position_jaccard_O", 0},
            {"utterance_tagging_top_probability", 1},
            {"duration", 10},
            {"min_age", 18},
            {"view_count", 9001},
            {"position", 1}
        };
        auto featuresMap = ComputeFeatureMap(request, item, /* removeBioPrefix = */ true);

        UNIT_ASSERT_VALUES_EQUAL(expected.size(), featuresMap.size());
        for (const auto& [key, value] : expected) {
            UNIT_ASSERT_DOUBLES_EQUAL_C(value, featuresMap[key], 1e-3, key);
        }
    }

    Y_UNIT_TEST(TagAndComputeFeatures) {
        const TEasyTagger tagger = GetEasyTagger();
        TCatboostSelectionRequest request;
        request.Phrase = {"алиса включи третье пожалуйста", ELanguage::LANG_RUS};
        request.TaggerResult = tagger.Tag(request.Phrase.Text);

        TCatboostSelectionItem item;
        item.Synonyms = {{"Веселые пингвины возвращаются! |@<|@| балет 2006 DvDrIp", ELanguage::LANG_RUS}};
        item.TaggerResults = {tagger.Tag(item.Synonyms[0].Text, item.Synonyms[0].Language)};
        item.PositionTaggerResult = tagger.Tag("включи 179");
        item.Meta = TVideoItemMeta {
            .Duration = 12345,
            .Genre = "ballet",
            .MinAge = 90,
            .ProviderName = "",
            .ReleaseYear = 2006,
            .Type = "video",
            .ViewCount = 100500,
            .Position = 179
        };

        const TFeatureMap expected {
            {"item_length", 6},
            {"position_edit12_nonsense", 14.2724},
            {"position_edit12_sense", 1.94282},
            {"lemma_edit12_nonsense", 14.2724},
            {"lemma_edit21_sense", 3.3455},
            {"position_jaccard_nonsense", 0},
            {"no_lemma_edit12_sense", 6.57488},
            {"no_lemma_jaccard_nonsense", 0},
            {"lemma_edit21_nonsense", 0},
            {"item_non_russian_words_count", 1},
            {"position_tagging_top_probability", 0.869767},
            {"no_lemma_edit21_nonsense", 0},
            {"lemma_edit12_sense", 8.3144},
            {"utterance_non_russian_words_count", 0},
            {"lemma_jaccard_sense", 0},
            {"utterance_length", 4},
            {"position_jaccard_sense", 0.293995},
            {"no_lemma_edit12_nonsense", 14.2724},
            {"no_lemma_jaccard_sense", 0},
            {"item_tagging_top_probability", 0.0899473},
            {"position_edit21_sense", 2.94681},
            {"lemma_jaccard_nonsense", 0},
            {"no_lemma_edit21_sense", 3.43545},
            {"utterance_tagging_top_probability", 0.951495},
            {"position_edit21_nonsense", 0},
            {"duration", 12345},
            {"min_age", 90},
            {"release_year", 2006},
            {"view_count", 100500},
            {"position", 179}
        };
        auto featuresMap = ComputeFeatureMap(request, item, /* removeBioPrefix = */ false);

        UNIT_ASSERT_VALUES_EQUAL(expected.size(), featuresMap.size());
        for (const auto& [key, value] : expected) {
            UNIT_ASSERT_DOUBLES_EQUAL_C(value, featuresMap[key], 1e-3, key);
        }
    }

    Y_UNIT_TEST(TagAndComputeFeaturesEmptyName) {
        const TEasyTagger tagger = GetEasyTagger();
        TCatboostSelectionRequest request;
        request.Phrase = {"алиса включи третье пожалуйста", ELanguage::LANG_RUS};
        request.TaggerResult = tagger.Tag(request.Phrase.Text);

        TCatboostSelectionItem item;
        item.Synonyms = {{"", ELanguage::LANG_RUS}};
        item.TaggerResults = {tagger.Tag(item.Synonyms[0].Text, item.Synonyms[0].Language)};
        item.PositionTaggerResult = tagger.Tag("включи 3");
        item.Meta = TVideoItemMeta{
            .Position = 3
        };

        const TFeatureMap expected {
            {"item_length", 0},
            {"position_edit12_nonsense", 14.2724},
            {"position_edit12_sense", 1.115243435},
            {"lemma_edit12_nonsense", 14.2724},
            {"lemma_edit21_sense", 0},
            {"position_jaccard_nonsense", 0},
            {"no_lemma_edit12_sense", 6.660462856},
            {"no_lemma_jaccard_nonsense", 0},
            {"lemma_edit21_nonsense", 0},
            {"item_non_russian_words_count", 0},
            {"position_tagging_top_probability", 0.869767},
            {"no_lemma_edit21_nonsense", 0},
            {"lemma_edit12_sense", 8.563451767},
            {"utterance_non_russian_words_count", 0},
            {"lemma_jaccard_sense", 0},
            {"utterance_length", 4},
            {"position_jaccard_sense", 0.8328397274},
            {"no_lemma_edit12_nonsense", 14.2724},
            {"no_lemma_jaccard_sense", 0},
            {"item_tagging_top_probability", 1},
            {"position_edit21_sense", 0.3796948493},
            {"lemma_jaccard_nonsense", 0},
            {"no_lemma_edit21_sense", 0},
            {"utterance_tagging_top_probability", 0.951495},
            {"position_edit21_nonsense", 0},
            {"position", 3}
        };
        auto featuresMap = ComputeFeatureMap(request, item, /* removeBioPrefix = */ false);

        UNIT_ASSERT_VALUES_EQUAL(expected.size(), featuresMap.size());
        for (const auto& [key, value] : expected) {
            UNIT_ASSERT_DOUBLES_EQUAL_C(value, featuresMap[key], 1e-3, key);
        }
    }
}
