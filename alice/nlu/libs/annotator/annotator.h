#pragma once

#include <library/cpp/text_processing/tokenizer/tokenizer.h>

#include <library/cpp/containers/comptrie/pattern_searcher.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/hash_set.h>
#include <util/generic/hash.h>


namespace NAnnotator {
    using TClassId = ui32;
    using TPatternId = ui32;


    class TAnnotatorBuilder {
    public:
        explicit TAnnotatorBuilder(bool lemmatizing = false);

        void AddPattern(const TStringBuf pattern);
        TClassId FinishClass(); // returns finished class id

        size_t EstimateSize() const;
        void Save(IOutputStream* output) const;

    private:
        using TPatternSearcherBuilder = TCompactPatternSearcherBuilder<char, TPatternId>;

        bool Lemmatizing;
        TPatternSearcherBuilder PatternSearcherBuilder;
        TVector<TVector<TClassId>> ClassesListsByPattern;
        TVector<size_t> PatternLengths;
        NTextProcessing::NTokenizer::TTokenizer Tokenizer;
        TClassId CurrentClassId;

    private:
        size_t EstimatePatternsInfosSize() const;
        void SavePatternsInfos(IOutputStream* output) const;
    };

    struct TOccurencePosition {
        // Tokens positions as a semi-opened interval [StartToken, EndToken).
        size_t StartToken;
        size_t EndToken;

        TOccurencePosition(size_t startToken, size_t endToken)
            : StartToken(startToken)
            , EndToken(endToken)
        {
        }

        bool operator<(const TOccurencePosition& other) const;
        bool operator==(const TOccurencePosition& other) const;
        bool operator!=(const TOccurencePosition& other) const;
    };


    using TAnnotations = THashMap<TClassId, TVector<TOccurencePosition>>;

    class TAnnotator {
    public:
        explicit TAnnotator(const TString& fileName);
        explicit TAnnotator(const TBlob& blob);

        TAnnotations Annotate(const TStringBuf text) const;

    private:
        class TPatternsInfos;
        using TPatternSearcher = TCompactPatternSearcher<char, TPatternId>;

        TBlob DataHolder;
        THolder<TPatternSearcher> PatternSearcher;
        THolder<TPatternsInfos> PatternsInfos;
        NTextProcessing::NTokenizer::TTokenizer Tokenizer;
    };


    struct TPatternMeta {
        ui32 Length;
        ui64 ClassesListOffset;

        TPatternMeta(ui32 length, ui64 classesListOffset)
            : Length(length)
            , ClassesListOffset(classesListOffset)
        {
        }
    };


    class TAnnotator::TPatternsInfos {
    public:
        explicit TPatternsInfos(const TBlob& blob);

        void AddAnnotations(const TVector<size_t>& tokensPositions,
                            const TPatternId patternId,
                            const size_t matchEndPos,
                            TAnnotations* annotations) const;

    private:
        TBlob DataHolder;
        size_t PatternsNum;
        const TPatternMeta* MetaInfos;
        const TClassId* ClassesLists;
    };

}; // NAnnotator
