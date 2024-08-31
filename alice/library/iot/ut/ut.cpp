#include <alice/library/iot/demo_smart_home.h>
#include <alice/library/iot/entities.h>
#include <alice/library/iot/iot.h>
#include <alice/library/iot/structs.h>
#include <alice/library/iot/utils.h>

#include <alice/nlu/libs/iot/custom_entities.h>
#include <alice/nlu/libs/tokenization/tokenizer.h>

#include <alice/megamind/protos/common/iot.pb.h>

#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>


using namespace NAlice::NIot;
using NAlice::TIoTUserInfo;


constexpr TStringBuf DEMO_SMART_HOME_NAME = "DEMO";

NSc::TValue GetParsedTestsResource() {
    static NSc::TValue result = LoadAndParseResource("tests.json");
    return result;
}

TVector<TString> Tokenize(const TString& utterance, ELanguage language) {
    return ::NNlu::CreateTokenizer(::NNlu::ETokenizerType::SMART, utterance, language)->GetOriginalTokens();
}

TVector<NSc::TValue> SortHypotheses(const NSc::TValue& hypotheses) {
    TVector<NSc::TValue> result = {hypotheses.GetArray().begin(), hypotheses.GetArray().end()};
    SortBy(result, [](const NSc::TValue& hypothesis) { return hypothesis.ToJsonSafe().size(); });
    return result;
}

void SortEntities(TVector<TRawEntity>& entities) {
    SortBy(entities, [](const TRawEntity& entity) {
        return std::make_tuple(entity.AsEntity().GetStart(), entity.AsEntity().GetEnd(), entity.AsEntity().GetTypeStr(), entity.AsEntity().GetValue());
    });
}

TVector<NAlice::NNlu::TEntityString> EmptyEntityStrings(ELanguage language) {
    return ParseIoTConfig({}, language);
}


class TSmartHomeEx {
public:
    TSmartHomeEx(const TStringBuf name, const TIoTUserInfo& ioTUserInfo, const ELanguage language)
        : Name_(ToString(name))
        , IoTUserInfo_(ioTUserInfo)
        , EntitiesStrings_(ParseIoTConfig(ioTUserInfo, language))
    {
        ClientDeviceIdToIndex_.emplace("", BuildIndex({{&IoTUserInfo_}}));
    }

    const TSmartHomeIndex& GetSmartHomeIndex(const TStringBuf clientDeviceId) const {
        if (!ClientDeviceIdToIndex_.contains(clientDeviceId)) {
            ClientDeviceIdToIndex_.emplace(clientDeviceId, BuildIndex({{&IoTUserInfo_}, ToString(clientDeviceId)}));
        }
        return ClientDeviceIdToIndex_[clientDeviceId];
    }

    const TIoTUserInfo& IoTUserInfo() const {
        return IoTUserInfo_;
    }

    const TVector<NAlice::NNlu::TEntityString> EntitiesStrings() const {
        return EntitiesStrings_;
    }

    const TString Name() const {
        return Name_;
    }

private:
    const TString Name_;
    const TIoTUserInfo IoTUserInfo_;
    const TVector<NAlice::NNlu::TEntityString> EntitiesStrings_;
    mutable THashMap<TString, TSmartHomeIndex> ClientDeviceIdToIndex_;
};


class TSmartHomeStorage {
public:
    static const TSmartHomeEx& GetSmartHome(ELanguage language, const TStringBuf smartHomeName) {
        return Singleton<TSmartHomeStorage>()->Storage_.at(language).at(smartHomeName);
    }

private:
    TSmartHomeStorage() {
        const auto tests = GetParsedTestsResource();
        for (const auto& [langStr, testsDataForLang] : tests.GetDict()) {
            const ELanguage language = LanguageByName(langStr);
            for (const auto& [smartHomeName, smartHomeValue] : testsDataForLang["smart_homes"].GetDict()) {
                Y_ASSERT(smartHomeName != DEMO_SMART_HOME_NAME);
                LoadSmartHome(language, smartHomeName, smartHomeValue);
            }
            Storage_[language].emplace(
                DEMO_SMART_HOME_NAME,
                TSmartHomeEx(DEMO_SMART_HOME_NAME, *GetDemoSmartHome(language), language)
            );
        }
    }

    void LoadSmartHome(ELanguage language, const TStringBuf smartHomeName, NSc::TValue smartHomeValue) {
        Storage_[language].emplace(smartHomeName, TSmartHomeEx(smartHomeName, IoTFromIoTValue(smartHomeValue), language));
    }

private:
    THashMap<ELanguage, THashMap<TString, TSmartHomeEx>> Storage_;

    Y_DECLARE_SINGLETON_FRIEND();
};


class TEntityFinder {
public:
    TIoTEntitiesInfo FindEntities(const TNluInput nluInput, ELanguage language, const TSmartHomeEx& smartHome) {
        NluInput_ = &nluInput;
        Language_ = language;
        IsDemo_ = smartHome.Name() == DEMO_SMART_HOME_NAME;
        SmartHome_ = &smartHome;

        auto result = FindEntitiesWithAliceCustomEntities();
        AddFstAndNonsenseEntities(result.Entities);
        SortEntities(result.Entities);

        return result;
    }

private:
    TIoTEntitiesInfo FindEntitiesWithAliceCustomEntities() {
        const auto tokens = Tokenize(NluInput_->Utterance, Language_);
        TVector<TRawEntity> foundEntities;
        if (IsDemo_) {
            const auto foundGranetEntities = FindIoTEntities(EmptyEntityStrings(Language_), tokens, Language_);
            foundEntities = MakeRawEntities(foundGranetEntities, tokens);
        } else {
            const auto foundGranetEntities = FindIoTEntities(SmartHome_->EntitiesStrings(), tokens, Language_);
            foundEntities = MakeRawEntities(foundGranetEntities, tokens);
            FilterOutDemoEntities(foundEntities);
        }
        return {
            .Entities = foundEntities,
            .NormalizedUtterance = NluInput_->Utterance,
            .NTokens = tokens.ysize(),
        };
    }

    void FilterOutDemoEntities(TVector<TRawEntity>& entities) {
        EraseIf(entities, [](const auto& entity) {
            return entity.IsDemo();
        });
    }

    void AddFstAndNonsenseEntities(TVector<TRawEntity>& result) {
        const auto tokens = ::NNlu::CreateTokenizer(::NNlu::ETokenizerType::SMART, NluInput_->Utterance, LANG_RUS)->GetOriginalTokens();

        for (const auto& entity : ComputeFstEntities(tokens, Language_)) {
            result.push_back(entity);
        }

        for (const auto& entity : ComputeNonsenseEntities(*NluInput_, tokens, Language_)) {
            result.push_back(entity);
        }
    }

private:
    const TNluInput* NluInput_;
    ELanguage Language_;
    bool IsDemo_;
    const TSmartHomeEx* SmartHome_;
};


class THypothesisFinder {
public:
    TVector<NSc::TValue> FindHypotheses(const TNluInput& nluInput, ELanguage language, const TSmartHomeEx& smartHome,
                                        const TString& clientDeviceId, const TExpFlags& expFlags)
    {
        const auto foundEntities = TEntityFinder().FindEntities(nluInput, language, smartHome);

        auto hypotheses = MakeHypotheses(smartHome.GetSmartHomeIndex(clientDeviceId), foundEntities, language, expFlags);
        RemoveExtra(hypotheses);
        return SortHypotheses(hypotheses);
    }

private:
    void RemoveExtra(NSc::TValue& hypotheses) {
        for (auto& hypothesis : hypotheses.GetArrayMutable()) {
            hypothesis.Delete("nlg");
            hypothesis.Delete("raw_entities");
        }
    }
};


struct TTestCase {
    virtual int GetNumberOfSubTests() const {
        return 1;
    }
    bool Empty() const {
        return GetNumberOfSubTests() == 0;
    }
    virtual ~TTestCase() {}
};


struct THypothesisTestCase : public TTestCase {
    THypothesisTestCase(const NSc::TValue& rawHypothesisTestCase, ELanguage language)
        : Language(language)
        , NluInputs(CollectNluInputs(rawHypothesisTestCase))
        , ExpectedHypotheses(SortHypotheses(rawHypothesisTestCase["expected"]))
        , SmartHome(&TSmartHomeStorage::GetSmartHome(language, rawHypothesisTestCase["smart_home"].GetString()))
        , ClientDeviceId(rawHypothesisTestCase["client_device_id"].GetString())
        , ExpFlags(CollectExpFlags(rawHypothesisTestCase))
    {
    }

    const ELanguage Language;
    const TVector<TNluInput> NluInputs;
    const TVector<NSc::TValue> ExpectedHypotheses;
    const TSmartHomeEx* SmartHome;
    const TString ClientDeviceId;
    const TExpFlags ExpFlags;

    int GetNumberOfSubTests() const override {
        return NluInputs.ysize();
    }

private:
    TVector<TNluInput> CollectNluInputs(const NSc::TValue& rawHypothesisTestCase) {
        TVector<TNluInput> result;

        const auto& tokens = rawHypothesisTestCase["tokens"];
        const auto& utterances = rawHypothesisTestCase["utterances"];
        if (tokens.IsNull() && utterances.IsArray()) {
            for (const auto& utterance : utterances.GetArray()) {
                result.emplace_back(NormalizeWithFST(utterance, Language));
            }
        } else if (tokens.IsArray() && utterances.IsNull()) {
            auto maybeNluInput = NluInputFromJson(tokens);
            UNIT_ASSERT_C(maybeNluInput, "Got no nlu input");
            result.push_back(maybeNluInput.GetRef());
        } else if (tokens.IsArray() && utterances.IsArray()) {
            auto nluInput = NluInputFromJson(tokens);
            UNIT_ASSERT_C(nluInput, "Got no nlu input");
            for (const auto& utterance : utterances.GetArray()) {
                nluInput->Utterance = ToString(utterance.GetString());
                result.push_back(nluInput.GetRef());
            }
        }
        return result;
    }

    static TExpFlags CollectExpFlags(const NSc::TValue& rawHypothesisTestCase) {
        TExpFlags expFlags;
        for (const auto& exp : rawHypothesisTestCase["experiments"].GetArray()) {
            expFlags[exp] = Nothing();
        }
        return expFlags;
    }
};


struct TEntityTestCase : public TTestCase {
public:
    TEntityTestCase(const NSc::TValue& rawEntitiesTestCase, ELanguage language)
        : Language(language)
        , NluInput(NormalizeWithFST(rawEntitiesTestCase["utterance"].ForceString(), language))
        , SmartHome(&TSmartHomeStorage::GetSmartHome(language, rawEntitiesTestCase["smart_home"].GetString()))
        , ExpectedEntities(CollectExpectedEntities(rawEntitiesTestCase))
    {
    }

    const ELanguage Language;
    const TNluInput NluInput;
    const TSmartHomeEx* SmartHome;
    const TVector<TRawEntity> ExpectedEntities;

private:
    static TVector<TRawEntity> CollectExpectedEntities(const NSc::TValue& rawEntitiesTestCase) {
        TVector<TRawEntity> result;

        for (const auto& entityValue : rawEntitiesTestCase["expected"].GetArray()) {
            result.push_back(TRawEntity::FromValue(entityValue));
        }

        SortEntities(result);

        return result;
    }
};


struct TSmartHomeParsingTestCase : public TTestCase {
    TSmartHomeParsingTestCase(const NSc::TValue& smartHomeParsingTestCase, ELanguage language)
        : Language(language)
        , Name(smartHomeParsingTestCase["test_name"].GetString())
        , TestData(smartHomeParsingTestCase["smart_home"])
    {
    }

    const ELanguage Language;
    const TString Name;
    const NSc::TValue TestData;
};


class THypothesisTestCaseChecker {
public:
    // returns the number of passed subtests
    // OK if the return value equals to hypothesisTestCase.NluInput.size()
    int CheckTestCase(const THypothesisTestCase& hypothesisTestCase) {
        HypothesisTestCase_ = &hypothesisTestCase;
        Language_ = HypothesisTestCase_->Language;
        SuccessCounter_ = 0;

        for (const auto& nluInput : hypothesisTestCase.NluInputs) {
            CurrentNluInput_ = &nluInput;
            CheckSubTest();
        }

        return SuccessCounter_;
    }

private:
    void CheckSubTest() {
        FoundHypothesesForCurrentNluInput_ =
                THypothesisFinder().FindHypotheses(*CurrentNluInput_, Language_, *HypothesisTestCase_->SmartHome,
                                                   HypothesisTestCase_->ClientDeviceId, HypothesisTestCase_->ExpFlags);
        if (FoundHypothesesForCurrentNluInput_ == HypothesisTestCase_->ExpectedHypotheses) {
            ++SuccessCounter_;
        } else {
            PrintMessageOnFailedSubTest();
        }
    }

    void PrintMessageOnFailedSubTest() {
        Cerr << "For smart home configuration " << HypothesisTestCase_->SmartHome->Name()
             << " and utterance \"" << CurrentNluInput_->Utterance << "\" "
             << "expected to get" << Endl;
        PrintHypotheses(HypothesisTestCase_->ExpectedHypotheses);
        Cerr << "but got" << Endl;
        PrintHypotheses(FoundHypothesesForCurrentNluInput_);
        Cerr << "---" << Endl;
    }

    void PrintHypotheses(const TVector<NSc::TValue>& hypotheses) {
        for (const auto& hypothesis : hypotheses) {
            Cerr << hypothesis.ToJsonPretty() << Endl;
        }
    }

private:
    ELanguage Language_;
    int SuccessCounter_;
    const THypothesisTestCase* HypothesisTestCase_;

    const TNluInput* CurrentNluInput_;
    TVector<NSc::TValue> FoundHypothesesForCurrentNluInput_;
};


class TEntityTestCaseChecker {
public:
    bool CheckTestCase(const TEntityTestCase& entitiesTestCase) {
        EntitiesTestCase_ = &entitiesTestCase;

        FindEntities();
        return CheckEntities();
    }

private:
    void FindEntities() {
        FoundEntities_ = TEntityFinder().FindEntities(
            EntitiesTestCase_->NluInput,
            EntitiesTestCase_->Language,
            *EntitiesTestCase_->SmartHome
        ).Entities;
        SortEntities(FoundEntities_);
    }

    bool CheckEntities() {
        if (FoundEntities_ != EntitiesTestCase_->ExpectedEntities) {
            PrintMessageOnFail();
            return false;
        }
        return true;
    }

    void PrintMessageOnFail() {
        Cerr << "For smart home configuration " << EntitiesTestCase_->SmartHome->Name()
             << " and utterance \"" << EntitiesTestCase_->NluInput.Utterance << "\" "
             << "expected to get" << Endl;
        PrintVectorOfEntities(EntitiesTestCase_->ExpectedEntities);
        Cerr << "but got" << Endl;
        PrintVectorOfEntities(FoundEntities_);
        Cerr << "---" << Endl;
    }

    static void PrintVectorOfEntities(const TVector<TRawEntity>& rawEntities) {
        for (const auto& entity : rawEntities) {
            PrintEntity(entity.AsEntity());
        }
    }

    static void PrintEntity(const NAlice::NScenarios::TIotEntity& entity) {
        const auto& extra = entity.GetExtra();
        const TString extraString = TStringBuilder() << "(" << extra.GetIsCloseVariation(0) << " "
                                                     << extra.GetIsSynonym(0) << " " << extra.GetIsExact() << ")";
        const TString extraStringSpaced = extraString + " ";

        const TString intervalSpaced = TStringBuilder() << "(" << entity.GetStart() << ", " << entity.GetEnd() << ") ";

        const TString typeStr = TStringBuilder() << "type=\"" << entity.GetTypeStr() << "\"";
        const TString typeStrSpaced = typeStr + TString(50 - UTF8ToWide(typeStr).size(), ' ');

        const TString text = TStringBuilder() << "text=\"" << entity.GetText() << "\"";
        const TString textSpaced = text + TString(30 - UTF8ToWide(text).size(), ' ');

        const TString value = TStringBuilder() << "value=\"" << entity.GetValue() << "\"";

        Cerr << intervalSpaced << typeStrSpaced << textSpaced << extraStringSpaced << value << Endl;
    }

private:
    const TEntityTestCase* EntitiesTestCase_;
    TVector<TRawEntity> FoundEntities_;

};


class TSmartHomeParsingTestCaseChecker {
public:
    bool CheckTestCase(const TSmartHomeParsingTestCase& testCase) {
        ParseIoTConfig(IoTFromIoTValue(testCase.TestData), testCase.Language);
        return true;
    }
};


template <class TTestCase, class TTestCaseChecker>
class TTestCasesRunner {
public:
    TTestCasesRunner(const TStringBuf name, TTestCaseChecker testCaseChecker, ELanguage language, const TString& section)
        : Name_(name)
        , Language_(language)
        , Section_(section)
        , TestCaseChecker_(testCaseChecker)
    {
        CollectTestCases();
    }

    void Run() {
        int totalSubtestsCount = 0, passedSubtestsCount = 0;

        for (const auto& testCase : TestCases_) {
            if (testCase.Empty()) {
                ythrow yexception() << "Found an empty test, that checks nothing. Remove or fix it." << Endl;
            }

            passedSubtestsCount += static_cast<int>(TestCaseChecker_.CheckTestCase(testCase));
            totalSubtestsCount += testCase.GetNumberOfSubTests();
        }

        if (passedSubtestsCount != totalSubtestsCount) {
            ythrow yexception() << totalSubtestsCount - passedSubtestsCount << " out of "<< totalSubtestsCount
                                << " subtests have failed for " << GetTestCollectionIdentifier() << Endl;
        }
        Cerr << "All " << LeftAdjust(TStringBuilder() << TestCases_.ysize(), 3) << " "
             << LeftAdjust(TStringBuilder() << "(" << totalSubtestsCount << ")", 6) << " "
             << " tests(subtests) are good for "<< GetTestCollectionIdentifier() << Endl;
    }

private:
    void CollectTestCases() {
        TestCases_.clear();

        const auto rawTests = GetParsedTestsResource()[NameByLanguage(Language_)][Section_];
        for (const auto& rawTestValue : rawTests.GetArray()) {
            TestCases_.emplace_back(rawTestValue, Language_);
        }
    }

    TString GetTestCollectionIdentifier() const {
        return TStringBuilder() << "[" << NameByLanguage(Language_) << ", " << Section_ << ", " << Name_ << "]";
    }

    TString LeftAdjust(TString string, int minWidth) {
        int nSpaces = Max<int>(minWidth - static_cast<int>(string.size()), 0);
        return string + TString(nSpaces, ' ');
    }

private:
    const TString Name_;
    const ELanguage Language_;
    const TString Section_;
    TTestCaseChecker TestCaseChecker_;
    TVector<TTestCase> TestCases_;
};


Y_UNIT_TEST_SUITE(IotTests) {
    Y_UNIT_TEST(RusHypothesesWithAliceCustomEntities) {
        TTestCasesRunner<THypothesisTestCase, THypothesisTestCaseChecker>(
            Name_,
            THypothesisTestCaseChecker(),
            LANG_RUS,
            "test_hypotheses"
        ).Run();
    }

    Y_UNIT_TEST(RusEntities) {
        TTestCasesRunner<TEntityTestCase, TEntityTestCaseChecker>(
            Name_,
            TEntityTestCaseChecker(),
            LANG_RUS,
            "test_entities"
        ).Run();
    }

    Y_UNIT_TEST(RusParsingSmartHomes) {
        TTestCasesRunner<TSmartHomeParsingTestCase, TSmartHomeParsingTestCaseChecker>(
            Name_,
            TSmartHomeParsingTestCaseChecker(),
            LANG_RUS,
            "test_smart_homes_parsing"
        ).Run();
    }

    Y_UNIT_TEST(AraHypotheses) {
        TTestCasesRunner<THypothesisTestCase, THypothesisTestCaseChecker>(
            Name_,
            THypothesisTestCaseChecker(),
            LANG_ARA,
            "test_hypotheses"
        ).Run();
    }

    Y_UNIT_TEST(AraEntities) {
        TTestCasesRunner<TEntityTestCase, TEntityTestCaseChecker>(
            Name_,
            TEntityTestCaseChecker(),
            LANG_ARA,
            "test_entities"
        ).Run();
    }
}
