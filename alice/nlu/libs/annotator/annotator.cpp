#include "annotator.h"

#include <util/memory/blob.h>


using namespace NTextProcessing::NTokenizer;

namespace {

TTokenizerOptions GetTokenizerOptions(bool lemmatizing) {
    TTokenizerOptions options;
    options.Lemmatizing = lemmatizing;
    options.SeparatorType = ESeparatorType::ByDelimiter;
    options.SkipEmpty = false;
    return options;
}

void PreprocessText(const TTokenizer& tokenizer,
                    const TStringBuf text,
                    TString* result,
                    TVector<size_t>* tokenByPosition = nullptr) {
    auto tokens = tokenizer.Tokenize(text);
    for (size_t token = 0; token < tokens.size(); ++token) {
        *result += tokens[token];
        if (tokenByPosition != nullptr) {
            tokenByPosition->insert(tokenByPosition->end(), tokens[token].size(), token);
        }

        if (token != tokens.size() - 1) {
            *result += " ";
        }
        if (tokenByPosition != nullptr) {
            tokenByPosition->push_back(token);
        }
    }
}

} // anonymous


namespace NAnnotator {

// class TAnnotatorBuilder

TAnnotatorBuilder::TAnnotatorBuilder(bool lemmatizing)
    : Lemmatizing(lemmatizing)
    , Tokenizer(GetTokenizerOptions(Lemmatizing))
    , CurrentClassId(0)
{
}

void TAnnotatorBuilder::AddPattern(const TStringBuf pattern) {
    TString preprocessedPattern;
    PreprocessText(Tokenizer, pattern, &preprocessedPattern);

    TPatternId patternId;
    if (!PatternSearcherBuilder.Find(preprocessedPattern, &patternId)) {
        patternId = ClassesListsByPattern.size();
        ClassesListsByPattern.emplace_back();
        PatternLengths.push_back(preprocessedPattern.size());
        PatternSearcherBuilder.Add(preprocessedPattern, patternId);
    }
    auto& classesList = ClassesListsByPattern[patternId];
    if (classesList.empty() || classesList.back() != CurrentClassId) {
        classesList.push_back(CurrentClassId);
    }
}

TClassId TAnnotatorBuilder::FinishClass() {
    return CurrentClassId++;
}

size_t TAnnotatorBuilder::EstimatePatternsInfosSize() const {
    size_t size = 0;
    size += sizeof(TPatternId); // TPatternId patternsNum
    size += sizeof(TPatternMeta) * (ClassesListsByPattern.size() + /*dummyMeta*/1);
    for (const auto& classesList : ClassesListsByPattern) {
        size += sizeof(TClassId) * classesList.size();
    }
    return size;
}

void TAnnotatorBuilder::SavePatternsInfos(IOutputStream* output) const {
    TPatternId patternsNum = ClassesListsByPattern.size();
    output->Write(&patternsNum, sizeof(TPatternId));

    size_t classesListsOffset = 0;
    for (size_t patternId = 0; patternId < ClassesListsByPattern.size(); ++patternId) {
        TPatternMeta meta(PatternLengths[patternId], classesListsOffset);
        output->Write(&meta, sizeof(TPatternMeta));

        classesListsOffset += ClassesListsByPattern[patternId].size();
    }

    TPatternMeta dummyMeta = {0, classesListsOffset};
    output->Write(&dummyMeta, sizeof(TPatternMeta));

    for (const auto& classesList: ClassesListsByPattern) {
        for (TClassId classId : classesList) {
            output->Write(&classId, sizeof(TClassId));
        }
    }
}

size_t TAnnotatorBuilder::EstimateSize() const {
    size_t size = 0;
    size += sizeof(char); // bool Lemmatizing (saved as char)
    size += sizeof(ui64); // ui64 patternSearcherSize
    size += PatternSearcherBuilder.MeasureByteSize();
    size += EstimatePatternsInfosSize();
    return size;
}

void TAnnotatorBuilder::Save(IOutputStream* output) const {
    const char lemmatizing = Lemmatizing;
    output->Write(&lemmatizing, sizeof(char));
    ui64 patternSearcherSize = PatternSearcherBuilder.MeasureByteSize();
    output->Write(&patternSearcherSize, sizeof(ui64));
    PatternSearcherBuilder.Save(*output);
    SavePatternsInfos(output);
}

// struct TOccurencePosition

bool TOccurencePosition::operator<(const TOccurencePosition& other) const {
    // TPatternSearcher follows this order
    return EndToken < other.EndToken || EndToken == other.EndToken && StartToken < other.StartToken;
}

bool TOccurencePosition::operator==(const TOccurencePosition& other) const {
    return StartToken == other.StartToken && EndToken == other.EndToken;
}

bool TOccurencePosition::operator!=(const TOccurencePosition& other) const {
    return !(*this == other);
}

// class TPatternsInfos

TAnnotator::TPatternsInfos::TPatternsInfos(const TBlob& blob)
    : DataHolder(blob)
{
    const char* data = reinterpret_cast<const char*>(blob.Data());
    PatternsNum = *(reinterpret_cast<const TPatternId*>(data));
    data += sizeof(TPatternId);

    MetaInfos = reinterpret_cast<const TPatternMeta*>(data);
    data += sizeof(TPatternMeta) * (PatternsNum + 1); // plus one dummy

    ClassesLists = reinterpret_cast<const TClassId*>(data);
}

void TAnnotator::TPatternsInfos::AddAnnotations(const TVector<size_t>& tokenByPosition,
                                                const TPatternId patternId,
                                                const size_t matchEndPos,
                                                TAnnotations* annotations) const {
    Y_ASSERT(patternId < PatternsNum);

    const TPatternMeta& meta = MetaInfos[patternId];
    size_t matchStartPos = matchEndPos - meta.Length;

    if (matchEndPos + 1 < tokenByPosition.size() && tokenByPosition[matchEndPos] == tokenByPosition[matchEndPos + 1]) {
        return;
    }

    if (matchStartPos > 0 && tokenByPosition[matchStartPos] == tokenByPosition[matchStartPos - 1]) {
        return;
    }

    const size_t classesListStartOffset = meta.ClassesListOffset;
    const size_t classesListEndOffset = MetaInfos[patternId + 1].ClassesListOffset;

    const size_t startToken = (tokenByPosition.size() > 0) ? tokenByPosition[matchStartPos] : 0;
    const size_t endToken = (tokenByPosition.size() > 0) ? tokenByPosition[matchEndPos] + 1 : 0;

    for (const TClassId* classId = ClassesLists + classesListStartOffset; classId < ClassesLists + classesListEndOffset; ++classId) {
        (*annotations)[*classId].emplace_back(startToken, endToken);
    }
}

// class TAnnotator

TAnnotator::TAnnotator(const TString& fileName)
    : TAnnotator(TBlob::PrechargedFromFile(fileName))
{
}

TAnnotator::TAnnotator(const TBlob& blob)
    : DataHolder(blob)
{
    const char* dataPos = DataHolder.AsCharPtr();
    const char* dataEnd = DataHolder.AsCharPtr() + DataHolder.Size();

    const bool lemmatizing = *reinterpret_cast<const char*>(dataPos);
    dataPos += sizeof(char);
    Tokenizer = TTokenizer(GetTokenizerOptions(lemmatizing));

    ui64 patternSearcherSize = *reinterpret_cast<const ui64*>(dataPos);
    dataPos += sizeof(ui64);

    Y_ASSERT(dataPos + patternSearcherSize <= dataEnd);
    PatternSearcher.Reset(new TPatternSearcher(TBlob::NoCopy(dataPos, patternSearcherSize)));
    dataPos += patternSearcherSize;
    PatternsInfos.Reset(new TPatternsInfos(TBlob::NoCopy(dataPos, dataEnd - dataPos)));
}

TAnnotations TAnnotator::Annotate(const TStringBuf text) const {
    TString preprocessedText;
    TVector<size_t> tokenByPosition;
    PreprocessText(Tokenizer, text, &preprocessedText, &tokenByPosition);

    TAnnotations annotations;

    auto matches = PatternSearcher->SearchMatches(preprocessedText);
    for (const auto& match : matches) {
        PatternsInfos->AddAnnotations(tokenByPosition, match.Data, match.End + 1, &annotations);
    }

    return annotations;
}

}; // NAnnotator
