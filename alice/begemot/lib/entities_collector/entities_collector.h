#pragma once

#include "aligned_entities.h"
#include "entity_collecting.h"
#include "workaround.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/sample/sample.h>
#include <search/begemot/apphost/context.h>
#include <search/begemot/rules/alice/ar_fst/proto/alice_ar_fst.pb.h>
#include <search/begemot/rules/alice/custom_entities/proto/alice_custom_entities.pb.h>
#include <search/begemot/rules/alice/nonsense_tagger/proto/alice_nonsense_tagger.pb.h>
#include <search/begemot/rules/alice/request/proto/alice_request.pb.h>
#include <search/begemot/rules/alice/thesaurus/proto/alice_thesaurus.pb.h>
#include <search/begemot/rules/alice/translit/proto/alice_translit.pb.h>
#include <search/begemot/rules/alice/type_parser/proto/alice_type_parser.pb.h>
#include <search/begemot/rules/alice/user_entities/proto/alice_user_entities.pb.h>
#include <search/begemot/rules/entity_finder/proto/entity_finder.pb.h>
#include <search/begemot/rules/external_markup/proto/external_markup.pb.h>
#include <search/begemot/rules/external_markup/proto/format.pb.h>
#include <search/begemot/rules/fst/proto/result.pb.h>
#include <search/begemot/rules/granet/proto/granet.pb.h>
#include <search/begemot/rules/occurrences/custom_entities/rule/proto/custom_entities.pb.h>

#include <util/generic/noncopyable.h>

namespace NBg::NAliceEntityCollector {

// ~~~~ TEntitiesCollector ~~~~

class TEntitiesCollector : public TNonCopyable {
public:
    explicit TEntitiesCollector(const TVector<TString>& tokens);

    void SetIsPASkills(bool value);

    template<typename TRule>
    void Collect(const TRule& rule, const TRuleContext& ctx);

    template<typename TRule>
    void CollectForBinaryIntentClassifier(const TRule& rule, const TRuleContext& ctx);

    void CollectGranetEntities(const NProto::TGranetResult* srcResult);

    const TAlignedEntities& GetAlignedEntities() const;

private:
    void CollectNonsense(const NProto::TAliceNonsenseTaggerResult* nonsenseResult);
    void CollectCustomEntities(const NProto::TCustomEntitiesResult* customEntitiesResult);
    void CollectAliceTypeParserResult(const NProto::TAliceTypeParserTimeResult* srcResult);
    void CollectExternalMarkupEntities(const NProto::TExternalMarkupProto& markup, const TVector<TString>& tokens);
    static TVector<TString> CollectExternalMarkupTokens(const NProto::TExternalMarkupProto& markup);
    void CollectEntityFinderEntities(const NProto::TEntityFinderResult* entityFinderResult,
                                     const TVector<TString>& tokens);
    template<typename TRule>
    void CollectAllFstEntities(const TRule& rule, const TRuleContext& ctx);

    template<typename TRule>
    void CollectAllArFstEntities(const TRule& rule, const TRuleContext& ctx);

    template<class TFstResultType>
    void CollectRepeatedFstEntities(const ::google::protobuf::RepeatedPtrField<TFstResultType>& srcRepeatedResult);

    template<class TFstResultType>
    void CollectFstEntities(const TFstResultType* srcResult);

    template<class TAliceResultType>
    void CollectAliceEntities(const TAliceResultType* srcResult);

    void FindWorkaroundEntities(const NProto::TAliceRequest* request);

private:
    const TVector<TString> Tokens;
    bool IsPASkills = false;
    TAlignedEntities AlignedEntities;
};

// ~~~~ TEntitiesCollector template methods ~~~~

template<typename TRule>
void TEntitiesCollector::Collect(const TRule& rule, const TRuleContext& ctx) {
    CollectNonsense(ctx.Get<NProto::TAliceNonsenseTaggerResult>(&rule));
    CollectAliceEntities(ctx.Get<NProto::TAliceUserEntitiesResult>(&rule));
    CollectCustomEntities(ctx.Get<NProto::TCustomEntitiesResult>(&rule));
    CollectAliceTypeParserResult(ctx.Get<NProto::TAliceTypeParserTimeResult>(&rule));
    CollectAliceEntities(ctx.Get<NProto::TAliceThesaurusResult>(&rule));
    CollectAliceEntities(ctx.Get<NProto::TAliceTranslitResult>(&rule));
    CollectAliceEntities(ctx.Get<NProto::TAliceCustomEntitiesResult>(&rule));

    const NProto::TExternalMarkupResult* markup = ctx.Get<NProto::TExternalMarkupResult>(&rule);
    if (markup != nullptr) {
        const TVector<TString> tokens = CollectExternalMarkupTokens(markup->GetJSON());
        CollectExternalMarkupEntities(markup->GetJSON(), tokens);
        CollectEntityFinderEntities(ctx.Get<NProto::TEntityFinderResult>(&rule), tokens);
    }

    CollectAllFstEntities(rule, ctx);
    FindWorkaroundEntities(ctx.Get<NProto::TAliceRequest>(&rule));
}

template<typename TRule>
void TEntitiesCollector::CollectForBinaryIntentClassifier(const TRule& rule, const TRuleContext& ctx) {
    CollectCustomEntities(ctx.Get<NProto::TCustomEntitiesResult>(&rule));
    CollectAllFstEntities(rule, ctx);
    FindWorkaroundEntities(ctx.Get<NProto::TAliceRequest>(&rule));

    const NProto::TExternalMarkupResult* markup = ctx.Get<NProto::TExternalMarkupResult>(&rule);
    if (markup != nullptr) {
        const TVector<TString> tokens = CollectExternalMarkupTokens(markup->GetJSON());
        CollectExternalMarkupEntities(markup->GetJSON(), tokens);
        CollectEntityFinderEntities(ctx.Get<NProto::TEntityFinderResult>(&rule), tokens);
    }
}

template<typename TRule>
void TEntitiesCollector::CollectAllFstEntities(const TRule& rule, const TRuleContext& ctx) {
    CollectFstEntities(ctx.Get<NProto::TFstAlbumResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstArtistResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstCurrencyResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstFilms100_750Result>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstFilms50FilteredResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstPoiCategoryRuResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstSiteResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstSoftResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstSwearResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstTrackResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstCalcResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstDateResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstDatetimeResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstDatetimeRangeResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstFioResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstFloatResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstGeoResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstNumResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstTimeResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstUnitsTimeResult>(&rule));
    CollectFstEntities(ctx.Get<NProto::TFstWeekdaysResult>(&rule));
    CollectAllArFstEntities(rule, ctx);
}

template<typename TRule>
void TEntitiesCollector::CollectAllArFstEntities(const TRule& rule, const TRuleContext& ctx) {
    const NProto::TAliceArFstResult* aliceArFst = ctx.Get<NProto::TAliceArFstResult>(&rule);
    if (!aliceArFst) {
        return;
    }
    CollectRepeatedFstEntities(aliceArFst->GetNumberEntities());
    CollectRepeatedFstEntities(aliceArFst->GetTimeEntities());
    CollectRepeatedFstEntities(aliceArFst->GetUnitsTimeEntities());
    CollectRepeatedFstEntities(aliceArFst->GetWeekDaysEntities());
    CollectRepeatedFstEntities(aliceArFst->GetDateEntities());
    CollectRepeatedFstEntities(aliceArFst->GetFloatEntities());
    CollectRepeatedFstEntities(aliceArFst->GetDualNumEntities());
    CollectRepeatedFstEntities(aliceArFst->GetDatetimeEntities());
    CollectRepeatedFstEntities(aliceArFst->GetDatetimeRangeEntities());
}

template<class TFstResultType>
void TEntitiesCollector::CollectRepeatedFstEntities(const ::google::protobuf::RepeatedPtrField<TFstResultType>& srcRepeatedResult) {
    for (const TFstResultType& srcResult : srcRepeatedResult) {
        CollectFstEntities(&srcResult);
    }
}

template<class TFstResultType>
void TEntitiesCollector::CollectFstEntities(const TFstResultType* srcResult) {
    if (!srcResult) {
        return;
    }
    NBg::NAliceEntityCollector::CollectFstEntities(srcResult->GetEntities(), &AlignedEntities, IsPASkills);
}

template<class TAliceResultType>
void TEntitiesCollector::CollectAliceEntities(const TAliceResultType* srcResult) {
    if (!srcResult) {
        return;
    }
    NBg::NAliceEntityCollector::CollectAliceEntities(srcResult->GetEntities(), srcResult->GetTokens(),
                                                     &AlignedEntities);
}

inline NNlu::TInterval ToInterval(const NProto::TExternalMarkupProto::TTokenSpan& span) {
    return {span.GetBegin(), span.GetEnd()};
}

} // namespace NBg::NAliceEntityCollector
