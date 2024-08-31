#include "post.h"

#include <alice/megamind/library/classifiers/formulas/formulas_description.h>
#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>

#include <alice/megamind/library/common/defs.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/scenarios/config_registry/config_registry.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/session/session.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_responses.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/protos/scenarios/features/vins.pb.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/frame/builder.h>
#include <alice/library/json/json.h>
#include <alice/library/metrics/names.h>
#include <alice/library/music/defs.h>
#include <alice/library/unittest/mock_sensors.h>
#include <alice/library/video_common/defs.h>

#include <alice/protos/data/language/language.pb.h>

#include <kernel/factor_storage/factor_storage.h>
#include <kernel/formula_storage/formula_storage.h>
#include <kernel/matrixnet/relev_calcer.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/generic/hash.h>
#include <util/string/cast.h>

using namespace ::testing;
using namespace NAlice;

namespace {

const TVector<TSemanticFrame> NO_INPUT_FRAMES;

struct TScenarioInfo {
    TString Name;
    TVector<TSemanticFrame> InputFrames = NO_INPUT_FRAMES;
    ELang ResponseLanguage = ELang::L_UNK;
};

TVector<TScenarioResponse> MakeResponses(
    const TVector<TScenarioInfo>& scenarioInfos,
    const TFeatures& vinsFeatures = {}
) {
    static const bool SCENARIO_ACCEPTS_ANY_INPUT = true;
    TVector<TScenarioResponse> responses(Reserve(scenarioInfos.size()));
    for (const auto& info : scenarioInfos) {
        responses.emplace_back(TString{info.Name}, info.InputFrames, SCENARIO_ACCEPTS_ANY_INPUT);

        if (info.Name == MM_PROTO_VINS_SCENARIO) {
            responses.back().SetFeatures(vinsFeatures);
        }
    }
    return responses;
}


TVector<TScenarioResponse> MakeResponses(
    const THashMap<TStringBuf, TMaybe<ui32>>& scenarioToIndex,
    const TFeatures& vinsFeatures = {}
) {
    TVector<TScenarioInfo> infos(Reserve(scenarioToIndex.size()));
    for (auto&& [name, _] : scenarioToIndex) {
        infos.push_back({ToString(name)});
    }
    return MakeResponses(infos, vinsFeatures);
}

void CheckOrder(
    const TVector<TScenarioResponse>& responses,
    const THashMap<TStringBuf, TMaybe<ui32>>& scenarioToIndex
) {
    for (const auto& [scenarioName, index] : scenarioToIndex) {
        if (!index.Defined()) {
            continue;
        }
        UNIT_ASSERT_C(*index < responses.size(),
                      "Expected index " << *index << " for scenario " << scenarioName <<
                      " is not less than response list size: " << responses.size());
        UNIT_ASSERT_VALUES_EQUAL_C(responses[*index].GetScenarioName(), scenarioName,
                                   "Unexpected scenario at position " << *index << ": "
                                   "expected " << scenarioName << ", got " << responses[*index].GetScenarioName());
    }
    for (const ui32 index : xrange(responses.size())) {
        UNIT_ASSERT_C(scenarioToIndex.at(responses[index].GetScenarioName()).Defined(),
                      "Unexpected scenario response: " << responses[index].GetScenarioName());
    }
}


TFeatures VinsFeatures(TStringBuf intent, bool isContinuing = false, bool isPureGC = false, bool isIrrelevant = false,
                       bool restorePlayer = false, ui32 secondsSincePause = 0) {
    TFeatures features;
    auto& scenarioFeatures = *features.MutableScenarioFeatures();
    scenarioFeatures.SetIntent(TString{intent});
    scenarioFeatures.SetIsIrrelevant(isIrrelevant);
    scenarioFeatures.MutablePlayerFeatures()->SetRestorePlayer(restorePlayer);
    scenarioFeatures.MutablePlayerFeatures()->SetSecondsSincePause(secondsSincePause);

    auto& vinsFeatures = *scenarioFeatures.MutableVinsFeatures();
    vinsFeatures.SetIsContinuing(isContinuing);
    vinsFeatures.SetIsPureGC(isPureGC);

    return features;
}

class TConstCalcer : public NMatrixnet::IRelevCalcer {
public:
    TConstCalcer(const double value)
        : Value{value}
    {
    }

    double DoCalcRelev(const float* features) const override {
        Y_UNUSED(features);
        return Value;
    }

    size_t GetNumFeats() const override {
        return 0;
    }

private:
    const double Value;
};

class TSimpleStorage : public ISharedFormulasAdapter {
public:
    void AddFormula(const TStringBuf name, const TBaseCalcerPtr calcer) {
        Storage[name] = calcer;
    }

    TBaseCalcerPtr GetSharedFormula(const TStringBuf name) const override {
        return Storage.at(name);
    }

private:
    THashMap<TString, TBaseCalcerPtr> Storage;
};

TFormulasDescription InitDefaultFormulasDescription() {
    TFormulasDescription formulasDescription;

    TFormulaDescription formulaDescription;
    formulaDescription.MutableKey()->SetScenarioName("GeneralConversation");
    formulaDescription.MutableKey()->SetClassificationStage(ECS_POST);
    formulaDescription.MutableKey()->SetClientType(ECT_SMART_SPEAKER);
    formulaDescription.MutableKey()->SetLanguage(L_RUS);
    formulaDescription.SetFormulaName("794213.GeneralConversation");
    formulasDescription.AddFormula(formulaDescription);

    formulaDescription.MutableKey()->SetScenarioName("Search");
    formulaDescription.MutableKey()->SetClassificationStage(ECS_POST);
    formulaDescription.MutableKey()->SetClientType(ECT_SMART_SPEAKER);
    formulaDescription.MutableKey()->SetLanguage(L_RUS);
    formulaDescription.SetFormulaName("794217.Search");
    formulasDescription.AddFormula(formulaDescription);

    formulaDescription.MutableKey()->SetScenarioName("HollywoodMusic");
    formulaDescription.MutableKey()->SetClassificationStage(ECS_POST);
    formulaDescription.MutableKey()->SetClientType(ECT_SMART_SPEAKER);
    formulaDescription.MutableKey()->SetLanguage(L_RUS);
    formulaDescription.SetFormulaName("802883.HollywoodMusic");
    formulasDescription.AddFormula(formulaDescription);

    return formulasDescription;
}

struct TFixture : public NUnitTest::TBaseFixture {
    TFixture()
        : FormulasDescription{InitDefaultFormulasDescription()}
        , FormulasStorage{RawFormulasStorage, FormulasDescription}
        , ClientFeatures(/* proto= */ TClientInfoProto{}, /* flags= */ THashMap<TString, TMaybe<TString>>{})
        , ClientInfo(/* proto= */ TClientInfoProto{})
    {
        ClientFeatures.Name = "aliced";
        ClientInfo.Name = "aliced";

        const auto& formulasPath = TFsPath(GetWorkPath()) / "formulas" / "megamind_formulas";
        RawFormulasStorage.AddFormulasFromDirectoryRecursive(formulasPath);

        EXPECT_CALL(Ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));
        EXPECT_CALL(Ctx, LanguageForClassifiers()).WillRepeatedly(Return(ELanguage::LANG_RUS));
        EXPECT_CALL(Ctx, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(Ctx, ClientFeatures()).WillRepeatedly(ReturnRef(ClientFeatures));
        EXPECT_CALL(Ctx, ClientInfo()).WillRepeatedly(ReturnRef(ClientInfo));

        static THashMap<TString, TMaybe<TString>> expFlags;
        EXPECT_CALL(Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));
        EXPECT_CALL(Ctx, HasExpFlag(EXP_FORCE_VINS_SCENARIOS)).WillRepeatedly(Return(false));

        static auto speechKitRequest = TSpeechKitRequestBuilder(
            TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        EXPECT_CALL(Ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

        EXPECT_CALL(Ctx, Responses()).WillRepeatedly(ReturnRef(Responses));
        EXPECT_CALL(Responses, WizardResponse(_)).WillRepeatedly(ReturnRef(WizardResponse));

        static const TScenarioInfraConfig scenariosDefaultConfig;
        EXPECT_CALL(Ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenariosDefaultConfig));

        EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(Session.Get()));

        EXPECT_CALL(Ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));

        static TMockSensors sensors;
        EXPECT_CALL(Ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        EXPECT_CALL(Ctx, IsProtoVinsEnabled()).WillRepeatedly(Return(true));
    }

    void AddScenario(TStringBuf name, const ELang language = ELang::L_RUS) {
        TScenarioConfig config;
        config.SetName(TString{name});
        config.SetEnabled(true);
        config.MutableLanguages()->Add(language);
        Registry.AddScenarioConfig(config);
    }

    NiceMock<TMockContext> Ctx;
    TMaybe<TRequest::TScenarioInfo> BoostedScenario;
    TVector<TSemanticFrame> RecognizedActionEffectFrames;
    TScenarioConfigRegistry Registry;
    ::TFormulasStorage RawFormulasStorage;
    TFormulasDescription FormulasDescription;
    NAlice::TFormulasStorage FormulasStorage;
    TFactorStorage FactorStorage;
    TQualityStorage QualityStorage;
    THolder<ISession> Session;
    TMockResponses Responses;
    TWizardResponse WizardResponse;
    TClientFeatures ClientFeatures;
    TClientInfo ClientInfo;
    NScenarios::TInterfaces Interfaces;
};

void AddConstantPrediction(TSimpleStorage& simpleStorage, TFormulasDescription& formulasDescription, TStringBuf name, float value) {
    const auto calcer = MakeAtomicShared<TConstCalcer>(value);
    simpleStorage.AddFormula(name, calcer);

    TFormulaDescription formulaDescription;
    formulaDescription.MutableKey()->SetScenarioName(TString{name});
    formulaDescription.MutableKey()->SetClassificationStage(ECS_POST);
    formulaDescription.MutableKey()->SetClientType(ECT_SMART_SPEAKER);
    formulaDescription.MutableKey()->SetLanguage(L_RUS);
    formulaDescription.SetFormulaName(TString{name});

    formulasDescription.AddFormula(formulaDescription);
}

void ReplaceConstantPrediction(TSimpleStorage& simpleStorage, TStringBuf name, float value) {
    const auto calcer = MakeAtomicShared<TConstCalcer>(value);
    simpleStorage.AddFormula(name, calcer);
}

void InitPredictions(const THashMap<TStringBuf, TMaybe<ui32>>& scenarioNameToExpectedIndex, TSimpleStorage& simpleStorage, TFormulasDescription& formulasDescription) {
    for (const auto& [scenarioName, _] : scenarioNameToExpectedIndex) {
        AddConstantPrediction(simpleStorage, formulasDescription, scenarioName, 0.0);
    }
}

void InitPredictions(const TVector<TScenarioInfo>& scenarioInfos, TSimpleStorage& simpleStorage, TFormulasDescription& formulasDescription) {
    for (const auto& scenarioInfo : scenarioInfos) {
        AddConstantPrediction(simpleStorage, formulasDescription, scenarioInfo.Name, 0.0);
    }
}

TVector<TScenarioResponse> PrepareGeneralConversationResponses(
        TFixture& fixture,
        const THashMap<TStringBuf, TMaybe<ui32>>& scenarioNameToExpectedIndex,
        TMaybe<TString> proactivityActionFrame,
        bool gc_vins_response,
        const TString& gcIntent) {
    if (proactivityActionFrame) {
        fixture.RecognizedActionEffectFrames = {TSemanticFrameBuilder{proactivityActionFrame.GetRef()}.Build()};
    }
    fixture.WizardResponse = TWizardResponse();

    const auto& vinsFeatures = VinsFeatures(
        /* intent= */ gc_vins_response ? "personal_assistant.general_conversation.general_conversation" : "some_vins");

    TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
    for (auto& scenarioResponse : scenarioResponses) {
        if (scenarioResponse.GetScenarioName() == MM_PROTO_VINS_SCENARIO)
        {
            continue;
        }
        TString intent = scenarioResponse.GetScenarioName() == PROTOCOL_GENERAL_CONVERSATION_SCENARIO ? gcIntent : "some_intent";
        TFeatures features;
        features.MutableScenarioFeatures()->SetIntent(intent);
        scenarioResponse.SetFeatures(features);
    }

    return scenarioResponses;
}

void CheckVinsVsHollywoodMusic(const THashMap<TStringBuf, TMaybe<ui32>>& scenarioNameToExpectedIndex,
                               const bool putHollywoodMusic, const bool enableExp,
                               const bool irrelevantVins) {
    TFixture fixture;

    fixture.ClientInfo.Name = "ru.yandex.mobile.navigator";
    UNIT_ASSERT(!fixture.ClientInfo.IsSmartSpeaker());
    UNIT_ASSERT(!fixture.ClientInfo.IsSearchApp());
    fixture.ClientFeatures.Name = "ru.yandex.mobile.navigator";
    UNIT_ASSERT(!fixture.ClientFeatures.IsSmartSpeaker());
    UNIT_ASSERT(!fixture.ClientFeatures.IsSearchApp());

    THashMap<TString, TMaybe<TString>> expFlags;
    if (enableExp) {
        const TStringBuf flag = EXP_MUSIC_PLAY_CONFIDENCE_THRESHOLD_PREFIX;
        expFlags[flag + TString{"0.5"}] = "1";
    }
    EXPECT_CALL(fixture.Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

    TVector<TScenarioResponse> scenarioResponses = MakeResponses(
        scenarioNameToExpectedIndex,
        VinsFeatures(
            /* intent= */ "personal_assistant.scenarios.music_play",
            /* isContinuing= */ false,
            /* isPureGC= */ false,
            /* isIrrelevant= */ irrelevantVins)
        );

    if (!putHollywoodMusic) {
        // simulate preclassifier rejecting HollywoodMusic
        EraseIf(scenarioResponses, [](const auto& response) {
            return response.GetScenarioName() == HOLLYWOOD_MUSIC_SCENARIO;
        });
    }

    PostClassify(
        fixture.Ctx,
        fixture.Interfaces,
        fixture.BoostedScenario,
        fixture.RecognizedActionEffectFrames,
        fixture.Registry,
        fixture.FormulasStorage,
        fixture.FactorStorage,
        scenarioResponses,
        fixture.QualityStorage
    );
    CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
}

TWizardResponse ParseBegemotResponseJsonToWizardResponse(const TStringBuf begemotResponseJsonStr) {
    const auto begemotResponseJson = JsonFromString(begemotResponseJsonStr);
    auto begemotResponse = JsonToProto<NBg::NProto::TAlicePolyglotMergeResponseResult>(begemotResponseJson);
    return TWizardResponse(std::move(begemotResponse));
}

Y_UNIT_TEST_SUITE_F(PostClassify, TFixture) {
    Y_UNIT_TEST(WriteMetricsOnAmbiguousPostClassify) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_GENERAL_CONVERSATION_SCENARIO, 1u},
            {MM_MARKET_HOW_MUCH, 0u}
        };

        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(Ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", MM_MARKET_HOW_MUCH},
            {"name", "wins_per_second"}
        }));

        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"name", "ambiguous_wins_per_second"}
        }));

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
    }

    Y_UNIT_TEST(WriteMetricsNotOnAmbiguousPostClassify) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_ALICE4BUSINESS_SCENARIO, 0u},
            {MM_IOT_PROTOCOL_SCENARIO, 1u}
        };

        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(Ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", MM_ALICE4BUSINESS_SCENARIO},
            {"name", "wins_per_second"}
        }));

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
    }

    Y_UNIT_TEST(WriteMetricsNotOnAmbiguousPostClassifyWithPostClassifyPriority) {
        AddScenario(ToString(HOLLYWOOD_MUSIC_SCENARIO));

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 0u},
            {HOLLYWOOD_MUSIC_SCENARIO, 1u}
        };

        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        InitPredictions(scenarioNameToExpectedIndex, simpleStorage, formulasDescription);
        ReplaceConstantPrediction(simpleStorage, MM_PROTO_VINS_SCENARIO, 0.5);
        ReplaceConstantPrediction(simpleStorage, HOLLYWOOD_MUSIC_SCENARIO, 0.4);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);
        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(Ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", MM_PROTO_VINS_SCENARIO},
            {"name", "wins_per_second"}
        }));
        EXPECT_CALL(Ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
    }

    Y_UNIT_TEST(OneScenario) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 0u},
        };

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(TwoScenarios) {
        AddScenario(TString{HOLLYWOOD_COMMANDS_SCENARIO});

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_COMMANDS_SCENARIO, 0u},
            {MM_PROTO_VINS_SCENARIO, 1u},
        };

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(ForceVinsScenarios_EraseMusic) {
        AddScenario(TString{HOLLYWOOD_COMMANDS_SCENARIO});

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 0u},
        };

        EXPECT_CALL(Ctx, HasExpFlag(EXP_FORCE_VINS_SCENARIOS)).WillOnce(Return(true));
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(ForceVinsScenarios_LeaveGC) {
        AddScenario(TString{PROTOCOL_GENERAL_CONVERSATION_SCENARIO});

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 1u},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u}
        };

        EXPECT_CALL(Ctx, HasExpFlag(EXP_FORCE_VINS_SCENARIOS)).WillOnce(Return(true));
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(EraseScenarioWithoutPriority) {
        AddScenario(TString{HOLLYWOOD_COMMANDS_SCENARIO});

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_COMMANDS_SCENARIO, 0u},
            {MM_PROTO_VINS_SCENARIO, 1u},
            {"ScenarioWithoutPriority", Nothing()},
        };

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(PreferIsContinuing) {
        AddScenario(TString{HOLLYWOOD_COMMANDS_SCENARIO});

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_COMMANDS_SCENARIO, Nothing()},
            {MM_PROTO_VINS_SCENARIO, 0u},
        };

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        TScenarioResponse* responsePtr = FindIf(
            scenarioResponses,
            [](const auto& response) { return response.GetScenarioName() == MM_PROTO_VINS_SCENARIO; }
        );
        UNIT_ASSERT(responsePtr);
        TFeatures features;
        features.MutableScenarioFeatures()->MutableVinsFeatures()->SetIsContinuing(true);
        responsePtr->SetFeatures(features);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(RankByFormulasHollywood) {
        AddScenario(ToString(HOLLYWOOD_MUSIC_SCENARIO));

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_MUSIC_SCENARIO, 0u},
            {MM_PROTO_VINS_SCENARIO, 1u},
        };

        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        InitPredictions(scenarioNameToExpectedIndex, simpleStorage, formulasDescription);
        ReplaceConstantPrediction(simpleStorage, HOLLYWOOD_MUSIC_SCENARIO, 1.0);
        ReplaceConstantPrediction(simpleStorage, MM_PROTO_VINS_SCENARIO, 0.5);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);
        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(SearchAppHollywoodMusicExp) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_MUSIC_SCENARIO, 0u},
            {MM_PROTO_VINS_SCENARIO, Nothing()},
        };

        CheckVinsVsHollywoodMusic(scenarioNameToExpectedIndex,
                                    /* putHollywoodMusic= */ true,
                                    /* enableExp= */ true,
                                    /* irrelevantVins= */ false);
    }

    Y_UNIT_TEST(SearchAppHollywoodMusicAbsent) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 0u},
            {HOLLYWOOD_MUSIC_SCENARIO, Nothing()},
        };

        CheckVinsVsHollywoodMusic(scenarioNameToExpectedIndex,
                                    /* putHollywoodMusic= */ false,
                                    /* enableExp= */ true,
                                    /* irrelevantVins= */ false);
    }

    Y_UNIT_TEST(SearchAppHollywoodMusicExpIrrelevant) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_MUSIC_SCENARIO, 0u},
            {MM_PROTO_VINS_SCENARIO, Nothing()},
        };

        CheckVinsVsHollywoodMusic(scenarioNameToExpectedIndex,
                                    /* putHollywoodMusic= */ true,
                                    /* enableExp= */ true,
                                    /* irrelevantVins= */ true);
    }

    Y_UNIT_TEST(SearchAppHollywoodMusicNoExpIrrelevant) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_MUSIC_SCENARIO, 0u},
            {MM_PROTO_VINS_SCENARIO, Nothing()},
        };

        CheckVinsVsHollywoodMusic(scenarioNameToExpectedIndex,
                                    /* putHollywoodMusic= */ true,
                                    /* enableExp= */ false,
                                    /* irrelevantVins= */ true);
    }

    Y_UNIT_TEST(RankByFormulasPreferVins) {
        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        AddConstantPrediction(simpleStorage, formulasDescription, HOLLYWOOD_MUSIC_SCENARIO, 1.0);
        AddConstantPrediction(simpleStorage, formulasDescription, MM_PROTO_VINS_SCENARIO, 0.5);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);

        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_MUSIC_SCENARIO, Nothing()},
            {MM_PROTO_VINS_SCENARIO, 0u},
        };

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(
            scenarioNameToExpectedIndex,
            VinsFeatures(/* intent= */ "personal_assistant.scenarios.alarm_ask_sound")
        );
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(PlayerFeaturesAreUsed) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 0u},
            {MM_SEARCH_PROTOCOL_SCENARIO, Nothing()},
            {MM_NEWS_PROTOCOL_SCENARIO, Nothing()}
        };
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(
            scenarioNameToExpectedIndex,
            VinsFeatures("personal_assistant.scenarios.music_sing_song",
                         /* isContinuing */ false,
                         /* isPureGC */ false,
                         /* isIrrelevant */ false,
                         /* restorePlayer */ true,
                         /* secondsSincePause*/ 10)
        );

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );

        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(MostRecentPlayerIsChosen) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, Nothing()},
            {HOLLYWOOD_MUSIC_SCENARIO, 0u}
        };

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(
            scenarioNameToExpectedIndex,
            VinsFeatures("personal_assistant.scenarios.music_sing_song",
                         /* isContinuing */ false,
                         /* isPureGC */ false,
                         /* isIrrelevant */ false,
                         /* restorePlayer */ true,
                         /* secondsSincePause*/ 10)
        );

        auto& hollywoodMusicResponse = *FindIfPtr(scenarioResponses, [](const TScenarioResponse& response) {
            return response.GetScenarioName() == HOLLYWOOD_MUSIC_SCENARIO;
        });
        TFeatures features;
        features.MutableScenarioFeatures()->MutablePlayerFeatures()->SetRestorePlayer(true);
        features.MutableScenarioFeatures()->MutablePlayerFeatures()->SetSecondsSincePause(5);
        hollywoodMusicResponse.SetFeatures(features);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );

        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(PlayerVsPreferableByVins) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, Nothing()},
            {HOLLYWOOD_MUSIC_SCENARIO, 0u}
        };

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(
            scenarioNameToExpectedIndex,
            VinsFeatures("personal_assistant.scenarios.player_dislike")
        );

        auto& hollywoodMusicResponse = *FindIfPtr(scenarioResponses, [](const TScenarioResponse& response) {
            return response.GetScenarioName() == HOLLYWOOD_MUSIC_SCENARIO;
        });
        TFeatures features;
        features.MutableScenarioFeatures()->MutablePlayerFeatures()->SetRestorePlayer(true);
        features.MutableScenarioFeatures()->MutablePlayerFeatures()->SetSecondsSincePause(5);
        hollywoodMusicResponse.SetFeatures(features);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );

        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }


    Y_UNIT_TEST(ResponsesWithPlayerFeaturesDoNotWinHigherPriorityScenarios) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_IOT_PROTOCOL_SCENARIO, 0u},
            {MM_PROTO_VINS_SCENARIO, Nothing()}
        };

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(
            scenarioNameToExpectedIndex,
            VinsFeatures("personal_assistant.scenarios.player_continue",
                         /* isContinuing */ false,
                         /* isPureGC */ false,
                         /* isIrrelevant */ false,
                         /* restorePlayer */ true,
                         /* secondsSincePause*/ 10)
        );

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );

        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(FilterByFixlistScenarioLevel) {
        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        AddConstantPrediction(simpleStorage, formulasDescription, HOLLYWOOD_MUSIC_SCENARIO, 1.0);
        AddConstantPrediction(simpleStorage, formulasDescription, MM_PROTO_VINS_SCENARIO, 0.5);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);

        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        NiceMock<TMockResponses> responses;
        const auto wizardResponse = ParseBegemotResponseJsonToWizardResponse(R"({
            "AliceResponse": {
                "AliceFixlist": {
                    "Matches": {
                        "gc_request_banlist": {},
                        "general_fixlist": {"Intents": ["Vins"]}
                    }
                }
            }
        })");
        EXPECT_CALL(responses, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponse));
        EXPECT_CALL(Ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_MUSIC_SCENARIO, Nothing()},
            {MM_PROTO_VINS_SCENARIO, 0u},
        };
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(
            scenarioNameToExpectedIndex,
            VinsFeatures(/* intent= */ "personal_assistant.scenarios.not_fixlisted_intent")
        );
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(FilterByFixlistSupportedFeature) {
        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        AddConstantPrediction(simpleStorage, formulasDescription, HOLLYWOOD_MUSIC_SCENARIO, 1.0);
        AddConstantPrediction(simpleStorage, formulasDescription, MM_PROTO_VINS_SCENARIO, 0.5);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);

        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        NiceMock<TMockResponses> responses;
        const auto wizardResponse = ParseBegemotResponseJsonToWizardResponse(R"({
            "AliceResponse": {
                "AliceFixlist": {
                    "Matches": {
                        "gc_request_banlist": {},
                        "general_fixlist": {"Intents": ["Vins:CanOpenLink"]}
                    }
                }
            }
        })");
        EXPECT_CALL(responses, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponse));
        EXPECT_CALL(Ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_MUSIC_SCENARIO, 0u},
            {MM_PROTO_VINS_SCENARIO, 1u},
        };
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        {
            PostClassify(
                Ctx,
                Interfaces,
                BoostedScenario,
                RecognizedActionEffectFrames,
                Registry,
                formulasStorage,
                FactorStorage,
                scenarioResponses,
                QualityStorage
            );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }
        {
            scenarioNameToExpectedIndex = {
                {HOLLYWOOD_MUSIC_SCENARIO, Nothing()},
                {MM_PROTO_VINS_SCENARIO, 0u},
            };
            NScenarios::TInterfaces interfaces;
            interfaces.SetCanOpenLink(true);
            PostClassify(
                Ctx,
                interfaces,
                BoostedScenario,
                RecognizedActionEffectFrames,
                Registry,
                formulasStorage,
                FactorStorage,
                scenarioResponses,
                QualityStorage
            );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }
    }

    Y_UNIT_TEST(EnableModalMode) {
        const TString testScenarioName = "alice.test.scenario";

        AddScenario(testScenarioName);

        auto scenarioSession = NewScenarioSession(TState{});
        scenarioSession.SetActivityTurn(1);

        const auto& session = (*MakeSessionBuilder())
            .SetPreviousScenarioName(testScenarioName)
            .SetScenarioSession(testScenarioName, scenarioSession)
            .Build();
        EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(session.Get()));

        // Allow modality.
        {
            TScenarioInfraConfig scenarioConfig;
            scenarioConfig.MutableDialogManagerParams()->SetMaxActivityTurns(-1);
            EXPECT_CALL(Ctx, ScenarioConfig(testScenarioName)).WillRepeatedly(ReturnRef(scenarioConfig));

            THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
                {testScenarioName, 0u},
                {MM_IOT_PROTOCOL_SCENARIO, Nothing()},
                {MM_PROTO_VINS_SCENARIO, Nothing()},
            };

            TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
            PostClassify(
                Ctx,
                Interfaces,
                BoostedScenario,
                RecognizedActionEffectFrames,
                Registry,
                FormulasStorage,
                FactorStorage,
                scenarioResponses,
                QualityStorage
            );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }

        // Disallow modality.
        {
            TScenarioInfraConfig scenarioConfig;
            scenarioConfig.MutableDialogManagerParams()->SetMaxActivityTurns(0);
            EXPECT_CALL(Ctx, ScenarioConfig(testScenarioName)).WillRepeatedly(ReturnRef(scenarioConfig));

            THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
                {testScenarioName, Nothing()},
                {MM_IOT_PROTOCOL_SCENARIO, 0u},
                {MM_PROTO_VINS_SCENARIO, Nothing()},
            };

            TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
            PostClassify(
                Ctx,
                Interfaces,
                BoostedScenario,
                RecognizedActionEffectFrames,
                Registry,
                FormulasStorage,
                FactorStorage,
                scenarioResponses,
                QualityStorage
            );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }

        // Reset modality by irrelevance.
        {
            TScenarioInfraConfig scenarioConfig;
            scenarioConfig.MutableDialogManagerParams()->SetMaxActivityTurns(-1);
            EXPECT_CALL(Ctx, ScenarioConfig(testScenarioName)).WillRepeatedly(ReturnRef(scenarioConfig));

            THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
                {testScenarioName, Nothing()},
                {MM_PROTO_VINS_SCENARIO, 0u},
            };

            TVector<TScenarioResponse> scenarioResponses(Reserve(scenarioNameToExpectedIndex.size()));
            scenarioResponses.emplace_back(TString{MM_PROTO_VINS_SCENARIO}, NO_INPUT_FRAMES, true);

            TFeatures features;
            features.MutableScenarioFeatures()->SetIsIrrelevant(true);
            scenarioResponses.emplace_back(TString{testScenarioName}, NO_INPUT_FRAMES, true);
            scenarioResponses.back().SetFeatures(features);

            PostClassify(
                Ctx,
                Interfaces,
                BoostedScenario,
                RecognizedActionEffectFrames,
                Registry,
                FormulasStorage,
                FactorStorage,
                scenarioResponses,
                QualityStorage
            );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }
    }

    Y_UNIT_TEST(EraseIrrelevantScenario) {
        const TString testScenarioName = "alice.test.scenario";
        AddScenario(testScenarioName);

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, Nothing()},
            {testScenarioName, 0u},
        };

        TFeatures vinsFeatures = VinsFeatures("personal_assistant.general_conversation.general_conversation");
        vinsFeatures.MutableScenarioFeatures()->SetIsIrrelevant(true);
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(LeaveLastIrrelevantScenario) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 0u},
        };

        TFeatures vinsFeatures = VinsFeatures("personal_assistant.general_conversation.general_conversation");
        vinsFeatures.MutableScenarioFeatures()->SetIsIrrelevant(true);
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(SequentialExpectedRequests) {
        const TString& testScenarioName = "alice.test.scenario";

        TScenarioInfraConfig scenarioConfig;
        scenarioConfig.MutableDialogManagerParams()->SetMaxActivityTurns(1);
        EXPECT_CALL(Ctx, ScenarioConfig(testScenarioName)).WillRepeatedly(ReturnRef(scenarioConfig));

        AddScenario(testScenarioName);
        // First activity turn receives priority.
        {
            auto scenarioSession = NewScenarioSession(TState{});
            scenarioSession.SetActivityTurn(1);
            const auto& session = (*MakeSessionBuilder())
                .SetPreviousScenarioName(testScenarioName)
                .SetScenarioSession(testScenarioName, scenarioSession)
                .Build();
            EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(session.Get()));

            THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
                {testScenarioName, 0u},
                {MM_IOT_PROTOCOL_SCENARIO, Nothing()},
                {MM_PROTO_VINS_SCENARIO, Nothing()},
            };

            TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
            PostClassify(
                Ctx,
                Interfaces,
                BoostedScenario,
                RecognizedActionEffectFrames,
                Registry,
                FormulasStorage,
                FactorStorage,
                scenarioResponses,
                QualityStorage
            );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }

        // Second turn - no priority at all.
        {
            auto scenarioSession = NewScenarioSession(TState{});
            scenarioSession.SetActivityTurn(2);
            const auto& session = (*MakeSessionBuilder())
                .SetPreviousScenarioName(testScenarioName)
                .SetScenarioSession(testScenarioName, scenarioSession)
                .Build();
            EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(session.Get()));

            THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
                {testScenarioName, Nothing()},
                {MM_IOT_PROTOCOL_SCENARIO, 0u},
                {MM_PROTO_VINS_SCENARIO, Nothing()},
            };

            TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
            PostClassify(
                Ctx,
                Interfaces,
                BoostedScenario,
                RecognizedActionEffectFrames,
                Registry,
                FormulasStorage,
                FactorStorage,
                scenarioResponses,
                QualityStorage
            );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }
    }

    Y_UNIT_TEST(GeneralConversation_GcForceIntent) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const TString gcIntent = "alice.generative_toast";
        EXPECT_CALL(Ctx, HasExpFlag(EXP_ENABLE_GC_PROACTIVITY)).WillRepeatedly(Return(true));
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, Nothing()},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
            {HOLLYWOOD_MUSIC_SCENARIO, Nothing()},
            {MM_VIDEO_PROTOCOL_SCENARIO, Nothing()}
        };

        auto scenarioResponses = PrepareGeneralConversationResponses(
            *this,
            scenarioNameToExpectedIndex,
            /* proactivityActionFrame = */ Nothing(),
            /* gc_vins_response = */ true,
            /* gcIntent */ gcIntent);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversation_GcIntentWithForceFlag) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const TString gcIntent = "gc_some_intent";
        THashMap<TString, TMaybe<TString>> expFlags;
        expFlags[EXP_POSTCLASSIFIER_GC_FORCE_INTENTS_PREFIX + gcIntent] = "1";
        EXPECT_CALL(Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        EXPECT_CALL(Ctx, HasExpFlag(EXP_ENABLE_GC_PROACTIVITY)).WillRepeatedly(Return(true));
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, Nothing()},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
            {HOLLYWOOD_MUSIC_SCENARIO, Nothing()},
            {MM_VIDEO_PROTOCOL_SCENARIO, Nothing()}
        };

        auto scenarioResponses = PrepareGeneralConversationResponses(
            *this,
            scenarioNameToExpectedIndex,
            /* proactivityActionFrame = */ Nothing(),
            /* gc_vins_response = */ true,
            /* gcIntent */ gcIntent);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversation_GcIntentWithoutForceFlag) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const TString gcIntent = "gc_some_intent";
        EXPECT_CALL(Ctx, HasExpFlag(EXP_ENABLE_GC_PROACTIVITY)).WillRepeatedly(Return(true));
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 3u},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
            {HOLLYWOOD_MUSIC_SCENARIO, 1u},
            {MM_VIDEO_PROTOCOL_SCENARIO, 2u},
        };

        auto scenarioResponses = PrepareGeneralConversationResponses(
            *this,
            scenarioNameToExpectedIndex,
            /* proactivityActionFrame = */ Nothing(),
            /* gc_vins_response = */ true,
            /* gcIntent */ gcIntent);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }


    Y_UNIT_TEST(GeneralConversationProactivity_ActionNoFlag) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        EXPECT_CALL(Ctx, HasExpFlag(EXP_ENABLE_GC_PROACTIVITY)).WillRepeatedly(Return(false));
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 3u},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 1u},
            {HOLLYWOOD_MUSIC_SCENARIO, 0u},
            {MM_VIDEO_PROTOCOL_SCENARIO, 2u},
        };

        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        InitPredictions(scenarioNameToExpectedIndex, simpleStorage, formulasDescription);
        ReplaceConstantPrediction(simpleStorage, HOLLYWOOD_MUSIC_SCENARIO, 1.0);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);
        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        auto scenarioResponses = PrepareGeneralConversationResponses(
            *this,
            scenarioNameToExpectedIndex,
            ToString(MM_VIDEO_PROTOCOL_SCENARIO),
            /* gc_vins_response = */ true,
            /* gcIntent */ "");

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversationProactivity_NoActionFlag) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        EXPECT_CALL(Ctx, HasExpFlag(EXP_ENABLE_GC_PROACTIVITY)).WillRepeatedly(Return(true));
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 3u},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
            {HOLLYWOOD_MUSIC_SCENARIO, 1u},
            {MM_VIDEO_PROTOCOL_SCENARIO, 2u},
        };

        auto scenarioResponses = PrepareGeneralConversationResponses(
            *this,
            scenarioNameToExpectedIndex,
            /* proactivityActionFrame = */ Nothing(),
            /* gc_vins_response = */ true,
            /* gcIntent */ "");

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversationProactivity_VinsGCActionFlagVideo) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        EXPECT_CALL(Ctx, HasExpFlag(EXP_ENABLE_GC_PROACTIVITY)).WillOnce(Return(true));
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, Nothing()},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, Nothing()},
            {HOLLYWOOD_MUSIC_SCENARIO, Nothing()},
            {MM_VIDEO_PROTOCOL_SCENARIO, 0u},
        };

        auto scenarioResponses = PrepareGeneralConversationResponses(
            *this,
            scenarioNameToExpectedIndex,
            ToString(MM_VIDEO_PROTOCOL_SCENARIO),
            /* gc_vins_response = */ true,
            /* gcIntent */ "");

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversationProactivity_VinsGCActionFlagMusic) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        EXPECT_CALL(Ctx, HasExpFlag(EXP_ENABLE_GC_PROACTIVITY)).WillRepeatedly(Return(true));
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, Nothing()},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, Nothing()},
            {HOLLYWOOD_MUSIC_SCENARIO, 0u},
            {MM_VIDEO_PROTOCOL_SCENARIO, Nothing()},
        };

        auto scenarioResponses = PrepareGeneralConversationResponses(
            *this,
            scenarioNameToExpectedIndex,
            ToString(NMusic::MUSIC_PLAY),
            /* gc_vins_response = */ true,
            /* gcIntent */ "");

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversationProactivity_VinsNotGCActionFlagVideo) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        EXPECT_CALL(Ctx, HasExpFlag(EXP_ENABLE_GC_PROACTIVITY)).WillOnce(Return(true));
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, Nothing()},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, Nothing()},
            {HOLLYWOOD_MUSIC_SCENARIO, Nothing()},
            {MM_VIDEO_PROTOCOL_SCENARIO, 0u},
        };

        auto scenarioResponses = PrepareGeneralConversationResponses(
            *this,
            scenarioNameToExpectedIndex,
            ToString(MM_VIDEO_PROTOCOL_SCENARIO),
            /* gc_vins_response = */ false,
            /* gcIntent */ "");

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversationProactivity_VinsNotGCActionFlagMusic) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        EXPECT_CALL(Ctx, HasExpFlag(EXP_ENABLE_GC_PROACTIVITY)).WillRepeatedly(Return(true));
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, Nothing()},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, Nothing()},
            {HOLLYWOOD_MUSIC_SCENARIO, 0u},
            {MM_VIDEO_PROTOCOL_SCENARIO, Nothing()},
        };

        auto scenarioResponses = PrepareGeneralConversationResponses(
            *this,
            scenarioNameToExpectedIndex,
            ToString(NMusic::MUSIC_PLAY),
            /* gc_vins_response = */ false,
            /* gcIntent */ "");

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversation_SwapVinsGCAndProtocolGC) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversation_NoSwapVinsGCAndProtocolGCHigherPriority) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation");

        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        InitPredictions(scenarioNameToExpectedIndex, simpleStorage, formulasDescription);
        ReplaceConstantPrediction(simpleStorage, PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 1.0);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);
        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversation_NoSwapVinsAnyAndProtocolGC) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 1u},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(/* intent= */ "any");

        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        InitPredictions(scenarioNameToExpectedIndex, simpleStorage, formulasDescription);
        ReplaceConstantPrediction(simpleStorage, PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 1.0);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);
        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }



    Y_UNIT_TEST(GeneralConversation_DummyWithDisableFlag_NoSwapWithVins) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 1u},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
        };
        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation_dummy");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(Search_SwapSearchAndVins) {
        AddScenario(ToString(MM_SEARCH_PROTOCOL_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 1u},
            {MM_SEARCH_PROTOCOL_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.scenarios.search");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(Search_SwapSearchAndVinsAnaphoric) {
        AddScenario(ToString(MM_SEARCH_PROTOCOL_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 1u},
            {MM_SEARCH_PROTOCOL_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.scenarios.search_anaphoric");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        EXPECT_CALL(Ctx, HasExpFlag(EXP_DISABLE_SEARCH_ACTIVATE_BY_VINS)).WillRepeatedly(Return(false));
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryWithVinsGCNoPureGC) {
        AddScenario(ToString(MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 1u},
            {MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryGcWithVinsGCNoPureGC) {
        AddScenario(ToString(MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 1u},
            {MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryWithVinsGCIsPureGC) {
        AddScenario(ToString(MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation",
            /* isContinuing= */ false,
            /* isPureGC= */ true);

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryGcWithVinsGCIsPureGC) {
        AddScenario(ToString(MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation",
            /* isContinuing= */ false,
            /* isPureGC= */ true);

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryWithMMGC) {
        AddScenario(ToString(MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO));
        AddScenario(ToString(MM_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_GENERAL_CONVERSATION_SCENARIO, 2u},
            {MM_PROTO_VINS_SCENARIO, 1u},
            {MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryGcWithMMGC) {
        AddScenario(ToString(MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO));
        AddScenario(ToString(MM_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_GENERAL_CONVERSATION_SCENARIO, 2u},
            {MM_PROTO_VINS_SCENARIO, 1u},
            {MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryWithProtocolGC) {
        AddScenario(ToString(MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO));
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 2u},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 1u},
            {MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryGcWithProtocolGC) {
        AddScenario(ToString(MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO));
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 1u},
            {MM_PROTO_VINS_SCENARIO, 2u},
            {MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryWithMMGCAndFacts) {
        AddScenario(ToString(MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO));
        AddScenario(ToString(MM_GENERAL_CONVERSATION_SCENARIO));
        AddScenario(ToString(MM_FACTS_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_GENERAL_CONVERSATION_SCENARIO, 3u},
            {MM_PROTO_VINS_SCENARIO, 2u},
            {MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO, 1u},
            {MM_FACTS_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryGcWithMMGCAndFacts) {
        AddScenario(ToString(MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO));
        AddScenario(ToString(MM_GENERAL_CONVERSATION_SCENARIO));
        AddScenario(ToString(MM_FACTS_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_GENERAL_CONVERSATION_SCENARIO, 3u},
            {MM_PROTO_VINS_SCENARIO, 2u},
            {MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO, 1u},
            {MM_FACTS_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryWithMMGCAndWeatherGC) {
        AddScenario(ToString(MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO));
        AddScenario(ToString(MM_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_GENERAL_CONVERSATION_SCENARIO, 1u},
            {MM_PROTO_VINS_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(/* intent= */ "personal_assistant.scenarios.get_weather");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(DialogovoDiscoveryGcWithMMGCAndWeatherGC) {
        AddScenario(ToString(MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO));
        AddScenario(ToString(MM_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_GENERAL_CONVERSATION_SCENARIO, 1u},
            {MM_PROTO_VINS_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(/* intent= */ "personal_assistant.scenarios.get_weather");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversation_DummyWithoutDisableFlag_SwapWithVins) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.general_conversation.general_conversation_dummy");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversation_PureWithDisableFlag_NoSwapWithVins) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 1u},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
        };
        const auto& vinsFeatures = VinsFeatures(/* intent= */ "personal_assistant.scenarios.pure_general_conversation");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversation_PureWithoutDisableFlag_SwapWithVins) {
        AddScenario(ToString(PROTOCOL_GENERAL_CONVERSATION_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(/* intent= */ "personal_assistant.scenarios.pure_general_conversation");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(MmScenariosPreferableToVinsScenarios_NoMmPreferredScenario) {
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 0u},
            {MM_GENERAL_CONVERSATION_SCENARIO, 1u},
        };

        const auto& vinsFeatures = VinsFeatures(/* intent= */ "personal_assistant.scenarios.find_poi");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(ProcessActions_givenScenarioWithActionInInputFramesAndModalScenario_prefersThatScenarioWithAction) {
        const TString actionName = "action";
        AddScenario("Director");
        AddScenario("AnyScenario");

        const TString modalScenarioName = "alice.modal.scenario";
        AddScenario(modalScenarioName);

        auto scenarioSession = NewScenarioSession(TState{});
        scenarioSession.SetActivityTurn(1);

        const auto& session = (*MakeSessionBuilder())
            .SetPreviousScenarioName(modalScenarioName)
            .SetScenarioSession(modalScenarioName, scenarioSession)
            .Build();
        EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(session.Get()));

        TScenarioInfraConfig scenarioConfig;
        scenarioConfig.MutableDialogManagerParams()->SetMaxActivityTurns(-1);
        EXPECT_CALL(Ctx, ScenarioConfig(modalScenarioName)).WillRepeatedly(ReturnRef(scenarioConfig));

        const TVector<TScenarioInfo> scenarioInfos = {
            {modalScenarioName, NO_INPUT_FRAMES},
            {"Director", {TSemanticFrameBuilder(actionName).Build()}},
            {"AnyScenario", NO_INPUT_FRAMES}
        };
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {"Director", 0u},
            {modalScenarioName, Nothing()},
            {"AnyScenario", Nothing()}
        };

        NiceMock<TMockResponses> responses;
        TWizardResponse wizardResponse;
        EXPECT_CALL(responses, WizardResponse(_))
            .WillRepeatedly(ReturnRef(wizardResponse));
        EXPECT_CALL(Ctx, Responses())
            .WillRepeatedly(ReturnRef(responses));

        RecognizedActionEffectFrames = {TSemanticFrameBuilder{actionName}.Build()};

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioInfos);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(ProcessActions_givenScenarioWithActionNotInInputFrames_doesntPreferThatScenario) {
        const TString actionName = "action";
        AddScenario("Director");
        AddScenario("AnyScenario");

        const TVector<TScenarioInfo> scenarioInfos = {
            {"Director", NO_INPUT_FRAMES},
            {"AnyScenario", NO_INPUT_FRAMES}
        };
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {"Director", 1u},
            {"AnyScenario", 0u},
        };

        NiceMock<TMockResponses> responses;
        TWizardResponse wizardResponse;
        EXPECT_CALL(responses, WizardResponse(_))
            .WillRepeatedly(ReturnRef(wizardResponse));
        EXPECT_CALL(Ctx, Responses())
            .WillRepeatedly(ReturnRef(responses));

        RecognizedActionEffectFrames = {TSemanticFrameBuilder{actionName}.Build()};

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }
    Y_UNIT_TEST(MusicAnaphora_BoostScenario) {
        AddScenario(TString(HOLLYWOOD_MUSIC_SCENARIO));

        const TVector<TScenarioInfo> scenarioInfos = {
            {TString(MM_PROTO_VINS_SCENARIO), NO_INPUT_FRAMES},
            {TString(HOLLYWOOD_MUSIC_SCENARIO), NO_INPUT_FRAMES},
        };

        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        InitPredictions(scenarioInfos, simpleStorage, formulasDescription);
        ReplaceConstantPrediction(simpleStorage, MM_PROTO_VINS_SCENARIO, 1.0);
        ReplaceConstantPrediction(simpleStorage, HOLLYWOOD_MUSIC_SCENARIO, 0.8);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);
        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioInfos);
        {
            const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
                {MM_PROTO_VINS_SCENARIO, 0u},
                {HOLLYWOOD_MUSIC_SCENARIO, 1u},
            };
            PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }
        TFeatures features;
        features.MutableScenarioFeatures()->SetIntent("personal_assistant.scenarios.music_play_anaphora");
        scenarioResponses[1].SetFeatures(features);
        {
            const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
                {MM_PROTO_VINS_SCENARIO, 0u},
                {HOLLYWOOD_MUSIC_SCENARIO, 1u},
            };
            PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }
        NiceMock<TMockResponses> responses;
        const auto wizardResponse = ParseBegemotResponseJsonToWizardResponse(R"({
            "AliceResponse": {
                "AliceParsedFrames": {
                    "Frames": [
                        {
                            "Name": "personal_assistant.scenarios.music_play_anaphora"
                        }
                    ],
                    "Sources": [
                        "Granet"
                    ],
                    "Confidences": [
                        1.0
                    ]
                }
            }
        })");
        EXPECT_CALL(responses, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponse));
        EXPECT_CALL(Ctx, Responses()).WillRepeatedly(ReturnRef(responses));
        EXPECT_CALL(Ctx, HasExpFlag(NAlice::NExperiments::EXP_HOLLYWOOD_MUSIC_PLAY_ANAPHORA)).WillOnce(Return(false));
        {
            const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
                {MM_PROTO_VINS_SCENARIO, 0u},
                {HOLLYWOOD_MUSIC_SCENARIO, 1u},
            };
            PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }

        EXPECT_CALL(Ctx, HasExpFlag(NAlice::NExperiments::EXP_HOLLYWOOD_MUSIC_PLAY_ANAPHORA)).WillOnce(Return(true));
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_MUSIC_SCENARIO, 0u},
        };
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(ForceRecipesOnTimerStop) {
        AddScenario(TString{HOLLYWOOD_COMMANDS_SCENARIO});
        AddScenario(TString{PROTOCOL_RECIPES_SCENARIO});
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_COMMANDS_SCENARIO, 1u},
            {PROTOCOL_RECIPES_SCENARIO, 0u},
        };

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(LeaveOnlyVinsScenarioByPreferableIntents) {
        AddScenario(ToString(MM_SHOW_TRAFFIC_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_SHOW_TRAFFIC_SCENARIO, {}},
            {MM_PROTO_VINS_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.navi.show_layer");

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(LeaveOnlyVinsScenarioByPreferableIntents_PlayerDislike) {
        AddScenario(ToString(MM_SHOW_TRAFFIC_SCENARIO));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {HOLLYWOOD_COMMANDS_SCENARIO, Nothing()},
            {HOLLYWOOD_MUSIC_SCENARIO, Nothing()},
            {MM_PROTO_VINS_SCENARIO, 0u},
        };

        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.scenarios.player_dislike",
            /* isContinuing */ false,
            /* isPureGC */ false,
            /* isIrrelevant */ false,
            /* restorePlayer */ true,
            /* secondsSincePause*/ 0);

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(LeaveOnlyVinsScenarioByPreferableIntents_PlayerLikeDisableFlag) {
        const auto& vinsFeatures = VinsFeatures(
            /* intent= */ "personal_assistant.scenarios.player_like");

        NiceMock<TMockResponses> responses;
        const auto wizardResponse = ParseBegemotResponseJsonToWizardResponse(R"({
            "AliceResponse": {
                "AliceParsedFrames": {
                    "Frames": [
                        {
                            "Name": "personal_assistant.scenarios.player_like"
                        }
                    ],
                    "Sources": [
                        "Granet"
                    ],
                    "Confidences": [
                        1.0
                    ]
                }
            }
        })");
        UNIT_ASSERT(wizardResponse.HasGranetFrame("personal_assistant.scenarios.player_like"));
        EXPECT_CALL(responses, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponse));
        EXPECT_CALL(Ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        {
            const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
                {HOLLYWOOD_COMMANDS_SCENARIO, 0u},
                {HOLLYWOOD_MUSIC_SCENARIO, 1u},
                {MM_PROTO_VINS_SCENARIO, 2u},
            };

            EXPECT_CALL(Ctx, HasExpFlag("mm_disable_like_preferred_vins_intent")).WillRepeatedly(Return(true));
            TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
            PostClassify(
                Ctx,
                Interfaces,
                BoostedScenario,
                RecognizedActionEffectFrames,
                Registry,
                FormulasStorage,
                FactorStorage,
                scenarioResponses,
                QualityStorage
            );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }

        {
            const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
                {HOLLYWOOD_COMMANDS_SCENARIO, 0u},
                {MM_PROTO_VINS_SCENARIO, 1u},
            };

            TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);
            PostClassify(
                Ctx,
                Interfaces,
                BoostedScenario,
                RecognizedActionEffectFrames,
                Registry,
                FormulasStorage,
                FactorStorage,
                scenarioResponses,
                QualityStorage
            );
            CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
        }
    }

    Y_UNIT_TEST(GeneralConversation_Postclassifier_GCWins) {
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 3u},
            {HOLLYWOOD_MUSIC_SCENARIO, 1u},
            {MM_VIDEO_PROTOCOL_SCENARIO, 2u},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
            {MM_SEARCH_PROTOCOL_SCENARIO, 4u},
        };

        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        InitPredictions(scenarioNameToExpectedIndex, simpleStorage, formulasDescription);
        ReplaceConstantPrediction(simpleStorage, PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 1.0);
        ReplaceConstantPrediction(simpleStorage, HOLLYWOOD_MUSIC_SCENARIO, 0.9);
        ReplaceConstantPrediction(simpleStorage, MM_VIDEO_PROTOCOL_SCENARIO, 0.8);
        ReplaceConstantPrediction(simpleStorage, MM_PROTO_VINS_SCENARIO, 0.7);
        ReplaceConstantPrediction(simpleStorage, MM_SEARCH_PROTOCOL_SCENARIO, 0.6);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);
        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        const auto& vinsFeatures = VinsFeatures(/* intent= */ "any");
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversation_Postclassifier_GCLose) {
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 2u},
            {HOLLYWOOD_MUSIC_SCENARIO, 0u},
            {MM_VIDEO_PROTOCOL_SCENARIO, 1u},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 4u},
            {MM_SEARCH_PROTOCOL_SCENARIO, 3u},
        };

        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        InitPredictions(scenarioNameToExpectedIndex, simpleStorage, formulasDescription);
        ReplaceConstantPrediction(simpleStorage, PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0.5);
        ReplaceConstantPrediction(simpleStorage, HOLLYWOOD_MUSIC_SCENARIO, 0.9);
        ReplaceConstantPrediction(simpleStorage, MM_VIDEO_PROTOCOL_SCENARIO, 0.8);
        ReplaceConstantPrediction(simpleStorage, MM_PROTO_VINS_SCENARIO, 0.7);
        ReplaceConstantPrediction(simpleStorage, MM_SEARCH_PROTOCOL_SCENARIO, 0.6);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);
        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        const auto& vinsFeatures = VinsFeatures(/* intent= */ "any");
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(GeneralConversation_Postclassifier_SwapVinsGCAndProtocolGC) {
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_PROTO_VINS_SCENARIO, 1u},
            {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, 0u},
        };

        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        InitPredictions(scenarioNameToExpectedIndex, simpleStorage, formulasDescription);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);
        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        const auto& vinsFeatures = VinsFeatures(/* intent= */ "personal_assistant.general_conversation.general_conversation");
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex, vinsFeatures);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }
}

struct TScenarioSlotRequestFixture : public TFixture {
    static const TSemanticFrame MakeFrame(bool withRequestedSlot = true) {
        TSemanticFrame frame = TSemanticFrameBuilder()
            .SetName("frame1")
            .AddSlot("slot1", {"type1"}, "type1", "value1", /* isRequested= */ false)
            .AddSlot("slot2", {"type2"}, "type2", "value2", /* isRequested= */ withRequestedSlot)
            .Build();
        return frame;
    }

    TScenarioSlotRequestFixture() {
        AddScenario(TestScenarioName);
        TSemanticFrame frame = TScenarioSlotRequestFixture::MakeFrame();
        THolder<ISessionBuilder> sessionBuilder = MakeSessionBuilder();

        TSessionProto::TScenarioSession scenarioSession;
        *scenarioSession.MutableState() = {};
        scenarioSession.SetConsequentIrrelevantResponseCount(0);

        sessionBuilder
            ->SetPreviousScenarioName(TestScenarioName)
            .SetScenarioSession(TestScenarioName, scenarioSession)
            .SetResponseFrame(frame);

        FirstAttemptSession = sessionBuilder->Build();
        scenarioSession.SetConsequentIrrelevantResponseCount(1);
        SecondAttemptSession = sessionBuilder
            ->Copy()
            ->SetScenarioSession(TestScenarioName, scenarioSession)
            .Build();
    }

    TVector<TScenarioResponse> MakeScenarioResponses(bool withRequestedSlot = true) const {
        const TSemanticFrame frame = TScenarioSlotRequestFixture::MakeFrame(withRequestedSlot);

        TVector<TScenarioResponse> responses;
        responses.emplace_back(TString{MM_IOT_PROTOCOL_SCENARIO}, NO_INPUT_FRAMES, SCENARIO_ACCEPTS_ANY_INPUT);
        responses.emplace_back(TestScenarioName, TVector<TSemanticFrame>{frame}, SCENARIO_ACCEPTS_ANY_INPUT);
        return responses;
    }

    const TString TestScenarioName = "alice.test.scenario";
    THolder<ISession> FirstAttemptSession;
    THolder<ISession> SecondAttemptSession;

    const TVector<TSemanticFrame> NO_INPUT_FRAMES;
    const bool SCENARIO_ACCEPTS_ANY_INPUT = true;

    const ui32 IOT_SCENARIO_INDEX = 0u;
    const ui32 TEST_SCENARIO_INDEX = 1u;
};

Y_UNIT_TEST_SUITE_F(PostClassifyScenarioRequestsSlot, TScenarioSlotRequestFixture) {
    Y_UNIT_TEST(SuccessfulSlotRequest) {
        EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(SecondAttemptSession.Get()));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {TestScenarioName, 0u},
            {MM_IOT_PROTOCOL_SCENARIO, Nothing()},
        };
        TVector<TScenarioResponse> responses = MakeScenarioResponses();

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            responses,
            QualityStorage
        );
        CheckOrder(responses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(NoRelatedFrameFirstAttempt) {
        EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(FirstAttemptSession.Get()));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {TestScenarioName, 0u},
            {MM_IOT_PROTOCOL_SCENARIO, Nothing()},
        };
        TVector<TScenarioResponse> responses = MakeScenarioResponses();
        responses[TEST_SCENARIO_INDEX] =
            TScenarioResponse(TestScenarioName, NO_INPUT_FRAMES, SCENARIO_ACCEPTS_ANY_INPUT);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            responses,
            QualityStorage
        );
        CheckOrder(responses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(NoRelatedFrameSecondAttempt) {
        EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(SecondAttemptSession.Get()));

        // Nothing should change
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {TestScenarioName, Nothing()},
            {MM_IOT_PROTOCOL_SCENARIO, 0u},
        };
        TVector<TScenarioResponse> responses = MakeScenarioResponses();
        responses[TEST_SCENARIO_INDEX] =
            TScenarioResponse(TestScenarioName, NO_INPUT_FRAMES, SCENARIO_ACCEPTS_ANY_INPUT);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            responses,
            QualityStorage
        );
        CheckOrder(responses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(NoRelatedFrameFirstAttemptHasOtherStronglyRelevantResponse) {
        EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(FirstAttemptSession.Get()));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {TestScenarioName, Nothing()},
            {MM_IOT_PROTOCOL_SCENARIO, 0u},
        };
        TVector<TScenarioResponse> responses = MakeScenarioResponses();
        responses[TEST_SCENARIO_INDEX] = TScenarioResponse(TestScenarioName, NO_INPUT_FRAMES, true);
        responses[IOT_SCENARIO_INDEX] =
            TScenarioResponse(TString{MM_IOT_PROTOCOL_SCENARIO}, NO_INPUT_FRAMES, false);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            responses,
            QualityStorage
        );
        CheckOrder(responses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(IrrelevantAnswerFromScenarioWithRelatedFrame) {
        EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(FirstAttemptSession.Get()));

        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {TestScenarioName, Nothing()},
            {MM_IOT_PROTOCOL_SCENARIO, 0u},
        };
        TVector<TScenarioResponse> responses = MakeScenarioResponses();

        TFeatures features;
        features.MutableScenarioFeatures()->SetIsIrrelevant(true);
        responses[TEST_SCENARIO_INDEX].SetFeatures(features);

        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            responses,
            QualityStorage
        );
        CheckOrder(responses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(WinsPerSecondSensor) {
        const THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_SHOW_TRAFFIC_SCENARIO, 1u},
            {MM_PROTO_VINS_SCENARIO, 2u},
            {HOLLYWOOD_COMMANDS_SCENARIO, 0u},
        };

        TSimpleStorage simpleStorage;
        TFormulasDescription formulasDescription;
        InitPredictions(scenarioNameToExpectedIndex, simpleStorage, formulasDescription);
        ReplaceConstantPrediction(simpleStorage, MM_SHOW_TRAFFIC_SCENARIO, 0.5);
        ReplaceConstantPrediction(simpleStorage, MM_PROTO_VINS_SCENARIO, 0.7);
        ReplaceConstantPrediction(simpleStorage, HOLLYWOOD_COMMANDS_SCENARIO, 1.0);
        AddConstantPrediction(simpleStorage, formulasDescription, SIDE_SPEECH_SCENARIO, 0.0);
        NAlice::TFormulasStorage formulasStorage{simpleStorage, formulasDescription};

        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {NAlice::NSignal::SCENARIO_NAME, MM_SHOW_TRAFFIC_SCENARIO},
            {NAlice::NSignal::NAME, "wins_per_second"}
        })).WillRepeatedly(Invoke([](){
            UNIT_ASSERT(false);
        }));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {NAlice::NSignal::SCENARIO_NAME, MM_PROTO_VINS_SCENARIO},
            {NAlice::NSignal::NAME, "wins_per_second"}
        })).WillRepeatedly(Invoke([](){
            UNIT_ASSERT(false);
        }));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {NAlice::NSignal::SCENARIO_NAME, HOLLYWOOD_COMMANDS_SCENARIO},
            {NAlice::NSignal::NAME, "wins_per_second"}
        }));
        EXPECT_CALL(Ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioNameToExpectedIndex);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            formulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );
    }

}

struct TArabicStackFixture : public TFixture {
    TArabicStackFixture() {
        EXPECT_CALL(Ctx, Language()).WillRepeatedly(Return(LANG_ARA));
        EXPECT_CALL(Ctx, LanguageForClassifiers()).WillRepeatedly(Return(LANG_ARA));
    }

    void AddScenario(TStringBuf name, const ELang language = ELang::L_ARA) {
        TFixture::AddScenario(name, language);
    }

    void AddScenarios(const TVector<TScenarioInfo>& scenarioInfos) {
        for (const TScenarioInfo& scenarioInfo : scenarioInfos) {
            AddScenario(scenarioInfo.Name, scenarioInfo.ResponseLanguage);
        }
    }
};


Y_UNIT_TEST_SUITE_F(ArabicPostClassify, TArabicStackFixture) {
    Y_UNIT_TEST(OneScenario) {
        TVector<TScenarioInfo> scenarioInfos = {
            {TString(MM_NEWS_PROTOCOL_SCENARIO), NO_INPUT_FRAMES, L_ARA}
        };

        AddScenarios(scenarioInfos);
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioInfos);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_NEWS_PROTOCOL_SCENARIO, 0u},
        };

        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(ThreeArabicScenarios) {
        TVector<TScenarioInfo> scenarioInfos = {
            {TString(MM_NEWS_PROTOCOL_SCENARIO), NO_INPUT_FRAMES, L_ARA},
            {TString(MM_SHOW_TRAFFIC_SCENARIO), NO_INPUT_FRAMES, L_ARA},
            {TString(MM_TV_CHANNELS_SCENARIO), NO_INPUT_FRAMES, L_ARA},
        };

        AddScenarios(scenarioInfos);
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioInfos);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );

        // rank by scenario names
        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_NEWS_PROTOCOL_SCENARIO, 0u},
            {MM_SHOW_TRAFFIC_SCENARIO, 1u},
            {MM_TV_CHANNELS_SCENARIO, 2u},
        };

        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(ProtocolArabicVsProtocolRussian) {
        TVector<TScenarioInfo> scenarioInfos = {
            {TString(MM_NEWS_PROTOCOL_SCENARIO), NO_INPUT_FRAMES, L_RUS},
            {TString(MM_TV_CHANNELS_SCENARIO), NO_INPUT_FRAMES, L_ARA},
        };

        AddScenarios(scenarioInfos);
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioInfos);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_TV_CHANNELS_SCENARIO, 0u},
            {MM_NEWS_PROTOCOL_SCENARIO, 1u},
        };

        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }

    Y_UNIT_TEST(HighPriorityRussianVsProtocolArabic) {
        TVector<TScenarioInfo> scenarioInfos = {
            {TString(MM_IOT_PROTOCOL_SCENARIO), NO_INPUT_FRAMES, L_RUS},
            {TString(MM_NEWS_PROTOCOL_SCENARIO), NO_INPUT_FRAMES, L_RUS},
            {TString(MM_TV_CHANNELS_SCENARIO), NO_INPUT_FRAMES, L_ARA},
        };

        AddScenarios(scenarioInfos);
        TVector<TScenarioResponse> scenarioResponses = MakeResponses(scenarioInfos);
        PostClassify(
            Ctx,
            Interfaces,
            BoostedScenario,
            RecognizedActionEffectFrames,
            Registry,
            FormulasStorage,
            FactorStorage,
            scenarioResponses,
            QualityStorage
        );

        THashMap<TStringBuf, TMaybe<ui32>> scenarioNameToExpectedIndex = {
            {MM_IOT_PROTOCOL_SCENARIO, 0u},
        };

        CheckOrder(scenarioResponses, scenarioNameToExpectedIndex);
    }
}

} // namespace
