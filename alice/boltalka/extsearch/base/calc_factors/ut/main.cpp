#include <alice/boltalka/extsearch/base/calc_factors/basic_text_factors.h>
#include <alice/boltalka/extsearch/base/calc_factors/dssm_cos_factors.h>
#include <alice/boltalka/extsearch/base/calc_factors/factor_calcer.h>
#include <alice/boltalka/extsearch/base/calc_factors/pronoun_factors.h>
#include <alice/boltalka/extsearch/base/calc_factors/rus_lister_factors.h>
#include <alice/boltalka/extsearch/base/calc_factors/text_intersect_factors.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/fast.h>
#include <util/string/join.h>

using namespace NNlg;

Y_UNIT_TEST_SUITE(TNlgCalcFactorsText) {

    Y_UNIT_TEST(TestPunctFactors) {
        TBasicTextFactors factors;
        {
            TString query = "привет , меня зовут алиса ? ! !";
            TString punct = "?!.,()";
            TVector<float> features;
            factors.AppendPunctFactors(query, &features, punct);
            UNIT_ASSERT_EQUAL(features, (TVector<float>{ 1.0f, 2.0f, 0.0f, 1.0f, 0.0f, 0.0f }));
        }
        {
            TString query = "";
            TString punct = "?!.,()";
            TVector<float> features;
            factors.AppendPunctFactors(query, &features, punct);
            UNIT_ASSERT_EQUAL(features, TVector<float>(punct.size()));
        }
    }

    Y_UNIT_TEST(TestWordLenFactors) {
        TBasicTextFactors factors;
        {
            TString query = "hi , my name is alice !";
            TVector<float> features;
            factors.AppendWordLengthFactors(query, &features);
            UNIT_ASSERT_EQUAL(features, (TVector<float>{ 7.0f, 17.0f, 17.0 / 7.0, 1.0f, 5.0f }));
        }
        {
            TString query = "";
            TVector<float> features;
            factors.AppendWordLengthFactors(query, &features);
            UNIT_ASSERT_EQUAL(features, TVector<float>(5));
        }
    }

    Y_UNIT_TEST(TestQueryFactors) {
        TBasicTextFactors factors;
        TVector<TString> query = { "привет , меня зовут алиса ? ! !", "hi , my name is alice !", "" };
        const size_t numDocs = 5;
        TVector<TVector<float>> features(numDocs);
        factors.AppendQueryFactors(query, &features);
        for (size_t i = 1; i < numDocs; ++i) {
            UNIT_ASSERT_EQUAL(features[0], features[i]);
        }
    }

    Y_UNIT_TEST(TestWordIntersectFactors) {
        TTextIntersectionFactors factors;
        TString query = "привет , меня зовут алиса ? !";
        auto queryFreq = TTextIntersectionFactors::GetWordFrequencies(query);
        UNIT_ASSERT_EQUAL(TTextIntersectionFactors::CalcIntersection(queryFreq, TTextIntersectionFactors::GetWordFrequencies("")), 0);
        UNIT_ASSERT_EQUAL(TTextIntersectionFactors::CalcIntersection(queryFreq, TTextIntersectionFactors::GetWordFrequencies("привет алиса !")), 3);
        UNIT_ASSERT_EQUAL(TTextIntersectionFactors::CalcIntersection(queryFreq, queryFreq), 7);
    }

    Y_UNIT_TEST(TestTrigramIntersectFactors) {
        TTextIntersectionFactors factors;
        TString query = "ababa !";
        auto queryFreq = TTextIntersectionFactors::GetTrigramFrequencies(query);
        UNIT_ASSERT_EQUAL(TTextIntersectionFactors::CalcIntersection(queryFreq, TTextIntersectionFactors::GetTrigramFrequencies("")), 0);
        UNIT_ASSERT_EQUAL(TTextIntersectionFactors::CalcIntersection(queryFreq, TTextIntersectionFactors::GetTrigramFrequencies("ababa")), 3);
        UNIT_ASSERT_EQUAL(TTextIntersectionFactors::CalcIntersection(queryFreq, queryFreq), query.size() - 2);
    }

    Y_UNIT_TEST(TestRusListerFactors) {
        TVector<TString> fileLines = {
            "я\t1 38 0",
            "тестирую\t0",
            "тебя\t0 1 2 3 4 5 6 7 38"
        };
        TRusListerFactors factors(fileLines);
        TString query = "я тестирую тебя алиса !";
        TVector<float> features;
        factors.AppendRusListerFactors(query, &features);
        UNIT_ASSERT_EQUAL(features.size(), 40);
        UNIT_ASSERT_EQUAL(features[0], 3);
        UNIT_ASSERT_EQUAL(features[1], 2);
        UNIT_ASSERT_EQUAL(features[38], 2);
        UNIT_ASSERT(std::all_of(features.begin() + 2, features.begin() + 8, [](float x) { return x == 1.0; }));
        UNIT_ASSERT(std::all_of(features.begin() + 8, features.begin() + 38, [](float x) { return x == 0.0; }));
        UNIT_ASSERT_EQUAL(features[39], 0.4f);
    }

    TString GetRandomTurn(TFastRng<ui64>& rng) {
        const TVector<TString> words = { "я", "test", ".", "!", "алиса", "тебя", "люблю", "ненавижу", "привет" };
        size_t len = rng.Uniform(1, 11);
        TVector<TString> result;
        for (size_t i = 0; i < len; ++i) {
            result.push_back(words[rng.Uniform(words.size())]);
        }
        return JoinSeq(" ", result);
    }

    TVector<TString> GetRandomTurns(TFastRng<ui64>& rng, size_t numTurns) {
        TVector<TString> result;
        for (size_t i = 0; i < numTurns; ++i) {
            result.push_back(GetRandomTurn(rng));
        }
        return result;
    }

    TVector<float> GetRandomVector(TFastRng<ui64>& rng, size_t dimension) {
        TVector<float> result(dimension);
        for (float& x : result) {
            x = 2.0 * rng.GenRandReal4() - 1;
        }
        return result;
    }

    void InitializeRandomDocs(TFastRng<ui64>& rng, size_t numDocs, size_t numTurns,
        TVector<TVector<TString>>& contexts, TVector<TString>& replies) {
        for (size_t i = 0; i < numDocs; ++i) {
            contexts.push_back(GetRandomTurns(rng, rng.Uniform(1, numTurns + 2)));
            replies.push_back(GetRandomTurn(rng));
        }
    }

    TFactorCalcerCtx GetRandomCtx(TFastRng<ui64>& rng, const size_t numDocs, TVector<TVector<TString>>& contexts, TVector<TString>& replies) {
        static const size_t dimension = 51;

        static const TVector<float> queryEmbedding = GetRandomVector(rng, dimension);
        static const TVector<float> contextEmbedding = GetRandomVector(rng, dimension);
        static const TVector<float> replyEmbedding = GetRandomVector(rng, dimension);

        TFactorCalcerCtx ctx(contexts, replies);
        ctx.DssmFactorCtxs["my_model"].QueryEmbedding = queryEmbedding.data();
        for (size_t i = 0; i < numDocs; ++i) {
            ctx.DssmFactorCtxs["my_model"].ContextEmbeddings.push_back(contextEmbedding.data());
            ctx.DssmFactorCtxs["my_model"].ReplyEmbeddings.push_back(replyEmbedding.data());
        }
        ctx.DssmFactorCtxs["my_model"].Dimension = dimension;
        return ctx;
    }

    Y_UNIT_TEST(TestFactorCalcer) {
        const size_t numDocs = 37;
        const size_t numTurns = 3;
        TFastRng<ui64> rng(12347);
        TVector<TVector<TString>> contexts;
        TVector<TString> replies;
        InitializeRandomDocs(rng, numDocs, numTurns, contexts, replies);
        auto ctx = GetRandomCtx(rng, numDocs, contexts, replies);
        TFactorCalcer calcer({
            new TBasicTextFactors,
            new TTextIntersectionFactors,
            new TDssmCosFactors("my_model"),
            new TPronounFactors,
            new TRusListerFactors(TVector<TString>{"я\t0 38", "test\t16", "алиса\t0 1 2 3 4 5 6 7 8"})
        }, numTurns);

        const size_t numStaticFeatures = 5;
        TVector<TVector<float>> staticFeatures;
        TVector<TVector<float>> features;
        for (size_t i = 0; i < numDocs; ++i) {
            staticFeatures.push_back(GetRandomVector(rng, numStaticFeatures));
            features.push_back(staticFeatures.back());
        }

        calcer.CalcFactors(&ctx, &features);
        UNIT_ASSERT_EQUAL(features.size(), numDocs);
        UNIT_ASSERT(std::all_of(features.begin(), features.end(), [&](const auto& f) { return f.size() == features[0].size(); } ));
        for (size_t i = 0; i < numDocs; ++i) {
            for (size_t j = 0; j < numStaticFeatures; ++j) {
                UNIT_ASSERT_EQUAL(features[i][j], staticFeatures[i][j]);
            }
        }
    }
}

