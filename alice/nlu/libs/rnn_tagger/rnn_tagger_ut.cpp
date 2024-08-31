#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/rnn_tagger/rnn_tagger.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>
#include <util/string/split.h>

namespace {

constexpr TStringBuf EMBEDDINGS_PATH = "embeddings";
constexpr TStringBuf DICTIONARY_PATH = "embeddings_dictionary.trie";

constexpr TStringBuf MODEL_PROTOBUF_FILE = "model.pb";
constexpr TStringBuf MODEL_DESCRIPTION_FILE = "model_description";

NAlice::TTokenEmbedder GetTokenEmbedder() {
    return NAlice::TTokenEmbedder(TBlob::FromString(NResource::Find(EMBEDDINGS_PATH)),
                                  TBlob::FromString(NResource::Find(DICTIONARY_PATH)));
}

NVins::TRnnTagger LoadModel() {
    TStringStream graphStream(NResource::Find(MODEL_PROTOBUF_FILE));
    TStringStream descriptionStream(NResource::Find(MODEL_DESCRIPTION_FILE));

    return NVins::TRnnTagger(&graphStream, &descriptionStream);
}

NVins::TSampleFeatures GetSampleFeatures(const NAlice::TTokenEmbedder& embedder, const TString& text) {
    NVins::TSampleFeatures sampleFeatures;
    sampleFeatures.Sample.Tokens = StringSplitter(text).Split(' ').SkipEmpty();
    sampleFeatures.Sample.Tags = TVector<TString>(sampleFeatures.Sample.Tokens.size(), "O");

    sampleFeatures.DenseSeq["alice_requests_emb"] = embedder.EmbedSequence(sampleFeatures.Sample.Tokens);

    return sampleFeatures;
}

constexpr size_t TOP_SIZE = 2;
constexpr size_t BEAM_WIDTH = 10;

const TVector<std::pair<TString, TVector<NVins::TPrediction>>> TEST_DATA = {
    {
        "",
        {
            {
                {},
                {},
                1.
            }
        }
    },
    {
        "алиса скажи пожалуйста какая сегодня погода",
        {
            {
                {"nonsense", "nonsense", "nonsense", "sense", "sense", "sense"},
                {0.9994689663, 0.8528910245, 0.999854705, 0.9570732228, 0.9995145771, 0.9998451711},
                0.815204942
            },
            {
                {"nonsense", "sense", "nonsense", "sense", "sense", "sense"},
                {0.9994689663, 0.1471026282, 0.9984412686, 0.9669793785, 0.9995687676, 0.9998564926},
                0.1418664923
            }
        }
    },
    {
        "алиса назови случайное число от 1 до 10 пожалуйста",
        {
            {
                {"nonsense", "sense", "sense", "sense", "sense", "sense", "sense", "sense", "nonsense"},
                {0.9995616213, 0.8877777086, 0.9975310873, 0.9999154878, 0.9953044446, 0.9998694829, 0.9993778764, 0.9998643582, 0.9891365591},
                0.8706224716
            },
            {
                {"nonsense", "nonsense", "sense", "sense", "sense", "sense", "sense", "sense", "nonsense"},
                {0.9995616213, 0.1122198442, 0.9910960234, 0.9998895052, 0.9948435075, 0.999863524, 0.9993429927, 0.9998597103, 0.9894858819},
                0.1093215336
            }
        }
    },
    {
        "пожалуйста видео поставь",
        {
            {
                {"nonsense", "sense", "sense"},
                {0.997670487, 0.9735721259, 0.9633870526},
                0.9357418682
            },
            {
                {"nonsense", "sense", "nonsense"},
                {0.997670487, 0.9735721259, 0.03661166748},
                0.03556106555
            }
        }
    }
};

} // anonymous namespace

Y_UNIT_TEST_SUITE(TRnnTaggerTestSuite) {
    Y_UNIT_TEST(ApplyTest) {
        auto tagger = LoadModel();
        tagger.EstablishSession();

        const auto embedder = GetTokenEmbedder();

        for (const auto& [text, expectedPredictions] : TEST_DATA) {
            const auto sampleFeatures = GetSampleFeatures(embedder, text);

            const auto predictions = tagger.PredictTop(sampleFeatures, TOP_SIZE, BEAM_WIDTH);
            UNIT_ASSERT_VALUES_EQUAL(expectedPredictions.size(), predictions.size());
            for (size_t predictionIndex = 0; predictionIndex < predictions.size(); ++predictionIndex) {
                const auto& expectedPrediction = expectedPredictions[predictionIndex];
                const auto& resultPrediction = predictions[predictionIndex];

                UNIT_ASSERT_DOUBLES_EQUAL(expectedPrediction.FullProbability,
                                          resultPrediction.FullProbability, 1e-6);

                UNIT_ASSERT_VALUES_EQUAL(expectedPrediction.Tags.size(), resultPrediction.Tags.size());
                for (size_t tokenIndex = 0; tokenIndex < resultPrediction.Tags.size(); ++tokenIndex) {
                    UNIT_ASSERT_DOUBLES_EQUAL(expectedPrediction.Probabilities[tokenIndex],
                                              resultPrediction.Probabilities[tokenIndex], 1e-6);
                    UNIT_ASSERT_VALUES_EQUAL(expectedPrediction.Tags[tokenIndex], resultPrediction.Tags[tokenIndex]);
                }
            }
        }
    }
}
