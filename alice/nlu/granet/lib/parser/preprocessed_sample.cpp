#include "preprocessed_sample.h"
#include "state.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <dict/nerutil/tstimer.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/generic/adaptor.h>
#include <util/generic/xrange.h>
#include <util/string/join.h>
#include <util/string/split.h>

using namespace NNlu;

namespace NGranet {

static const THashMap<TString, ESynonymFlags> SYNONYM_TYPE_TO_FLAG = {
    {NEntityTypes::SYN_THESAURUS_TRANSLIT_LEMMA,        ESynonymFlag::SF_TRANSLIT},
    {NEntityTypes::SYN_THESAURUS_TRANSLIT_EN_LEMMA,     ESynonymFlag::SF_TRANSLIT},
    {NEntityTypes::SYN_THESAURUS_TRANSLIT_RU_LEMMA,     ESynonymFlag::SF_TRANSLIT},
    {NEntityTypes::SYN_TRANSLIT_EN_LEMMA,               ESynonymFlag::SF_TRANSLIT},
    {NEntityTypes::SYN_TRANSLIT_RU_LEMMA,               ESynonymFlag::SF_TRANSLIT},
    {NEntityTypes::SYN_TRANSLIT_EN,                     ESynonymFlag::SF_TRANSLIT},
    {NEntityTypes::SYN_TRANSLIT_RU,                     ESynonymFlag::SF_TRANSLIT},
    {NEntityTypes::SYS_FIO_NAME,                        ESynonymFlag::SF_FIO},
    {NEntityTypes::SYS_FIO_PATRONYM,                    ESynonymFlag::SF_FIO},
    {NEntityTypes::SYS_FIO_SURNAME,                     ESynonymFlag::SF_FIO},
    {NEntityTypes::SYN_THESAURUS_DIMIN_NAME_LEMMA,      ESynonymFlag::SF_DIMIN_NAME},
    {NEntityTypes::SYN_THESAURUS_SYNSET_LEMMA,          ESynonymFlag::SF_SYNSET},
    {NEntityTypes::SYN_THESAURUS_LEMMA,                 ESynonymFlag::SF_SYNON},
};

static ESynonymFlags GetSynonymFlag(const TString& entityType) {
    return SYNONYM_TYPE_TO_FLAG.Value(entityType, ESynonymFlags{});
}

// ~~~~ TPreprocessedSample ~~~~

TPreprocessedSample::TRef TPreprocessedSample::Copy(const TSample::TConstRef& sampleCopy) {
    return new TPreprocessedSample(Grammar, sampleCopy, Vertices, SampleEntityCount,
        EntityTypeToIndexes, NeedUpdatePromisingElements, PromisingRootElements);
}

TPreprocessedSample::TPreprocessedSample(
        const TGrammar::TConstRef& grammar,
        const TSample::TConstRef& sample,
        const TVector<TParserVertex>& vertices,
        size_t sampleEntityCount,
        const THashMap<TString, TVector<size_t>>& entityTypeToIndexes,
        bool needUpdatePromisingElements,
        const TDynBitMap& promisingRootElements)
    : Grammar(grammar)
    , GrammarData((grammar->GetData()))
    , Sample(sample)
    , Vertices(vertices)
    , SampleEntityCount(sampleEntityCount)
    , EntityTypeToIndexes(entityTypeToIndexes)
    , NeedUpdatePromisingElements(needUpdatePromisingElements)
    , PromisingRootElements(promisingRootElements)
{
}

// static
TPreprocessedSample::TRef TPreprocessedSample::Create(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample) {
    return new TPreprocessedSample(grammar, sample);
}

TPreprocessedSample::TPreprocessedSample(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample)
    : Grammar(grammar)
    , GrammarData(grammar->GetData())
    , Sample(sample)
{
    DEBUG_TIMER("TPreprocessedSample::TPreprocessedSample");

    Vertices.resize(Sample->GetTokens().size() + 1);
    InitPromisingElementsSubsets();
    InitTokenArcs();
    ProcessNewEntitiesInternal();
    UpdatePromisingElements();
}

void TPreprocessedSample::InitPromisingElementsSubsets() {
    for (TParserVertex& vertex : Vertices) {
        vertex.PromisingByFirstWord = GrammarData.OptimizationInfo.FirstWordToElements.CommonElements;
        vertex.PromisingBySpecificWord = GrammarData.OptimizationInfo.SpecificWordToElements.CommonElements;
    }
    NeedUpdatePromisingElements = true;
}

void TPreprocessedSample::InitTokenArcs() {
    for (const auto& [i, token] : Enumerate(Sample->GetTokens())) {
        const NNlu::TInterval interval = {i, i + 1};
        AddNewTokenArc(interval, 0, GetTokenId(token, false), /* synonymFlag */ 0);
        AddNewTokenArc(interval, WILDCARD_LOG_PROB, TOKEN_WILDCARD, /* synonymFlag */ 0);
    }

    for (const auto& [i, lemmaVariants] : Enumerate(Sample->GetSureLemmas())) {
        const NNlu::TInterval interval = {i, i + 1};
        for (const TStringBuf lemma : StringSplitter(lemmaVariants).Split(',')) {
            AddNewTokenArc(interval, LEMMA_LOG_PROB, GetTokenId(lemma, true), /* synonymFlag */ 0);
        }
    }
}

TTokenId TPreprocessedSample::GetTokenId(TStringBuf word, bool isLemma) const {
    return GetWordTokenId(GrammarData.WordTrie, word, isLemma);
}

void TPreprocessedSample::ProcessNewEntities() {
    if (SampleEntityCount == Sample->GetEntities().size()) {
        return;
    }
    ProcessNewEntitiesInternal();
    UpdatePromisingElements();
}

void TPreprocessedSample::ProcessNewEntitiesInternal() {
    for (; SampleEntityCount < Sample->GetEntities().size(); SampleEntityCount++) {
        ProcessNewEntity(SampleEntityCount);
    }
}

void TPreprocessedSample::ProcessNewEntity(ui32 entityIndex) {
    const TEntity& entity = Sample->GetEntities()[entityIndex];
    EntityTypeToIndexes[entity.Type].push_back(entityIndex);

    if (entity.Interval.Empty()) {
        // Not supported
        return;
    }
    Y_ASSERT(entity.Interval.End < Vertices.size());

    if (const TVector<TElementId>* elements = Grammar->GetEntityNameToElements().FindPtr(entity.Type)) {
        TParserVertex& vertex = Vertices[entity.Interval.Begin];

        for (const TElementId elementId : *elements) {
            TParserEntityArc& entityArc = vertex.EntityArcs[elementId].emplace_back();
            const double extraLogProb = AMBIGUOUS_ENTITY_EXTRA_LOG_PROB.Value(entity.Type, 0.);
            entityArc.LogProb = ToFloat(entity.LogProbability + extraLogProb);
            entityArc.To = ToInt(entity.Interval.End);
            entityArc.EntityIndexesInSample = {entityIndex, 1};//!!!!

            vertex.PromisingByEntity.Set(elementId);
            // Instead of "NeedUpdatePromisingElements = true"
            vertex.PromisingElements.Set(elementId);
        }
    }

    if (entity.Type == NEntityTypes::SYS_NUM
        || entity.Type == NEntityTypes::SYS_FLOAT
        || entity.Type == NEntityTypes::PA_SKILLS_NUMBER)
    {
        AddNewTokenArc(entity.Interval, LEMMA_LOG_PROB, GetTokenId(entity.Value, false), /* synonymFlag */ 0);
        AddNewTokenArc(entity.Interval, LEMMA_LOG_PROB, GetTokenId(entity.Value, true), /* synonymFlag */ 0);
    }
    if (entity.Type == NEntityTypes::SYS_FIO_NAME
        || entity.Type == NEntityTypes::SYS_FIO_PATRONYM
        || entity.Type == NEntityTypes::SYS_FIO_SURNAME)
    {
        TStringBuf value = entity.Value;
        if (TryRemoveBraces(&value, '"', '"')) {
            AddNewTokenArc(entity.Interval, LEMMA_LOG_PROB, GetTokenId(value, true), GetSynonymFlag(entity.Type));
        }
    }
    if (entity.Type.StartsWith(NEntityTypePrefixes::SYN)) {
        if (entity.Type == NEntityTypes::SYN_TRANSLIT_EN
            || entity.Type == NEntityTypes::SYN_TRANSLIT_RU)
        {
            const float logProb = ToFloat(entity.LogProbability);
            for (const auto& subValue : StringSplitter(entity.Value).Split(',')) {
                AddNewTokenArc(entity.Interval, logProb, GetTokenId(subValue, false), GetSynonymFlag(entity.Type));
            }
        }
        if (entity.Type == NEntityTypes::SYN_THESAURUS_LEMMA
            || entity.Type == NEntityTypes::SYN_THESAURUS_DIMIN_NAME_LEMMA
            || entity.Type == NEntityTypes::SYN_THESAURUS_SYNSET_LEMMA
            || entity.Type == NEntityTypes::SYN_THESAURUS_TRANSLIT_LEMMA
            || entity.Type == NEntityTypes::SYN_THESAURUS_TRANSLIT_EN_LEMMA
            || entity.Type == NEntityTypes::SYN_THESAURUS_TRANSLIT_RU_LEMMA
            || entity.Type == NEntityTypes::SYN_TRANSLIT_EN_LEMMA
            || entity.Type == NEntityTypes::SYN_TRANSLIT_RU_LEMMA)
        {
            const float logProb = ToFloat(entity.LogProbability) + LEMMA_LOG_PROB;
            for (const auto& subValue : StringSplitter(entity.Value).Split(',')) {
                AddNewTokenArc(entity.Interval, logProb, GetTokenId(subValue, true), GetSynonymFlag(entity.Type));
            }
        }
    }
}

void TPreprocessedSample::AddNewTokenArc(const NNlu::TInterval& interval, float logProb, TTokenId token, ESynonymFlags synonymFlag) {
    if (token == TOKEN_UNKNOWN) {
        return;
    }

    TParserVertex& vertex = Vertices[interval.Begin];
    vertex.TokenArcs.push_back({
        .LogProb = logProb,
        .To = ToInt(interval.End),
        .Token = token,
        .SynonymFlag = synonymFlag,
    });
    GrammarData.OptimizationInfo.FirstWordToElements.AddTokenUncommonElementsToSet(token, &vertex.PromisingByFirstWord);
    GrammarData.OptimizationInfo.SpecificWordToElements.AddTokenUncommonElementsToSet(token, &vertex.PromisingBySpecificWord);
    NeedUpdatePromisingElements = true;
}

void TPreprocessedSample::UpdatePromisingElements() {
    if (!NeedUpdatePromisingElements) {
        return;
    }
    TDynBitMap promisingBySpecificWord;
    for (TParserVertex& vertex : Reversed(Vertices)) {
        promisingBySpecificWord |= vertex.PromisingBySpecificWord;
        vertex.PromisingElements = promisingBySpecificWord;
        vertex.PromisingElements &= vertex.PromisingByFirstWord;
        vertex.PromisingElements |= vertex.PromisingByEntity;
    }
    PromisingRootElements = promisingBySpecificWord;
    NeedUpdatePromisingElements = false;
}

TVector<const TEntity*> TPreprocessedSample::GetEntitiesOfType(TStringBuf type) const {
    TVector<const TEntity*> result;
    if (const TVector<size_t>* indexes = EntityTypeToIndexes.FindPtr(type)) {
        for (const size_t index : *indexes) {
            result.push_back(&Sample->GetEntities()[index]);
        }
    }
    return result;
}

void TPreprocessedSample::Dump(IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    Y_ENSURE(Sample);

    *log << indent << "TPreprocessedSample:" << Endl;
    Sample->Dump(log, indent + "  ");

    *log << indent << "  Arcs:" << Endl;
    *log << indent << "    Tokens:" << Endl;
    for (const auto& [from, vertex] : Enumerate(Vertices)) {
        for (const TParserTokenArc& arc : vertex.TokenArcs) {
            *log << indent << "      " << from << "->" << arc.To
                << ". Token: " << Hex(arc.Token)
//                << ", SynonymFlag: " << (!arc.SynonymFlag ? 0 : 1)
                << ", LogProb: " << arc.LogProb
                << Endl;
        }
    }
    *log << indent << "    Entities:" << Endl;
    for (const auto& [from, vertex] : Enumerate(Vertices)) {
        for (const auto& [elementId, arcs] : vertex.EntityArcs) {
            for (const TParserEntityArc& arc : arcs) {
                *log << indent << "      " << from << "->" << arc.To
                    << ". ElementId: " << elementId
                    << ", LogProb: " << arc.LogProb
                    << Endl;
            }
        }
    }

    *log << indent << "  Promising elements:" << Endl;
    TDynBitMap common = Vertices[0].PromisingElements;
    for (const TParserVertex& vertex : Vertices) {
        common &= vertex.PromisingElements;
    }
    *log << indent << "    from any position: " << JoinSeq(", ", BitSetElements(common)) << Endl;
    size_t counter = 0;
    for (const TParserVertex& vertex : Vertices) {
        *log << indent << "    from position " << LeftPad(counter, 3) << ": ";
        *log << JoinSeq(", ", BitSetElements(vertex.PromisingElements - common)) << Endl;
        counter++;
    }
}

// ~~~~ TMultiPreprocessedSample ~~~~

TMultiPreprocessedSample::TRef TMultiPreprocessedSample::Copy() {
    const TSample::TRef sample = Sample->Copy();
    TVector<TPreprocessedSample::TRef> preprocessedSamples;
    for (const TPreprocessedSample::TRef& preprocessedSample : PreprocessedSamples) {
        preprocessedSamples.push_back(preprocessedSample->Copy(sample));
    }
    return new TMultiPreprocessedSample(MultiGrammar, sample, preprocessedSamples, ParsedEntities);
}

TMultiPreprocessedSample::TMultiPreprocessedSample(
        const TMultiGrammar::TConstRef& multiGrammar,
        const TSample::TRef& sample,
        const TVector<TPreprocessedSample::TRef>& preprocessedSamples,
        const THashSet<TString>& parsedEntities)
    : MultiGrammar(multiGrammar)
    , Sample(sample)
    , PreprocessedSamples(preprocessedSamples)
    , ParsedEntities(parsedEntities)
{
}

// static
TMultiPreprocessedSample::TRef TMultiPreprocessedSample::Create(
    const TMultiGrammar::TConstRef& multiGrammar, const TSample::TRef& sample)
{
    return new TMultiPreprocessedSample(multiGrammar, sample);
}

// static
TMultiPreprocessedSample::TRef TMultiPreprocessedSample::Create(
    const TGrammar::TConstRef& grammar, const TSample::TRef& sample)
{
    return new TMultiPreprocessedSample(TMultiGrammar::Create({grammar}), sample);
}

TMultiPreprocessedSample::TMultiPreprocessedSample(const TMultiGrammar::TConstRef& multiGrammar,
        const TSample::TRef& sample)
    : MultiGrammar(multiGrammar)
    , Sample(sample)
{
    for (const TMultiGrammar::TGrammarInfo& grammar : MultiGrammar->GetGrammars()) {
        PreprocessedSamples.push_back(TPreprocessedSample::Create(grammar.Grammar, Sample));
    }
}

bool TMultiPreprocessedSample::WasEntityParsed(TStringBuf name) const {
    return ParsedEntities.contains(name);
}

bool TMultiPreprocessedSample::SetEntityWasParsed(TStringBuf name) {
    const auto& [it, isNew] = ParsedEntities.emplace(name);
    return isNew;
}

void TMultiPreprocessedSample::AddFoundEntitiesToSample(const TVector<TEntity>& entities) {
    Sample->AddEntitiesOnTokens(entities);
    for (const TPreprocessedSample::TRef& preprocessedSample : PreprocessedSamples) {
        preprocessedSample->ProcessNewEntities();
    }
}

} // namespace NGranet
