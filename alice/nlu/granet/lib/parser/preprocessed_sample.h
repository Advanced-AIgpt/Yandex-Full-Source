#pragma once

#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/grammar/multi_grammar.h>
#include <alice/nlu/granet/lib/sample/sample.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <util/generic/bitmap.h>
#include <util/generic/ptr.h>

namespace NGranet {

// ~~~~ TParserTokenArc ~~~~

struct TParserTokenArc {
    float LogProb = 0;
    int To = 0;
    TTokenId Token = 0;
    ESynonymFlags SynonymFlag = 0;
};

// ~~~~ TParserEntityArc ~~~~

struct TParserEntityArc {
    float LogProb = 0;
    int To = 0;
    TRuleIndexes EntityIndexesInSample;
};

// ~~~~ TParserVertex ~~~~

struct TParserVertex {
    TVector<TParserTokenArc> TokenArcs;
    // Element -> entity occurrences started in that position which correspond to that element
    TMap<TElementId, TVector<TParserEntityArc>> EntityArcs;

    // PromisingElements[elementId] - element has a chance to be found with beginning in this position.
    TDynBitMap PromisingElements;

    // Components of PromisingElements.
    // Stored here to update PromisingElements after adding new entities to sample.
    TDynBitMap PromisingBySpecificWord;
    TDynBitMap PromisingByFirstWord;
    TDynBitMap PromisingByEntity;
};

// ~~~~ TPreprocessedSample ~~~~

class TPreprocessedSample : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TPreprocessedSample>;
    using TConstRef = TIntrusiveConstPtr<TPreprocessedSample>;

public:
    // Constructor
    static TRef Create(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample);

    TRef Copy(const TSample::TConstRef& sampleCopy);

    void ProcessNewEntities();

    const TGrammar::TConstRef& GetGrammar() const {
        return Grammar;
    }

    const TSample::TConstRef& GetSample() const {
        return Sample;
    }

    const TVector<TParserVertex>& GetVertices() const {
        return Vertices;
    }

    TVector<const TEntity*> GetEntitiesOfType(TStringBuf type) const;

    const TDynBitMap& GetPromisingRootElements() const {
        return PromisingRootElements;
    }

    void Dump(IOutputStream* log, const TString& indent = "") const;

private:
    TPreprocessedSample() = delete;
    TPreprocessedSample(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample);
    TPreprocessedSample(
        const TGrammar::TConstRef& grammar,
        const TSample::TConstRef& sample,
        const TVector<TParserVertex>& vertices,
        size_t sampleEntityCount,
        const THashMap<TString, TVector<size_t>>& entityTypeToIndexes,
        bool needUpdatePromisingElements,
        const TDynBitMap& promisingRootElements);

    void InitTokenArcs();
    TTokenId GetTokenId(TStringBuf word, bool isLemma) const;
    void InitPromisingElementsSubsets();
    void UpdatePromisingElements();
    void ProcessNewEntitiesInternal();
    void ProcessNewEntity(ui32 entityIndex);
    void AddNewTokenArc(const NNlu::TInterval& interval, float logProb, TTokenId token, ESynonymFlags synonymFlag);

private:
    TGrammar::TConstRef Grammar;
    const TGrammarData& GrammarData;
    TSample::TConstRef Sample;
    TVector<TParserVertex> Vertices;
    size_t SampleEntityCount = 0;
    THashMap<TString, TVector<size_t>> EntityTypeToIndexes;

    // Some of sets TParserVertex::PromisingElements are invalid and should be updated
    // in method UpdatePromisingElements.
    bool NeedUpdatePromisingElements = true;
    TDynBitMap PromisingRootElements;
};

// ~~~~ TMultiPreprocessedSample ~~~~

class TMultiPreprocessedSample : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TMultiPreprocessedSample>;
    using TConstRef = TIntrusiveConstPtr<TMultiPreprocessedSample>;

public:
    // Constructor
    static TRef Create(const TMultiGrammar::TConstRef& multiGrammar, const TSample::TRef& sample);
    static TRef Create(const TGrammar::TConstRef& grammar, const TSample::TRef& sample);

    TRef Copy();

    const TMultiGrammar::TConstRef& GetMultiGrammar() const {
        return MultiGrammar;
    }

    TSample::TConstRef GetSample() const {
        return Sample;
    }

    TPreprocessedSample::TConstRef GetPreprocessedSample(size_t grammarIndex) const {
        return PreprocessedSamples[grammarIndex];
    }

    bool WasEntityParsed(TStringBuf name) const;
    bool SetEntityWasParsed(TStringBuf name);
    void AddFoundEntitiesToSample(const TVector<TEntity>& entities);

    void Dump(IOutputStream* log, const TString& indent = "") const;

private:
    TMultiPreprocessedSample() = delete;
    TMultiPreprocessedSample(const TMultiGrammar::TConstRef& multiGrammar, const TSample::TRef& sample);
    TMultiPreprocessedSample(
        const TMultiGrammar::TConstRef& multiGrammar,
        const TSample::TRef& sample,
        const TVector<TPreprocessedSample::TRef>& preprocessedSamples,
        const THashSet<TString>& parsedEntities);

private:
    TMultiGrammar::TConstRef MultiGrammar;
    const TSample::TRef Sample;

    // Sample preprocessed for each grammar.
    // Array is parallel to array of grammars in MultiGrammar.
    TVector<TPreprocessedSample::TRef> PreprocessedSamples;

    THashSet<TString> ParsedEntities;
};

} // namespace NGranet
