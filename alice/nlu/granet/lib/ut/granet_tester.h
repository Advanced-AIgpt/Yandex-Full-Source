#pragma once

#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/granet.h>
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/parser/multi_parser.h>
#include <alice/nlu/granet/lib/parser/parser.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>

namespace NGranet {

// ~~~~ TGranetTester ~~~~

// Grammar testing tool for unit tests
class TGranetTester : public TMoveOnly {
public:
    explicit TGranetTester(TStringBuf str, const TGranetDomain& domain = {},
        const NCompiler::TCompilerOptions& options = {});
    explicit TGranetTester(TGrammar::TConstRef grammar);

    void EnableLog(bool isVerbose);
    void SetLogFile(const TFsPath& path, bool isVerbose);
    void SetLog(IOutputStream* log, bool isVerbose);

    void AddEntity(TString type, TString text, TString value = "", double logprob = 0);

    void TestHasMatch(const TVector<std::pair<TString, bool>>& testData) const;
    void TestHasMatch(TStringBuf line, bool expectedHasMatch) const;
    void TestMatchedForms(const TVector<std::pair<TString, TString>>& testData) const;
    void TestMatchedForms(TStringBuf line, TStringBuf expectedForms) const;
    void TestVariantCount(TStringBuf line, size_t expectedCount) const;
    void TestSlotData(TStringBuf line, const TVector<TString>& expectedSlots) const;
    void TestTagger(TStringBuf form, bool expectedIsPositive, TStringBuf expectedMarkup, bool compareSlotsByTop = true) const;

    void TestEntityFinder(const TVector<TString>& expectedMarkups, bool compareSlotsByTop = true) const;
    void TestEntityFinder(TStringBuf entity, const TSampleMarkup& expected, bool compareSlotsByTop = true) const;
    void TestEntityFinderVariants(TStringBuf entity, TStringBuf text, const TVector<TString>& variantMarkups, bool compareSlotsByTop = true) const;

private:
    void CreateSerializedGrammar();
    void TestSerialization(TStringBuf line) const;
    TVector<TParserFormResult::TConstRef> ParseAllForms(TStringBuf line) const;
    TSample::TRef CreateSample(TStringBuf line) const;
    TMultiParser::TRef CreateParser(const TSample::TRef& sample) const;

private:
    struct TEntitySearchInfo {
        TString Text;
        TEntity Entity;
    };

private:
    TGrammar::TConstRef Grammar;
    TGrammar::TConstRef SerializedGrammar;
    TVector<TEntitySearchInfo> Entities;
    IOutputStream* Log = nullptr;
    THolder<TFileOutput> LogFile;
    bool IsLogVerbose = false;
};

} // namespace NGranet
