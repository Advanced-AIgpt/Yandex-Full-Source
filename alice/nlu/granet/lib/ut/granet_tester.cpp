#include "granet_tester.h"
#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <util/string/strip.h>

namespace NGranet {

// ~~~~ TGranetTester ~~~~

TGranetTester::TGranetTester(TStringBuf str, const TGranetDomain& domain,
        const NCompiler::TCompilerOptions& options)
    : Grammar(NCompiler::TCompiler(options).CompileFromString(NAlice::NUtUtils::NormalizeText(str), domain))
{
    CreateSerializedGrammar();
}

TGranetTester::TGranetTester(TGrammar::TConstRef grammar)
    : Grammar(std::move(grammar))
{
    CreateSerializedGrammar();
}

void TGranetTester::CreateSerializedGrammar() {
    TBufferStream buffer;
    ::Save(&buffer, *Grammar);
    SerializedGrammar = TGrammar::LoadFromStream(&buffer);
}

void TGranetTester::EnableLog(bool isVerbose) {
    SetLog(&Cerr, isVerbose);
}

void TGranetTester::SetLogFile(const TFsPath& path, bool isVerbose) {
    LogFile = MakeHolder<TFileOutput>(path);
    SetLog(LogFile.Get(), isVerbose);
}

void TGranetTester::SetLog(IOutputStream* log, bool isVerbose) {
    Y_ENSURE(log);
    Log = log;
    IsLogVerbose = isVerbose;
    Grammar->Dump(Log);
}

void TGranetTester::AddEntity(TString type, TString text, TString value, double logprob) {
    TEntity entity;
    entity.Type = std::move(type);
    entity.Value = std::move(value);
    entity.LogProbability = logprob;
    Entities.push_back({std::move(text), entity});
}

void TGranetTester::TestHasMatch(const TVector<std::pair<TString, bool>>& testData) const {
    for (const auto& [line, expectedHasMatch] : testData) {
        TestHasMatch(line, expectedHasMatch);
    }
}

void TGranetTester::TestHasMatch(TStringBuf line, bool expectedHasMatch) const {
    bool hasMatch = false;
    for (const TParserFormResult::TConstRef& result : ParseAllForms(line)) {
        hasMatch = hasMatch || result->IsPositive();
    }
    UNIT_ASSERT_C(hasMatch == expectedHasMatch, "for line " << Cite(line));
    TestSerialization(line);
}

void TGranetTester::TestMatchedForms(const TVector<std::pair<TString, TString>>& testData) const {
    for (const auto& [line, expectedHasMatch] : testData) {
        TestMatchedForms(line, expectedHasMatch);
    }
}

static TString PrintFormSet(const TVector<TParserFormResult::TConstRef>& forms) {
    TSet<TString> set;
    for (const TParserFormResult::TConstRef& form : forms) {
        if (form->IsPositive()) {
            set.insert(form->GetName());
        }
    }
    return JoinSeq(",", set);
}

void TGranetTester::TestMatchedForms(TStringBuf line, TStringBuf expectedForms) const {
    const TString actualForms = PrintFormSet(ParseAllForms(line));
    UNIT_ASSERT_C(actualForms == expectedForms, "for line " << Cite(line));
    TestSerialization(line);
}

static size_t ToVariantCountDescrption(const TVector<TParserFormResult::TConstRef>& forms) {
    size_t count = 0;
    for (const TParserFormResult::TConstRef& form : forms) {
        count += form->GetVariants().size();
    }
    return count;
}

void TGranetTester::TestVariantCount(TStringBuf line, size_t expectedCount) const {
    UNIT_ASSERT_C(ToVariantCountDescrption(ParseAllForms(line)) == expectedCount, "for line " << Cite(line));
    TestSerialization(line);
}

static TVector<TString> PrintForCompareSlotData(const TVector<TParserFormResult::TConstRef>& forms) {
    TVector<TString> result;
    for (const TParserFormResult::TConstRef& form : forms) {
        if (!form->IsPositive()) {
            continue;
        }
        for (const TResultSlot& slot : form->GetBestVariant()->Slots) {
            TVector<TString> values;
            for (const TResultSlotValue& value : slot.Data) {
                values.push_back(value.Type + "/" + value.Value);
            }
            result.push_back(form->GetName() + "/" + slot.Name + ": [" + JoinSeq(", ", values) + "]");
        }
    }
    return result;
}

void TGranetTester::TestSlotData(TStringBuf line, const TVector<TString>& expectedSlots) const {
    NAlice::NUtUtils::TestEqualSeq(line, expectedSlots, PrintForCompareSlotData(ParseAllForms(line)));
}

void TGranetTester::TestTagger(TStringBuf formName, bool expectedIsPositive, TStringBuf expectedMarkup, bool compareSlotsByTop) const {
    const TSampleMarkup expected = ReadSampleMarkup(expectedIsPositive, expectedMarkup);
    TParserFormResult::TConstRef result = CreateParser(CreateSample(expected.Text))->ParseForm(formName);
    const TSampleMarkup actual = result->ToMarkup();

    if (expected.CheckResult(actual, compareSlotsByTop)) {
        return;
    }
    TStringStream out;
    out << "Error:" << Endl;
    out << "  Form:     " << formName << Endl;
    out << "  Sample:   " << expected.Text << Endl;
    out << "  Expected: " << expected.PrintForReport(SPO_NEED_ALL) << Endl;
    out << "  Actual:   " << actual.PrintForReport(SPO_NEED_ALL) << Endl;
    result->Dump(&out, "  ");
    UNIT_FAIL(out.Str());
}

void TGranetTester::TestEntityFinder(const TVector<TString>& expectedMarkups, bool compareSlotsByTop) const {
    for (const TString& markup : expectedMarkups) {
        TestEntityFinder("", ReadSampleMarkup(true, markup), compareSlotsByTop);
    }
}

void TGranetTester::TestEntityFinder(TStringBuf entity, const TSampleMarkup& expected, bool compareSlotsByTop) const {
    TSample::TRef sample = CreateSample(expected.Text);
    CreateParser(sample)->ParseEntities();
    const TSampleMarkup actual = sample->GetEntitiesAsMarkup(entity, true);

    if (expected.CheckResult(actual, compareSlotsByTop)) {
        return;
    }
    TStringStream out;
    out << "Error:" << Endl;
    out << "  Sample:   " << expected.Text << Endl;
    out << "  Expected: " << expected.PrintMarkup(SPO_NEED_ALL) << Endl;
    out << "  Actual:   " << actual.PrintMarkup(SPO_NEED_ALL) << Endl;
    UNIT_FAIL(out.Str());
}

void TGranetTester::TestEntityFinderVariants(TStringBuf entity, TStringBuf text, const TVector<TString>& variantMarkups, bool compareSlotsByTop) const {
    TSampleMarkup expected = ReadSampleMarkup(true, text);
    for (const TString& markupString : variantMarkups) {
        const TSampleMarkup markup = ReadSampleMarkup(true, markupString);
        Y_ENSURE(markup.Text == expected.Text);
        Extend(markup.Slots, &expected.Slots);
    }
    Sort(expected.Slots);
    TestEntityFinder(entity, expected, compareSlotsByTop);
}

void TGranetTester::TestSerialization(TStringBuf line) const {
    TSample::TRef sample = CreateSample(line);
    TVector<TParserFormResult::TConstRef> forms1 = ParseSample(Grammar, sample);
    TVector<TParserFormResult::TConstRef> forms2 = ParseSample(SerializedGrammar, sample);
    UNIT_ASSERT_C(PrintForCompareSlotData(forms1) == PrintForCompareSlotData(forms2), "for line " << Cite(line));
}

TVector<TParserFormResult::TConstRef> TGranetTester::ParseAllForms(TStringBuf line) const {
    return CreateParser(CreateSample(line))->ParseForms();
}

TSample::TRef TGranetTester::CreateSample(TStringBuf line) const {
    TSample::TRef sample = TSample::Create(line, Grammar->GetLanguage());

    // Find and write entities
    for (const TEntitySearchInfo& info : Entities) {
        for (size_t pos = line.find(info.Text); pos != TString::npos; pos = line.find(info.Text, pos + 1)) {
            TEntity entity = info.Entity;
            entity.Interval = {pos, pos + info.Text.length()};
            sample->AddEntityOnText(entity);
        }
    }
    return sample;
}

TMultiParser::TRef TGranetTester::CreateParser(const TSample::TRef& sample) const {
    TMultiParser::TRef parser = TMultiParser::Create(Grammar, sample, true);
    parser->SetLog(Log, IsLogVerbose);
    return parser;
}

} // namespace NGranet
