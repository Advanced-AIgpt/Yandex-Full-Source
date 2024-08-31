#include <alice/nlu/libs/ellipsis_rewriter/ellipsis_rewriter.h>
#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/embedder/embedding_loading.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/string/join.h>
#include <util/string/split.h>


namespace {

constexpr TStringBuf MODEL_DIRECTORY = "search/wizard/data/wizard/AliceEllipsisRewriter";

constexpr TStringBuf EMBEDDINGS_PATH = "embeddings";
constexpr TStringBuf DICTIONARY_PATH = "embeddings_dictionary.trie";

constexpr TStringBuf MODEL_PROTOBUF_FILE = "model.pb";
constexpr TStringBuf MODEL_DESCRIPTION_FILE = "model_description";
constexpr TStringBuf TRAINABLE_EMBEDDINGS_FILE = "trainable_embeddings.json";
constexpr TStringBuf CONFIG_FILE = "config.json";

NAlice::TTokenEmbedder GetTokenEmbedder() {
    return NAlice::TTokenEmbedder(TBlob::FromString(NResource::Find(EMBEDDINGS_PATH)),
                                  TBlob::FromString(NResource::Find(DICTIONARY_PATH)));
}

NAlice::TEllipsisRewriter LoadModel() {
    const TFsPath modelDirectory = BinaryPath(MODEL_DIRECTORY);

    TFileInput graphStream(modelDirectory / MODEL_PROTOBUF_FILE);
    TFileInput descriptionStream(modelDirectory / MODEL_DESCRIPTION_FILE);

    TFileInput configStream(modelDirectory / CONFIG_FILE);
    const NAlice::TEllipsisRewriterConfig config(&configStream);

    TFileInput trainableEmbeddingsStream(modelDirectory / TRAINABLE_EMBEDDINGS_FILE);
    const auto jsonEmbeddings = NAlice::LoadEmbeddingsFromJson(&trainableEmbeddingsStream);

    const auto embedder = GetTokenEmbedder();

    return NAlice::TEllipsisRewriter(jsonEmbeddings.at("[SEP]"), embedder, config, &graphStream, &descriptionStream);
}

TVector<NVins::TSample> ConvertDialogHistoryToSamples(const TVector<TString>& dialogHistory) {
    TVector<NVins::TSample> dialogHistorySamples;

    for (const auto& phrase : dialogHistory) {
        NVins::TSample sample;
        sample.Tokens = StringSplitter(phrase).Split(' ').SkipEmpty();

        dialogHistorySamples.push_back(sample);
    }

    return dialogHistorySamples;
}

TString GenerateRewriting(const NAlice::TEllipsisRewriter& rewriter, const TVector<TString>& dialogHistory) {
    const auto dialogHistorySamples = ConvertDialogHistoryToSamples(dialogHistory);

    const auto predictions = rewriter.Predict(dialogHistorySamples);
    TVector<TString> rewritingTokens;
    for (const auto& token : predictions) {
        if (token.IsRewrittenTextPart) {
            rewritingTokens.push_back(token.Text);
        }
    }

    return JoinSeq(" ", rewritingTokens);
}

} // anonymous namespace

Y_UNIT_TEST_SUITE(TEllipsisRewriterTestSuite) {
    Y_UNIT_TEST(ApplyTest) {
        const auto rewriter = LoadModel();

        {
            const auto rewriting = GenerateRewriting(rewriter, {"алиса скажи пожалуйста какая сегодня погода"});
            const TString expected = "какая сегодня погода";
            UNIT_ASSERT_VALUES_EQUAL(rewriting, expected);
        }
        {
            const auto rewriting = GenerateRewriting(rewriter, {"какая сегодня погода", "вот смотрите", "а завтра"});
            const TString expected = "какая погода завтра";
            UNIT_ASSERT_VALUES_EQUAL(rewriting, expected);
        }
        {
            const auto rewriting = GenerateRewriting(rewriter, {"назови случайное число от 1 до 10", "4", "а до 40"});
            const TString expected = "назови случайное число от 1 до 40";
            UNIT_ASSERT_VALUES_EQUAL(rewriting, expected);
        }
        {
            const auto rewriting = GenerateRewriting(rewriter,
                {"закажи помидоры пожалуйста", "где заказать", "в перекрестке"});
            const TString expected = "закажи помидоры в перекрестке";
            UNIT_ASSERT_VALUES_EQUAL(rewriting, expected);
        }
    }
}
