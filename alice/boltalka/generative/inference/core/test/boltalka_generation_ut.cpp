#include <alice/boltalka/generative/inference/core/model.h>
#include <alice/boltalka/generative/inference/core/proto/tokenizer_type.pb.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/threading/future/async.h>

#include <util/thread/pool.h>

#include <util/generic/algorithm.h>
#include <util/memory/blob.h>

using namespace NGenerativeBoltalka;

TGenerativeBoltalka LoadModel(const TGenerativeBoltalka::TParams& params) {
    return TGenerativeBoltalka(params);
}

TGenerativeBoltalka::TParams BasicBoltalkaParams(const TString& checkpointPath) {
    TGenerativeBoltalka::TParams params;
    params.ModelParams.CheckpointPath = checkpointPath + "/model.npz";
    params.ModelParams.GpuIds = {0};
    params.ModelParams.ShouldUseGpus = true;
    params.ModelParams.BeamSize = 10;
    params.ModelParams.MaxGenerationMsModel = 30000;

    params.TokenizerParams.BpeVocPath = checkpointPath + "/bpe.voc";
    params.TokenizerParams.TokenToIdVocPath = checkpointPath + "/token_to_id.voc";

    // filtering turned off
    params.FilterParams.FilterEmpty = false;
    params.FilterParams.FilterDuplicateWords = false;
    params.FilterParams.FilterBadWords = false;
    params.FilterParams.BadWordsUseLemmatizer = false;
    params.FilterParams.FilterDuplicateNGrams = false;
    params.FilterParams.FilterByUniqueWordsRatio = false;
    return params;
}

TGenerativeBoltalka::TParams GetSeq2SeqParams(const TString& checkpointPath) {
    auto params = BasicBoltalkaParams(checkpointPath);
    params.TokenizerParams.TokenizerType = ETokenizerType::GENERATIVE;
    params.ModelParams.ShouldInitializeScorer = false;
    params.ModelParams.ShouldInitializeEmbedder = false;
    return params;
}

TGenerativeBoltalka::TParams GetLMParams(const TString& checkpointPath) {
    auto params = BasicBoltalkaParams(checkpointPath);
    params.ModelParams.UsePrefixedInference = true;
    params.ModelParams.ShouldInitializeScorer = false;
    params.TokenizerParams.TokenizerType = ETokenizerType::ZELIBOBA_SEP;
    return params;
}

void AssertEqualResponses(const TVector<TString>& expected, const TVector<TGenerativeResponse>& responses) {
    UNIT_ASSERT_EQUAL(expected.size(), responses.size());
    for (size_t i = 0; i < responses.size(); i++) {
        UNIT_ASSERT_VALUES_EQUAL(expected[i], responses[i].Response);
    }
}

Y_UNIT_TEST_SUITE(BoltalkaSeq2Seq) {
    Y_UNIT_TEST(BeamSizeTen) {
        TString path = "./base_transformer/data";
        auto params = GetSeq2SeqParams(path);
        const auto& model = LoadModel(params);
        int seed = 42;
        const auto& responses = model.GenerateResponses({"привет!"}, 1, seed);

        TVector<TString> expected = {
                "Привет!", "Привет! Как дела?", "Здравствуйте!",  "Как дела?",
                "Как настроение?", "Как поживаете?", "Привет, привет! Что нового?", "А я уже здесь!",
                "С вами приятно разговаривать.", "Что-то я вас не слышу."
        };
        AssertEqualResponses(expected, responses);
    }

    Y_UNIT_TEST(BeamSizeOne) {
        TString path = "./base_transformer/data";
        auto params = GetSeq2SeqParams(path);
        params.ModelParams.BeamSize = 1;

        const auto& model = LoadModel(params);
        int seed = 42;
        const auto& responses = model.GenerateResponses({"привет!"}, 1, seed);

        TVector<TString> expected = {"Привет!"};

        AssertEqualResponses(expected, responses);
    }

    Y_UNIT_TEST(SameSeedSameResults) {
        TString path = "./base_transformer/data";
        auto params = GetSeq2SeqParams(path);
        params.ModelParams.BeamSize = 2;

        const auto& model = LoadModel(params);

        TVector<TString> expected = {
                "Привет!", "Что-то не так?"
        };

        for (size_t i = 0; i < 10; i++) {
            int seed = 42;
            const auto& responses = model.GenerateResponses({"привет!"}, 1, seed);

            AssertEqualResponses(expected, responses);
        }
    }

    Y_UNIT_TEST(BeamSearchDifferentSeedsSameResults) {
        TString path = "./base_transformer/data";
        auto params = GetSeq2SeqParams(path);
        params.ModelParams.SamplingStrategy = "beam_search";
        params.ModelParams.BeamSize = 2;

        const auto& model = LoadModel(params);
        TVector<TString> expected = {
                "Привет!", "Привет! Как дела?"
        };

        for (size_t i = 0; i < 10; i++) {
            int seed = 42 + i;
            const auto& responses = model.GenerateResponses({"привет!"}, 1, seed);

            AssertEqualResponses(expected, responses);
        }
    }
}

Y_UNIT_TEST_SUITE(BoltalkaZeliboba) {
    Y_UNIT_TEST(BeamSizeTen) {
        TString path = "./zeliboba_1_0_boltalka/v19_better";
        const auto& params = GetLMParams(path);
        const auto& model = LoadModel(params);
        int seed = 42;
        const auto& responses = model.GenerateResponses({"привет!"}, 1, seed);

        TVector<TString> expected = {
                "Привет!", "Как дела?", "Привет, привет!", "Привет! Как дела?", "Добрый вечер!",
                "Как ваши дела?", "Привет! Как жизнь молодая?", "Привет, Роман!", "А, привет. Как дела?", "Ха, привет, да я просто зашла на минутку, а ты уже тут"
        };
        AssertEqualResponses(expected, responses);
    }

    Y_UNIT_TEST(BeamSizeOne) {
        TString path = "./zeliboba_1_0_boltalka/v19_better";
        auto params = GetLMParams(path);
        params.ModelParams.BeamSize = 1;

        const auto& model = LoadModel(params);
        int seed = 42;
        const auto& responses = model.GenerateResponses({"привет!"}, 1, seed);

        TVector<TString> expected = {"Как дела?"};

        AssertEqualResponses(expected, responses);
    }

    Y_UNIT_TEST(SameSeedSameResults) {
        TString path = "./zeliboba_1_0_boltalka/v19_better";
        auto params = GetLMParams(path);
        params.ModelParams.BeamSize = 2;

        const auto& model = LoadModel(params);

        TVector<TString> expected = {
                "Как дела?", "Привет, Алекс!"
        };

        for (size_t i = 0; i < 10; i++) {
            int seed = 42;
            const auto& responses = model.GenerateResponses({"привет!"}, 1, seed);

            AssertEqualResponses(expected, responses);
        }
    }

    Y_UNIT_TEST(BeamSearchDifferentSeedsSameResults) {
        TString path = "./zeliboba_1_0_boltalka/v19_better";
        auto params = GetLMParams(path);
        params.ModelParams.SamplingStrategy = "beam_search";
        params.ModelParams.BeamSize = 2;

        const auto& model = LoadModel(params);

        TVector<TString> expected = {
                "Привет!", "Привет! Как дела?"
        };

        for (size_t i = 0; i < 10; i++) {
            int seed = 42 + i;
            const auto& responses = model.GenerateResponses({"привет!"}, 1, seed);

            AssertEqualResponses(expected, responses);
        }
    }

    Y_UNIT_TEST(Scoring) {
        TString path = "./zeliboba_1_0_boltalka/v19_better";
        auto params = GetLMParams(path);
        params.ModelParams.ShouldInitializeScorer = true;
        params.ModelParams.ShouldInitializeTranslator = false;
        const auto& model = LoadModel(params);
        const auto& scores = model.GenerateScores({"сколько океанов"});

        UNIT_ASSERT_DOUBLES_EQUAL(scores[0][0], 8.8, 1e-1);
        UNIT_ASSERT_DOUBLES_EQUAL(scores[0][1], 15.3, 1e-1);
        UNIT_ASSERT_DOUBLES_EQUAL(scores[0][2], 0.02, 1e-2);
    }

    Y_UNIT_TEST(BatchGeneration) {
        TString path = "./base_transformer/data";
        auto params = GetSeq2SeqParams(path);
        params.ModelParams.SamplingStrategy = "beam_search";
        params.ModelParams.BeamSize = 2;
        const auto& model = LoadModel(params);

        TVector<TString> contexts{"привет!", "седан класса люкс"};
        TVector<TGenerativeRequest> requestsBatch;
        Transform(contexts.begin(), contexts.end(), std::back_inserter(requestsBatch),
            [](const auto& context) {
                return TGenerativeRequest{
                    .Context = {context},
                    .Seed = 1,
                    .NumHypos = 2,
                    .PrefixOnly = false,
                    .SpanDelimiters = {},
                    .DiverseBeamSearch = false
                };
            });

        TVector<TVector<TString>> expected{
            {"Привет!", "Привет! Как дела?", "Привет!", "Привет! Как дела?"},
            {"Это что такое?", "А что это?", "Это что такое?", "А что это?"},
        };

        const auto responsesBatch = model.GenerateResponses(requestsBatch);
        UNIT_ASSERT_EQUAL(responsesBatch.size(), expected.size());
        for (size_t i = 0; i < expected.size(); ++i) {
            AssertEqualResponses(expected[i], responsesBatch[i]);
        }
    }

    Y_UNIT_TEST(PerRequestSamplingParams) {
        TString path = "./zeliboba_1_0_boltalka/v19_better";
        auto params = GetLMParams(path);

        params.ModelParams.SamplingStrategy = "beam_search";
        params.ModelParams.BeamSize = 2;

        const auto& model = LoadModel(params);
        auto pool = CreateThreadPool(3);
        TVector<NThreading::TFuture<TVector<TGenerativeResponse>>> translations;

        for (size_t i = 0; i < 12; i++) {
            TGenerativeRequest request = {
                .Context = {"привет!"},
                .Seed = 42,
                .NumHypos = 1,
                .PrefixOnly = false,
                .DiverseBeamSearch = false,
            };

            switch (i % 3) {
                case 1:
                    request.SamplerParams = {
                        .Mode = NDict::NMT::TSamplerParams::EMode::Sampling,
                        .Temperature = 0.6,
                    };
                    break;
                case 2:
                    request.SamplerParams = {
                        .Mode = NDict::NMT::TSamplerParams::EMode::Sampling,
                        .TopNLogits = 50,
                    };
                    break;
                default:
                    // Do not override anything when i % 3 == 0.
                    break;
            }

            translations.push_back(NThreading::Async([&model, request] {
                return model.GenerateResponses(request);
            }, *pool));
        }

        for (size_t i = 0; i < translations.size(); ++i) {
            const auto& responses = translations[i].GetValueSync();
            switch (i % 3) {
                case 0:
                    // beam search results
                    AssertEqualResponses({
                        "Привет!",
                        "Привет! Как дела?",
                    }, responses);
                    break;
                case 1:
                    // sampling with temperature = 0.6
                    AssertEqualResponses({
                        "Как дела?",
                        "Как настроение?",
                    }, responses);
                    break;
                case 2:
                    // sampling with top-50 logits
                    AssertEqualResponses({
                        "Как дела?",
                        "Как дела мои?",
                    }, responses);
                    break;
            }
        }
    }
}
