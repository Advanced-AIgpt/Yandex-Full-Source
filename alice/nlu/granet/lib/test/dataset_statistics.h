#pragma once

#include <alice/nlu/granet/lib/parser/result.h>
#include <alice/nlu/granet/lib/test/sample_processor.h>
#include <util/generic/queue.h>
#include <util/folder/path.h>

namespace NGranet {

// ~~~~ TSlowestSamplesCollector ~~~~

class TSlowestSamplesCollector {
public:
    void Accumulate(const TString& text, double timeUs);
    void Write(const TFsPath& dir) const;

private:
    TPriorityQueue<std::pair<double, TString>> SlowestSamples;
};

// ~~~~ TElementDictionaryCollector ~~~~

class TElementDictionaryCollector {
public:
    void Accumulate(const TSampleMarkup& markup);
    void Write(const TFsPath& dir) const;

private:
    // Tag occurrence statistics
    struct TTagInfo {
        int Count = 0;
        TSet<TString> Examples;
    };

private:
    void WriteBrief(const TFsPath& dir) const;
    void WriteVerbose(const TFsPath& dir) const;

private:
    // Tag name -> (Tag text -> Tag text statistics)
    THashMap<TString, THashMap<TString, TTagInfo>> Dictionaries;
};

// ~~~~ TNegativeNgramsCollector ~~~~

class TNegativeNgramsCollector {
public:
    TNegativeNgramsCollector();
    void Accumulate(const TString& text);
    void Write(const TFsPath& dir) const;

private:
    static const size_t N = 3;
    TVector<THashMap<TString, size_t>> Ngrams;
};

// ~~~~ TParserBlockerCollector ~~~~

class TParserBlockerCollector {
public:
    void Accumulate(const TSampleProcessorResult& result);
    void Write(const TFsPath& dir) const;
    bool HasBlockers() const;
    void PrintSuggests(const TGrammar::TConstRef& grammar, IOutputStream* out, const TString& indent = "") const;

private:
    TVector<std::pair<TString, TVector<TString>>> GetSortedBlockers() const;

private:
    THashMap<TString, TVector<TString>> BlockerToSamples;
};

} // namespace NGranet
