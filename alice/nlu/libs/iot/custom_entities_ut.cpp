#include "custom_entities.h"

#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <alice/nlu/libs/tokenization/tokenizer.h>

#include <alice/library/iot/utils.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>


THashMap<TString, NAlice::TIoTUserInfo> ReadIoTConfigs() {
    const auto parsedResource = NSc::TValue::FromJson(NResource::Find("iot_configs.json"));
    THashMap<TString, NAlice::TIoTUserInfo> result;
    for (const auto& [key, value] : parsedResource.GetDict()) {
        result[key] = NAlice::NIot::IoTFromIoTValue(value);
    }
    return result;
}

const NAlice::TIoTUserInfo& GetIoTConfig(ELanguage language) {
    static const auto parsedResource = ReadIoTConfigs();
    return parsedResource.at(NameByLanguage(language));
}

NSc::TValue ReadTests(ELanguage lang, const TStringBuf category) {
    static auto tests = NSc::TValue::FromJson(NResource::Find("tests.json"));
    return tests[NameByLanguage(lang)][category];
}

void SortEntities(TVector<NGranet::TEntity>& entities) {
    SortBy(entities, [](const NGranet::TEntity& entity) {
        return std::make_tuple(entity.Interval.Begin, entity.Interval.End, entity.Type, entity.Value, entity.Quality);
    });
}

TVector<NGranet::TEntity> ParseExpectedEntities(const NSc::TValue& expectedEntities) {
    TVector<NGranet::TEntity> result;

    for (const auto& entity : expectedEntities.GetArray()) {
        result.push_back(NGranet::TEntity{
            .Interval = NNlu::TInterval{
                .Begin = entity["interval"]["begin"],
                .End = entity["interval"]["end"]
            },
            .Type = ToString(entity["type"].GetString()),
            .Value = ToString(entity["value"].GetString()),
            .LogProbability = entity["log_probability"].GetNumber(),
            .Quality = entity["quality"].GetNumber()
        });
    }

    SortEntities(result);
    return result;
}

struct TTestCase {
    TString Request;
    TVector<NGranet::TEntity> ExpectedEntities;

    explicit TTestCase(const NSc::TValue& testCaseValue)
        : Request(testCaseValue["request"].GetString())
        , ExpectedEntities(ParseExpectedEntities(testCaseValue["expected"]))
    {
    }
};

void PrintEntities(const TVector<NGranet::TEntity>& entities) {
    for (const auto& entity : entities) {
        Cerr << "\t{"
             << " interval=" << entity.Interval
             << " type=\"" << entity.Type << "\""
             << " value=\"" << entity.Value << "\""
             << " logProb=" << entity.LogProbability
             << " quality=" << entity.Quality
             << " }\n";
    }
}

class TTestCaseChecker {
public:
    TTestCaseChecker(ELanguage lang, const TStringBuf category)
        : Category_(category)
        , Language_(lang)
        , EntityStrings_(NAlice::NIot::ParseIoTConfig(GetIoTConfig(lang), lang))
    {
        const auto testCasesValues = ReadTests(Language_, category);
        for (const auto& testCaseValue : testCasesValues.GetArray()) {
            TestCases_.emplace_back(testCaseValue);
        }
    }

    void Run() {
        TVector<TString> failedTestCaseRequests;
        for (const auto& testCase : TestCases_) {
            auto actualEntities = FindActualEntities(testCase.Request);
            SortEntities(actualEntities);
            if (actualEntities != testCase.ExpectedEntities) {
                Cerr << "TestCase with request=" << testCase.Request << " has failed.\n";
                Cerr << "Expected:\n";
                PrintEntities(testCase.ExpectedEntities);
                Cerr << "---\nActual:\n";
                PrintEntities(actualEntities);
                Cerr << "---\n---\n\n\n";
                failedTestCaseRequests.push_back(testCase.Request);
            }
        }

        if (!failedTestCaseRequests.empty()) {
            ythrow yexception() << "Failed for [" << JoinSeq(",", failedTestCaseRequests) << "]";
        }
    }

private:
    TVector<NGranet::TEntity> FindActualEntities(const TStringBuf request) {
        const auto input = ::NNlu::TRequestNormalizer::Normalize(Language_, request);
        const auto tokens = NNlu::TSmartTokenizer(input, Language_).GetNormalizedTokens();
        return NAlice::NIot::FindIoTEntities(EntityStrings_, tokens, Language_);
    }

private:
    TString Category_;
    ELanguage Language_;
    TVector<TTestCase> TestCases_;
    TVector<NAlice::NNlu::TEntityString> EntityStrings_;
};


Y_UNIT_TEST_SUITE(FindIoTEntitiesRus) {
    Y_UNIT_TEST(Capabilities) {
        TTestCaseChecker(LANG_RUS, "capabilities").Run();
    }

    Y_UNIT_TEST(Colors) {
        TTestCaseChecker(LANG_RUS, "colors").Run();
    }

    Y_UNIT_TEST(Devices) {
        TTestCaseChecker(LANG_RUS, "devices").Run();
    }

    Y_UNIT_TEST(Rooms) {
        TTestCaseChecker(LANG_RUS, "rooms").Run();
    }

    Y_UNIT_TEST(Scenarios) {
        TTestCaseChecker(LANG_RUS, "scenarios").Run();
    }

    Y_UNIT_TEST(Static) {
        TTestCaseChecker(LANG_RUS, "static").Run();
    }
}

Y_UNIT_TEST_SUITE(FindIoTEntitiesAra) {
    Y_UNIT_TEST(Colors) {
        TTestCaseChecker(LANG_ARA, "colors").Run();
    }

    Y_UNIT_TEST(Devices) {
        TTestCaseChecker(LANG_ARA, "devices").Run();
    }

    Y_UNIT_TEST(Groups) {
        TTestCaseChecker(LANG_ARA, "groups").Run();
    }

    Y_UNIT_TEST(Rooms) {
        TTestCaseChecker(LANG_ARA, "rooms").Run();
    }

    Y_UNIT_TEST(Scenarios) {
        TTestCaseChecker(LANG_ARA, "scenarios").Run();
    }

    Y_UNIT_TEST(Static) {
        TTestCaseChecker(LANG_ARA, "static").Run();
    }
}
