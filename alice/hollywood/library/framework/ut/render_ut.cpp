#include <alice/hollywood/library/framework/ut/nlg/register.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>
#include <alice/hollywood/library/framework/ut/proto/render_div2_test.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <alice/megamind/protos/scenarios/action_space.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/protos/api/renderer/api.pb.h>

#include <alice/library/json/json.h>

#include <alice/protos/api/renderer/api.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice::NHollywoodFw;

struct TTestCallbackFrame : public TFrame {
    TTestCallbackFrame(const NAlice::TSemanticFrame* proto)
        : TFrame(proto)
        , StringSlot(this, "slot_name", "") {
    }
    TSlot<TString> StringSlot;
};

class TTestScene : public TScene<TRenderDiv2TestProto> {
public:
    TTestScene(const TScenario* owner)
        : TScene(owner, "test_scene") {
    }
    TRetMain Main(const TRenderDiv2TestProto&, const TRunRequest&, TStorage&, const TSource&) const override {
        HW_ERROR("Impossible");
    }
};

class TTestScenario : public TScenario {
public:
    TTestScenario(std::initializer_list<TString> cards)
        : TScenario("my_test_scenario")
        , CardNames_(cards) {
        RegisterScene<TTestScene>([this]() { RegisterSceneFn(&TTestScene::Main); });
        Register(&TTestScenario::Dispatch);
        RegisterRenderer(&TTestScenario::RenderCards);
        RegisterRenderer(&TTestScenario::RenderSuggests);
        RegisterRenderer(&TTestScenario::RenderCallbacks);
        RegisterRenderer(&TTestScenario::RenderDivCards);
        RegisterRenderer(&TTestScenario::RenderActionSpaces);
        SetNlgRegistration(NAlice::NHollywood::NLibrary::NFramework::NUt::NNlg::RegisterAll);
        SetApphostGraph(ScenarioRequest() >> NodeRun() >> ScenarioResponse());
    }

    TTestScenario(const TString& cardName)
        : TTestScenario{{cardName}} {
    }

    static TRetResponse RenderSuggests(const TProtoUtRenderer1&, TRender& render) {
        render.AddSuggestion("render_div2_test", "suggest1");
        render.AddSuggestion("render_div2_test", "suggest2", "TypeTextSample2");
        render.AddSuggestion("render_div2_test", "suggest3", "TypeTextSample3", "NameSample3");
        return TReturnValueSuccess();
    }

    TRetResponse RenderDivCards(const TProtoUtRenderer1&, TRender& render) const {
        NAlice::NRenderer::TDivRenderData divRenderData1;
        divRenderData1.SetCardId("mycard1");
        render.AddDivRender(std::move(divRenderData1));
        NAlice::NRenderer::TDivRenderData divRenderData2;
        divRenderData2.SetCardId("mycard2");
        render.AddDivRender(std::move(divRenderData2));
        return TReturnValueSuccess();
    }

    TRetResponse RenderActionSpaces(const TProtoUtRenderer2&, TRender& render) const {
        NAlice::NScenarios::TActionSpace actionSpace1;
        render.AddActionSpace("actionSpace1", std::move(actionSpace1));
        NAlice::NScenarios::TActionSpace actionSpace2;
        render.AddActionSpace("actionSpace2", std::move(actionSpace2));
        return TReturnValueSuccess();
    }

    static TRetResponse RenderCallbacks(const TProtoUtRenderer2&, TRender& render) {
        NAlice::TSemanticFrame frame;
        NAlice::TSemanticFrame_TSlot slot;
        slot.SetName("slot_name");
        slot.SetType("string");
        slot.SetValue("slot_string_value");
        *frame.MutableSlots()->Add() = slot;
        render.AddCallbackFrame("test_frame_name", frame);
        return TReturnValueSuccess();
    }

private:
    TRetScene Dispatch(const TRunRequest&, const TStorage&, const TSource&) const {
        TRenderDiv2TestProto proto;
        for (const auto& name : CardNames_) {
            proto.AddCardName(name);
        }
        return TReturnValueRenderIrrelevant(&TTestScenario::RenderCards, proto);
    }

    static TRetResponse RenderCards(const TRenderDiv2TestProto& proto, TRender& render) {
        for (const auto& card : proto.GetCardName()) {
            render.AppendDiv2FromNlg("render_div2_test", card, proto, /* hideBorders */ true);
        }
        return TReturnValueSuccess();
    }

private:
    const TVector<TString> CardNames_;
};

Y_UNIT_TEST_SUITE(HollywoodFrameworkRender) {
    Y_UNIT_TEST(Div2Card) {
        TTestScenario testScenario{"div2_card"};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        const auto& cards = testEnv.ResponseBody.GetLayout().GetCards();
        UNIT_ASSERT_VALUES_EQUAL(cards.size(), 1);
        UNIT_ASSERT(cards[0].HasDiv2CardExtended());
        UNIT_ASSERT_VALUES_EQUAL(
            cards[0].GetDiv2CardExtended().ShortUtf8DebugString(),
            R"(Body { fields { key: "test" value { string_value: "test" } } } HideBorders: true)");

    } // Y_UNIT_TEST(Div2Card)

    Y_UNIT_TEST(Div2MergeCards) {
        TTestScenario testScenario{std::initializer_list<TString>{"div2_card", "div2_card_1"}};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);

        const auto& cards = testEnv.ResponseBody.GetLayout().GetCards();
        UNIT_ASSERT_VALUES_EQUAL(cards.size(), 2);
        UNIT_ASSERT(cards[0].HasDiv2CardExtended());
        UNIT_ASSERT_VALUES_EQUAL(
            cards[0].GetDiv2CardExtended().ShortUtf8DebugString(),
            R"(Body { fields { key: "test" value { string_value: "test" } } } HideBorders: true)");
        UNIT_ASSERT(cards[1].HasDiv2CardExtended());
        UNIT_ASSERT_VALUES_EQUAL(
            cards[1].GetDiv2CardExtended().ShortUtf8DebugString(),
            R"(Body { fields { key: "test1" value { string_value: "test1" } } } HideBorders: true)");
    } // Y_UNIT_TEST(Div2MergeCards)

    Y_UNIT_TEST(Div2CardTemplates) {
        TTestScenario testScenario{"div2_card_templates"};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT_VALUES_EQUAL(
            testEnv.ResponseBody.GetLayout().ShortUtf8DebugString(),
            R"(Cards { Div2CardExtended { Body { fields { key: "test" value { string_value: "test" } } } HideBorders: true } } Div2Templates { fields { key: "t1" value { struct_value { } } } fields { key: "t2" value { struct_value { fields { key: "color" value { string_value: "red" } } } } } })");
    } // Y_UNIT_TEST(Div2CardTemplates)

    Y_UNIT_TEST(Div2CardPalette) {
        TTestScenario testScenario{"div2_card_palette"};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT_VALUES_EQUAL(
            testEnv.ResponseBody.GetLayout().ShortUtf8DebugString(),
            R"(Cards { Div2CardExtended { Body { fields { key: "test" value { string_value: "test" } } } HideBorders: true } } Div2Palette { fields { key: "smth" value { string_value: "smvalue" } } })");
    } // Y_UNIT_TEST(Div2CardPalette)

    Y_UNIT_TEST(Div2CardTemplatesPalette) {
        TTestScenario testScenario{"div2_card_templates_palette"};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT_VALUES_EQUAL(
            testEnv.ResponseBody.GetLayout().ShortUtf8DebugString(),
            R"(Cards { Div2CardExtended { Body { fields { key: "test" value { string_value: "test" } } } HideBorders: true } } Div2Templates { fields { key: "t1" value { struct_value { fields { key: "k" value { string_value: "v" } } } } } } Div2Palette { fields { key: "smth" value { string_value: "smvalue" } } })");
    } // Y_UNIT_TEST(Div2CardTemplatesPalette)

    Y_UNIT_TEST(Div2ErrorNoCard) {
        TTestScenario testScenario{"div2_error_no_card"};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        testEnv.DisableErrorReporting();
        UNIT_ASSERT(!(testEnv >> TTestApphost("run") >> testEnv));
        UNIT_ASSERT(testEnv.GetErrorInfo().Defined());
    } // Y_UNIT_TEST(Div2CardTemplatesPalette)

    Y_UNIT_TEST(Div2BadJson) {
        TTestScenario testScenario{"div2_bad_json"};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        testEnv.DisableErrorReporting();
        UNIT_ASSERT(!(testEnv >> TTestApphost("run") >> testEnv));
        UNIT_ASSERT(testEnv.GetErrorInfo().Defined());
    } // Y_UNIT_TEST(Div2CardTemplatesPalette)

    Y_UNIT_TEST(Suggests) {
        TTestScenario testScenario{"div2_error_no_card"};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestRender(&TTestScenario::RenderSuggests, {}) >> testEnv);
        const auto& layout = testEnv.ResponseBody.GetLayout();
        const auto& frameActions = testEnv.ResponseBody.GetFrameActions();
        UNIT_ASSERT(layout.GetSuggestButtons().size() == 3);
        UNIT_ASSERT(frameActions.size() == 3);
        TString actionIds1, actionIds2, actionIds3;
        for (const auto& it : layout.GetSuggestButtons()) {
            UNIT_ASSERT(it.HasActionButton());
            if (it.GetActionButton().GetTitle() == "Test suggest1") {
                actionIds1 = it.GetActionButton().GetActionId().c_str();
            } else if (it.GetActionButton().GetTitle() == "Test suggest2") {
                actionIds2 = it.GetActionButton().GetActionId().c_str();
            } else if (it.GetActionButton().GetTitle() == "Test suggest3") {
                actionIds3 = it.GetActionButton().GetActionId().c_str();
            } else {
                UNIT_ASSERT(false); // Undefined name
            }
        }
        // Now analyze frame_actions
        for (const auto& it : frameActions) {
            UNIT_ASSERT(it.second.HasNluHint());
            UNIT_ASSERT(it.second.HasDirectives());
            UNIT_ASSERT(it.second.GetDirectives().list_size() == 1);
            const auto& directive = it.second.GetDirectives().get_idx_list(0);
            UNIT_ASSERT(directive.HasTypeTextDirective());
            if (it.first == actionIds1) {
                UNIT_ASSERT_STRINGS_EQUAL(it.second.GetNluHint().GetFrameName(), actionIds1);
                UNIT_ASSERT_STRINGS_EQUAL(directive.GetTypeTextDirective().GetName(), "");
                UNIT_ASSERT_STRINGS_EQUAL(directive.GetTypeTextDirective().GetText(), "Test suggest1");
            } else if (it.first == actionIds2) {
                UNIT_ASSERT_STRINGS_EQUAL(it.second.GetNluHint().GetFrameName(), actionIds2);
                UNIT_ASSERT_STRINGS_EQUAL(directive.GetTypeTextDirective().GetName(), "");
                UNIT_ASSERT_STRINGS_EQUAL(directive.GetTypeTextDirective().GetText(), "TypeTextSample2");
            } else if (it.first == actionIds3) {
                UNIT_ASSERT_STRINGS_EQUAL(it.second.GetNluHint().GetFrameName(), actionIds3);
                UNIT_ASSERT_STRINGS_EQUAL(directive.GetTypeTextDirective().GetName(), "NameSample3");
                UNIT_ASSERT_STRINGS_EQUAL(directive.GetTypeTextDirective().GetText(), "TypeTextSample3");
            } else {
                UNIT_ASSERT(false); // Undefined name
            }
        }

    } // Y_UNIT_TEST(Suggests)
    Y_UNIT_TEST(CallbackFrameAndBack) {
        TTestScenario testScenario{"div2_error_no_card"};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestRender(&TTestScenario::RenderCallbacks, {}) >> testEnv);

        UNIT_ASSERT(testEnv.ResponseBody.GetFrameActions().size() == 1);
        for (const auto& it : testEnv.ResponseBody.GetFrameActions()) {
            UNIT_ASSERT(it.second.HasCallback());
            UNIT_ASSERT_STRINGS_EQUAL(it.second.GetNluHint().GetFrameName(), "test_frame_name");
        }
    } // Y_UNIT_TEST(CallbackFrameAndBack)
    //
    // TODO [DD] Need to remove this function after refactoring
    //
    Y_UNIT_TEST(RenderWithDivRender) {
        TTestScenario testScenario({});
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestRender(&TTestScenario::RenderDivCards, TProtoUtRenderer1{}) >> testEnv);

        UNIT_ASSERT(testEnv.DivRenderData.size() == 2);
        UNIT_ASSERT_STRINGS_EQUAL(testEnv.DivRenderData[0]->GetCardId(), "mycard1");
        UNIT_ASSERT_STRINGS_EQUAL(testEnv.DivRenderData[1]->GetCardId(), "mycard2");
    } // Y_UNIT_TEST(RenderWithDivRender)
    Y_UNIT_TEST(RenderWithActionSpace) {
        TTestScenario testScenario({});
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestRender(&TTestScenario::RenderActionSpaces, TProtoUtRenderer2{}) >> testEnv);

        const auto actionSpaces = testEnv.ResponseBody.GetActionSpaces();
        UNIT_ASSERT(actionSpaces.size() == 2);
        UNIT_ASSERT_VALUES_EQUAL(actionSpaces.count("actionSpace1"), 1);
        UNIT_ASSERT_VALUES_EQUAL(actionSpaces.count("actionSpace2"), 1);

    } // Y_UNIT_TEST(RenderWithActionSpace)
} // Y_UNIT_TEST_SUITE(HollywoodFrameworkRender)

} // namespace
