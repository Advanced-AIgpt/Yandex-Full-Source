#include <alice/nlu/libs/binary_classifier/beggins_binary_classifier.h>
#include <alice/nlu/libs/binary_classifier/boltalka_dssm_embedder.h>
#include <alice/nlu/libs/binary_classifier/dssm_based_binary_classifier.h>
#include <alice/nlu/libs/binary_classifier/lstm_based_binary_classifier.h>
#include <alice/nlu/libs/embedder/embedder.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>
#include <util/stream/str.h>

namespace {

constexpr TStringBuf DSSM_EMBEDDER_DIRECTORY = "alice/nlu/data/ru/boltalka_dssm";
constexpr TStringBuf DSSM_EMBEDDER_CONFIG_FILE = "boltalka_dssm_config.json";
constexpr TStringBuf DSSM_EMBEDDER_FILE = "boltalka_dssm";

constexpr TStringBuf TOKEN_EMBEDDER_DIRECTORY = "search/wizard/data/wizard/AliceTokenEmbedder/ru";

constexpr TStringBuf MODEL_DIRECTORY = "search/wizard/data/wizard/AliceBinaryIntentClassifier/ru/static";
constexpr TStringBuf DSSM_MODELS_DIR = "dssm_models";
constexpr TStringBuf LSTM_MODELS_DIR = "lstm_models";

constexpr TStringBuf MODEL_DESCRIPTION_FILE = "model_description.json";
constexpr TStringBuf MODEL_FILE = "model.pb";

constexpr TStringBuf EMBEDDER_TEST_EXAMPLES_FILE = "embedder_test_examples.json";

struct TEmbedderTestExample {
    TString Query;
    TVector<float> ExpectedEmbedding;
};

struct TClassifierTestExample {
    TString Query;
    bool IsPositive = false;
};

struct TTestCase {
    TString ExamplesPath;
    TString Intent;
    float CertainPositiveProbability = 0.9f;
    float CertainNegativeProbability = 0.1f;
};

const TVector<TTestCase> TEST_CASES = {
    {
        .ExamplesPath = "classifier_test_examples.json",
        .Intent = "personal_assistant.scenarios.music_play",
        .CertainPositiveProbability = 0.9f,
        .CertainNegativeProbability = 0.1f,
    },
    {
        .ExamplesPath = "news_classifier_test_examples.json",
        .Intent = "personal_assistant.scenarios.get_news",
        .CertainPositiveProbability = 0.7f,
        .CertainNegativeProbability = 0.3f,
    },
};

NAlice::TBoltalkaDssmEmbedder LoadDSSMEmbedder() {
    const TFsPath modelDirectory = BinaryPath(DSSM_EMBEDDER_DIRECTORY);
    const TBlob modelBlob = TBlob::PrechargedFromFile(modelDirectory / DSSM_EMBEDDER_FILE);
    TFileInput modelConfigStream(modelDirectory / DSSM_EMBEDDER_CONFIG_FILE);

    return NAlice::TBoltalkaDssmEmbedder(modelBlob, &modelConfigStream);
}

NAlice::TTokenEmbedder LoadTokenEmbedder() {
    const TFsPath modelDirectory = BinaryPath(TOKEN_EMBEDDER_DIRECTORY);
    const TBlob embeddingsBlob = TBlob::PrechargedFromFile(modelDirectory / "embeddings");
    const TBlob dictBlob = TBlob::PrechargedFromFile(modelDirectory / "embeddings_dictionary.trie");

    return NAlice::TTokenEmbedder(embeddingsBlob, dictBlob);
}

NAlice::TDssmBasedBinaryClassifier LoadDSSMClassifier(TStringBuf intent) {
    const TFsPath modelDirectory = BinaryPath(MODEL_DIRECTORY);

    TFileInput modelStream(modelDirectory / DSSM_MODELS_DIR / intent / MODEL_FILE);
    TFileInput modelDescriptionStream(modelDirectory / DSSM_MODELS_DIR / intent / MODEL_DESCRIPTION_FILE);
    NAlice::TDssmBasedBinaryClassifier model(&modelStream, &modelDescriptionStream);
    model.EstablishSessionIfNotYet();

    return model;
}

TMaybe<NAlice::TLstmBasedBinaryClassifier> LoadLSTMClassifier(const NAlice::TTokenEmbedder& embedder, TStringBuf intent) {
    const TFsPath modelsDirectory = BinaryPath(MODEL_DIRECTORY);
    const TFsPath modelDirectory = modelsDirectory / LSTM_MODELS_DIR / intent;
    if (!modelDirectory.IsDirectory()) {
        return Nothing();
    }

    TFileInput modelStream(modelDirectory / MODEL_FILE);
    TFileInput modelDescriptionStream(modelDirectory / MODEL_DESCRIPTION_FILE);

    NAlice::TLstmBasedBinaryClassifier model(embedder, &modelStream, &modelDescriptionStream);
    return model;
}

TVector<TEmbedderTestExample> LoadEmbedderTestExamples() {
    NJson::TJsonValue testExamplesJson;
    const bool readCorrectly = NJson::ReadJsonTree(NResource::Find(EMBEDDER_TEST_EXAMPLES_FILE),
                                                   &testExamplesJson);
    UNIT_ASSERT_C(readCorrectly, "Cannot parse " << EMBEDDER_TEST_EXAMPLES_FILE);

    TVector<TEmbedderTestExample> testExamples;
    for (const auto& testExampleJson : testExamplesJson.GetArraySafe()) {
        TEmbedderTestExample example;
        example.Query = testExampleJson["query"].GetStringSafe();
        for (const auto& value : testExampleJson["embedding"].GetArraySafe()) {
            example.ExpectedEmbedding.push_back(value.GetDoubleSafe());
        }
        testExamples.push_back(example);
    }

    return testExamples;
}

TVector<TClassifierTestExample> LoadClassifierTestExamples(TStringBuf classifierTestSamplesFile) {
    NJson::TJsonValue testExamplesJson;
    const bool readCorrectly = NJson::ReadJsonTree(NResource::Find(classifierTestSamplesFile),
                                                   &testExamplesJson);
    UNIT_ASSERT_C(readCorrectly, "Cannot parse " << classifierTestSamplesFile);

    TVector<TClassifierTestExample> testExamples;
    for (const auto& testExampleJson : testExamplesJson.GetArraySafe()) {
        TClassifierTestExample example;
        example.Query = testExampleJson["query"].GetStringSafe();
        example.IsPositive = testExampleJson["is_positive"].GetBooleanSafe();
        testExamples.push_back(example);
    }

    return testExamples;
};

template <typename TClassifier>
void EvaluateClassifier(const TClassifier& classifier, const TTestCase& testCase)
{
    const auto testExamples = LoadClassifierTestExamples(testCase.ExamplesPath);

    for (const auto& testExample : testExamples) {
        const float predictedProb = classifier(testExample.Query);
        if (testExample.IsPositive) {
            UNIT_ASSERT_C(predictedProb > testCase.CertainPositiveProbability,
                testExample.Query << " is not certainly positive, P = " << predictedProb);
        } else {
            UNIT_ASSERT_C(predictedProb < testCase.CertainNegativeProbability,
                testExample.Query << " is not certainly negative, P = " << predictedProb);
        }
    }
}

} // namespace

Y_UNIT_TEST_SUITE(TBoltalkaDssmApplierTestSuite) {
    Y_UNIT_TEST(ApplyTest) {
        const auto embedder = LoadDSSMEmbedder();
        const auto testExamples = LoadEmbedderTestExamples();

        for (const auto& testExample : testExamples) {
            const auto predictedEmbedding = embedder.Embed(testExample.Query);

            UNIT_ASSERT_VALUES_EQUAL_C(predictedEmbedding.size(), testExample.ExpectedEmbedding.size(),
                "Embeddings for " << testExample.Query << " are different:"
                << " expected size: " << testExample.ExpectedEmbedding.size()
                << " got: " << predictedEmbedding.size());

            for (size_t i = 0; i < predictedEmbedding.size(); ++i) {
                UNIT_ASSERT_DOUBLES_EQUAL_C(predictedEmbedding[i], testExample.ExpectedEmbedding[i], 1e-6,
                    "Embeddings for " << testExample.Query << " are different:"
                    << " expected: " << testExample.ExpectedEmbedding[i] << " at position " << i
                    << " got: " << predictedEmbedding[i]);
            }
        }
    }
}

Y_UNIT_TEST_SUITE(TBinaryClassifierTestSuite) {
    Y_UNIT_TEST(SanityTest) {
        const auto tokenEmbedder = LoadTokenEmbedder();
        const auto dssmEmbedder = LoadDSSMEmbedder();

        for (const auto& testCase : TEST_CASES) {
            const auto dssmClassifier = LoadDSSMClassifier(testCase.Intent);
            EvaluateClassifier([&dssmClassifier, &dssmEmbedder](const TString& request) {
                return dssmClassifier.PredictProbability(dssmEmbedder.Embed(request));
            }, testCase);

            const auto lstmClassifier = LoadLSTMClassifier(tokenEmbedder, testCase.Intent);
            if (!lstmClassifier) {
                continue;
            }
            EvaluateClassifier([&lstmClassifier](const TString& request) {
                return lstmClassifier->PredictProbability(request);
            }, testCase);
        }
    }

    Y_UNIT_TEST(SimpleBegginsCatBoostClassifierCheck) {
        const auto modelData = NResource::Find("beggins_catboost_model.cbm");
        TStringInput modelStream(modelData);
        NAlice::TBegginsCatBoostBinaryClassifier model(&modelStream);
        TVector<float> embedding(1024);
        for (int i = 0; i < 1024; ++i) {
            embedding[i] = 10. * ((i & 1) ? 1 : -1) * (i + 1) / (i + 100);
        }
        UNIT_ASSERT_DOUBLES_EQUAL(model.PredictProbability(embedding), -8.33923912, 1e-7);
    }
}
