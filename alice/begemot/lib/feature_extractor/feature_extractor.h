#pragma once

#include <alice/begemot/lib/entities_collector/entity_collecting.h>
#include <alice/begemot/lib/rule_utils/rule_input.h>
#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/sample_features/sample_features.h>
#include <alice/nlu/libs/token_aligner/aligner.h>
#include <alice/library/json/json.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/langs/langs.h>
#include <search/begemot/core/rulecontext.h>
#include <search/begemot/rules/alice/embeddings/proto/alice_embeddings.pb.h>
#include <search/begemot/rules/alice/normalizer/proto/alice_normalizer.pb.h>
#include <search/begemot/rules/dirty_lang/proto/dirty_lang.pb.h>
#include <search/begemot/rules/entity_finder/proto/entity_finder.pb.h>
#include <search/begemot/rules/external_markup/proto/external_markup.pb.h>
#include <search/begemot/rules/fst/proto/result.pb.h>
#include <search/begemot/rules/is_nav/proto/is_nav.pb.h>
#include <search/begemot/rules/occurrences/custom_entities/rule/proto/custom_entities.pb.h>
#include <search/begemot/rules/alice/custom_entities/proto/alice_custom_entities.pb.h>


namespace NBg {
    constexpr TStringBuf NLU_EXTRA = "bg_nlu_extra";

    // Converts results of begemot rules to NVins::TSampleFeatures used in TRnnTagger
    class TFeatureExtractor {
    public:
        template <class TRule>
        NVins::TSampleFeatures ExtractSampleFeatures(
            const TRule* rule,
            const TRuleContext& ctx,
            ELanguage lang
        ) const;

    private:
        static NNlu::TInterval GetInterval(size_t begin, size_t end, const NNlu::TAlignment& alignment);

        NVins::TSampleFeatures ExtractWordEmbeddingsFeatures(
            const NProto::TAliceEmbeddingsResult& embeddingsProto
        ) const;

        template <class TRule>
        TVector<TVector<NVins::TSparseFeature>> ExtractNerFeatures(
            const TRule* rule,
            const TVector<TString>& normalizedTokens,
            const NNlu::TAlignment& alignment,
            int tokenCount,
            const TRuleContext& ctx
        ) const;

        template <class TRule>
        TVector<TVector<NVins::TSparseFeature>> ExtractWizardFeatures(
            const TRule* rule,
            const NProto::TExternalMarkupProto& externalMarkup,
            const NNlu::TAlignment& alignment,
            int tokenCount,
            const TRuleContext& ctx
        ) const;

        TVector<TVector<NVins::TSparseFeature>> ExtractPosTagFeatures(
            const NProto::TExternalMarkupProto& externalMarkup,
            const NNlu::TAlignment& alignment,
            int tokenCount
        ) const;

        void AddFeatureSpan(
            const TString& featureName,
            size_t begin,
            size_t end,
            TVector<TVector<NVins::TSparseFeature>>* features
        ) const;

        void ExtractCustomEntities(
            const NProto::TCustomEntitiesResult* customEntitiesResult,
            const NNlu::TAlignment& alignment,
            TVector<TVector<NVins::TSparseFeature>>* nerFeatures
        ) const;

        template <typename TFstResultType>
        void ExtractFstEntities(
            const TVector<TString>& normalizedTokens,
            const TFstResultType* fstResult,
            TVector<TVector<NVins::TSparseFeature>>* nerFeatures
        ) const;

        template <typename TFstResultType>
        void CollectFstEntities(
            const TFstResultType* fstResult,
            const TVector<size_t>& alignment,
            TVector<TVector<NVins::TSparseFeature>>* nerFeatures
        ) const;

        void ExtractGeoAddrFeature(
            const NProto::TExternalMarkupProto& externalMarkup,
            const NNlu::TAlignment& alignment,
            TVector<TVector<NVins::TSparseFeature>>* features
        ) const;

        void ExtractDateFeature(
            const NProto::TExternalMarkupProto& externalMarkup,
            const NNlu::TAlignment& alignment,
            TVector<TVector<NVins::TSparseFeature>>* features
        ) const;

        void ExtractFioFeature(
            const NProto::TExternalMarkupProto& externalMarkup,
            const NNlu::TAlignment& alignment,
            TVector<TVector<NVins::TSparseFeature>>* features
        ) const;

        void ExtractIsNavFeature(
            const NProto::TIsNavResult* isNavResult,
            size_t tokensCount,
            TVector<TVector<NVins::TSparseFeature>>* features
        ) const;

        void ExtractDirtyLangFeature(
            const NProto::TDirtyLangResult* dirtyLangResult,
            size_t tokensCount,
            TVector<TVector<NVins::TSparseFeature>>* features
        ) const;

        void ExtractEntityFinderFeature(
            const NProto::TEntityFinderResult* entityFinderResult,
            const NNlu::TAlignment& alignment,
            TVector<TVector<NVins::TSparseFeature>>* features
        ) const;

        void ExtractLocationAtHomeFeature(
            const NProto::TAliceCustomEntitiesResult* aliceCustomEntitiesResult,
            const NProto::TExternalMarkupProto& externalMarkup,
            const NJson::TJsonValue& nluExtra,
            const NNlu::TAlignment& alignment,
            TVector<TVector<NVins::TSparseFeature>>* features
        ) const;

        static TVector<TString> GetExternalMarkupTokens(const NProto::TExternalMarkupProto& markup);
        static TVector<TString> GetNormalizedTokens(const NProto::TAliceNormalizerResult& normalizerResult);
        static NJson::TJsonValue DecodeNluExtra(const TString& encoded);
    };

    template <class TRule>
    NVins::TSampleFeatures TFeatureExtractor::ExtractSampleFeatures(
        const TRule* rule,
        const TRuleContext& ctx,
        const ELanguage lang
    ) const {
        NVins::TSampleFeatures sampleFeatures = ExtractWordEmbeddingsFeatures(ctx.Require<NProto::TAliceEmbeddingsResult>(rule));

        // ToDo(DIALOG-8243) need config not to hardcode this
        if (lang == LANG_ARA) {
            return sampleFeatures;
        }

        const NProto::TExternalMarkupProto& externalMarkup = ctx.Require<NProto::TExternalMarkupResult>(rule).GetJSON();
        const NProto::TAliceNormalizerResult& normalizerResult = ctx.Require<NProto::TAliceNormalizerResult>(rule);
        const TVector<TString> externalMarkupTokes = GetExternalMarkupTokens(externalMarkup);
        const TVector<TString> normalizedTokens = GetNormalizedTokens(normalizerResult);
        const NNlu::TAlignment alignment = NNlu::TTokenAligner{}.Align(externalMarkupTokes, normalizedTokens);
        const int tokenCount = normalizedTokens.size();

        Y_ENSURE(sampleFeatures.Sample.Tokens.ysize() == tokenCount,
            "Expected equal token and embedding counts for the request " << externalMarkup.GetOriginalRequest()
            << ", got: " << tokenCount << " tokens and " << sampleFeatures.Sample.Tokens.ysize() << " embeddings");

        sampleFeatures.SparseSeq["ner"] = ExtractNerFeatures(rule, normalizedTokens, alignment, tokenCount, ctx);
        sampleFeatures.SparseSeq["wizard"] = ExtractWizardFeatures(rule, externalMarkup, alignment, tokenCount, ctx);
        sampleFeatures.SparseSeq["postag"] = ExtractPosTagFeatures(externalMarkup, alignment, tokenCount);

        return sampleFeatures;
    }

    template <class TRule>
    TVector<TVector<NVins::TSparseFeature>> TFeatureExtractor::ExtractNerFeatures(
        const TRule* rule,
        const TVector<TString>& normalizedTokens,
        const NNlu::TAlignment& alignment,
        int tokenCount,
        const TRuleContext& ctx
    ) const {
        TVector<TVector<NVins::TSparseFeature>> nerFeatures(tokenCount);

        ExtractCustomEntities(ctx.Get<NProto::TCustomEntitiesResult>(rule), alignment, &nerFeatures);

        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstAlbumResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstArtistResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstCurrencyResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstFilms100_750Result>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstFilms50FilteredResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstPoiCategoryRuResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstSiteResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstSoftResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstSwearResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstTrackResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstCalcResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstDateResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstDatetimeResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstDatetimeRangeResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstFioResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstFloatResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstGeoResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstNumResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstTimeResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstUnitsTimeResult>(rule), &nerFeatures);
        ExtractFstEntities(normalizedTokens, ctx.Get<NProto::TFstWeekdaysResult>(rule), &nerFeatures);

        return nerFeatures;
    }

    template <class TRule>
    TVector<TVector<NVins::TSparseFeature>> TFeatureExtractor::ExtractWizardFeatures(
        const TRule* rule,
        const NProto::TExternalMarkupProto& externalMarkup,
        const NNlu::TAlignment& alignment,
        int tokenCount,
        const TRuleContext& ctx
    ) const {
        TVector<TVector<NVins::TSparseFeature>> wizardFeatures(tokenCount);

        ExtractGeoAddrFeature(externalMarkup, alignment, &wizardFeatures);
        ExtractDateFeature(externalMarkup, alignment, &wizardFeatures);
        ExtractFioFeature(externalMarkup, alignment, &wizardFeatures);

        ExtractIsNavFeature(ctx.Get<NProto::TIsNavResult>(rule), tokenCount, &wizardFeatures);
        ExtractDirtyLangFeature(ctx.Get<NProto::TDirtyLangResult>(rule), tokenCount, &wizardFeatures);

        ExtractEntityFinderFeature(ctx.Get<NProto::TEntityFinderResult>(rule), alignment, &wizardFeatures);
        ExtractLocationAtHomeFeature(
            ctx.Get<NProto::TAliceCustomEntitiesResult>(rule),
            externalMarkup,
            DecodeNluExtra(NAlice::GetWizextraParam(ctx.Require<TInput>(rule), NLU_EXTRA)),
            alignment,
            &wizardFeatures
        );

        return wizardFeatures;
    }


    template <typename TFstResultType>
    void TFeatureExtractor::ExtractFstEntities(
        const TVector<TString>& normalizedTokens,
        const TFstResultType* fstResult,
        TVector<TVector<NVins::TSparseFeature>>* nerFeatures
    ) const {
        if (!fstResult) {
            return;
        }

        TVector<NGranet::TEntity> entities;
        const TVector<TString> tokens = NAliceEntityCollector::CollectFstEntities(
            fstResult->GetEntities(),
            [&entities](NGranet::TEntity entity){ entities.emplace_back(std::move(entity)); }
        );
        NAliceEntityCollector::TAlignedEntities alignedEntities{normalizedTokens};
        alignedEntities.AddEntities(tokens, std::move(entities));
        for (const NGranet::TEntity& entity : alignedEntities.GetEntities()) {
            const double weight = entity.Quality > 0.0 ? entity.Quality : 1.;
            for (size_t tokenIndex = entity.Interval.Begin; tokenIndex < entity.Interval.End; ++tokenIndex) {
                const TString bioPrefix = (tokenIndex == entity.Interval.Begin) ? "B-" : "I-";
                (*nerFeatures)[tokenIndex].emplace_back(bioPrefix + entity.Type, weight);
            }
        }
    }

    template <typename TFstResultType>
    void TFeatureExtractor::CollectFstEntities(
        const TFstResultType* fstResult,
        const TVector<size_t>& alignment,
        TVector<TVector<NVins::TSparseFeature>>* nerFeatures
    ) const {
        const auto& fstEntities = fstResult->GetEntities();

        size_t prevEntityTokenIndex = NPOS;
        for (size_t tokenIndex = 0; tokenIndex < alignment.size(); ++tokenIndex) {
            const size_t entityTokenIndex = alignment[tokenIndex];
            const auto& entity = fstEntities[entityTokenIndex];
            const auto& entityType = entity.GetType();
            if (entityType.empty()) {
                continue;
            }

            TString bioPrefix = "I-";
            if (prevEntityTokenIndex != entityTokenIndex) {
                bioPrefix = "B-";
                prevEntityTokenIndex = entityTokenIndex;
            }

            const double weight = entity.HasWeight() ? entity.GetWeight() : 1.;
            (*nerFeatures)[tokenIndex].emplace_back(bioPrefix + entityType, weight);
        }
    }

} // namespace NBg
