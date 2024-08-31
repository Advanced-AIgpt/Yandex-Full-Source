#include "layout_inspector.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace {

using namespace NAlice::NHollywood::NModifiers;
using namespace NAlice::NScenarios;
using namespace NAlice;
using namespace testing;

class TInspectorCallbacks {
public:
    MOCK_METHOD(void, OnOutputSpeech, (TOutputSpeechInfo outputSpeechInfo), (const));
    MOCK_METHOD(void, OnCard, (TCardInfo cardInfo), (const));
};

MATCHER_P(CardIndexMatcher, index, "") {
    return arg.Index == static_cast<size_t>(index);
}

Y_UNIT_TEST_SUITE(LayoutInspector) {
    Y_UNIT_TEST(TestOutputSpeech) {
        TLayout layout;
        layout.AddCards()->SetText("ignored card text");

        {
            TLayoutInspectorForConjugatablePhrases inspector(layout);
            StrictMock<TInspectorCallbacks> callbacks;
            EXPECT_CALL(callbacks, OnOutputSpeech(_)).Times(0);
            inspector.InspectOutputSpeech([&](auto it) { callbacks.OnOutputSpeech(it); });
        }

        layout.SetOutputSpeech("");

        {
            TLayoutInspectorForConjugatablePhrases inspector(layout);
            StrictMock<TInspectorCallbacks> callbacks;
            EXPECT_CALL(callbacks, OnOutputSpeech(_)).Times(0);
            inspector.InspectOutputSpeech([&](auto it) { callbacks.OnOutputSpeech(it); });
        }

        layout.SetOutputSpeech("test without tag");

        {
            TLayoutInspectorForConjugatablePhrases inspector(layout);
            StrictMock<TInspectorCallbacks> callbacks;
            EXPECT_CALL(callbacks, OnOutputSpeech(_)).WillOnce([](TOutputSpeechInfo outputSpeech) {
                UNIT_ASSERT_EQUAL(outputSpeech.SimplePhrase, "test without tag");
                UNIT_ASSERT(outputSpeech.StartingSpeakerTag.empty());
            });
            inspector.InspectOutputSpeech([&](auto it) { callbacks.OnOutputSpeech(it); });
        }

        layout.SetOutputSpeech("<speaker voice=\"shitova\" lang=\"ar\">test with tag");

        {
            TLayoutInspectorForConjugatablePhrases inspector(layout);
            StrictMock<TInspectorCallbacks> callbacks;
            EXPECT_CALL(callbacks, OnOutputSpeech(_)).WillOnce([](TOutputSpeechInfo outputSpeech) {
                UNIT_ASSERT_EQUAL(outputSpeech.SimplePhrase, "test with tag");
                UNIT_ASSERT_EQUAL(outputSpeech.StartingSpeakerTag, "<speaker voice=\"shitova\" lang=\"ar\">");
            });
            inspector.InspectOutputSpeech([&](auto it) { callbacks.OnOutputSpeech(it); });
        }
    }

    Y_UNIT_TEST(TestCards) {
        TLayout layout;
        layout.SetOutputSpeech("ignored output speech");

        {
            TLayoutInspectorForConjugatablePhrases inspector(layout);
            StrictMock<TInspectorCallbacks> callbacks;
            EXPECT_CALL(callbacks, OnCard(_)).Times(0);
            inspector.InspectCards([&](auto it) { callbacks.OnCard(it); });
        }

        layout.AddCards();
        layout.AddCards()->SetText("");
        layout.AddCards()->MutableTextWithButtons();
        layout.AddCards()->MutableDiv2Card();

        {
            TLayoutInspectorForConjugatablePhrases inspector(layout);
            StrictMock<TInspectorCallbacks> callbacks;
            EXPECT_CALL(callbacks, OnCard(_)).Times(0);
            inspector.InspectCards([&](auto it) { callbacks.OnCard(it); });
        }

        layout.AddCards()->SetText("card 4");
        layout.AddCards()->SetText("");
        layout.AddCards()->MutableTextWithButtons()->SetText("card 6");

        {
            TLayoutInspectorForConjugatablePhrases inspector(layout);
            StrictMock<TInspectorCallbacks> callbacks;
            EXPECT_CALL(callbacks, OnCard(CardIndexMatcher(4))).WillOnce([&layout](TCardInfo cardInfo) {
                UNIT_ASSERT_EQUAL(cardInfo.CardPhrase, "card 4");
                UNIT_ASSERT_EQUAL(&cardInfo.Card, &layout.GetCards(4));
            });
            EXPECT_CALL(callbacks, OnCard(CardIndexMatcher(6))).WillOnce([&layout](TCardInfo cardInfo) {
                UNIT_ASSERT_EQUAL(cardInfo.CardPhrase, "card 6");
                UNIT_ASSERT_EQUAL(&cardInfo.Card, &layout.GetCards(6));
            });
            inspector.InspectCards([&](auto it) { callbacks.OnCard(it); });
        }
    }

    Y_UNIT_TEST(TestOutputSpeechAndCards) {
        TLayout layout;
        layout.SetOutputSpeech("test phrase");

        layout.AddCards()->MutableDiv2Card();
        layout.AddCards()->SetText("test phrase");
        layout.AddCards()->MutableTextWithButtons()->SetText("card 2");

        TLayoutInspectorForConjugatablePhrases inspector(layout);
        StrictMock<TInspectorCallbacks> callbacks;
        EXPECT_CALL(callbacks, OnOutputSpeech(_)).WillOnce([](TOutputSpeechInfo outputSpeech) {
            UNIT_ASSERT_EQUAL(outputSpeech.SimplePhrase, "test phrase");
            UNIT_ASSERT(outputSpeech.StartingSpeakerTag.empty());
        });
        EXPECT_CALL(callbacks, OnCard(CardIndexMatcher(1))).WillOnce([&layout](TCardInfo cardInfo) {
            UNIT_ASSERT_EQUAL(cardInfo.CardPhrase, "test phrase");
            UNIT_ASSERT_EQUAL(&cardInfo.Card, &layout.GetCards(1));
        });
        EXPECT_CALL(callbacks, OnCard(CardIndexMatcher(2))).WillOnce([&layout](TCardInfo cardInfo) {
            UNIT_ASSERT_EQUAL(cardInfo.CardPhrase, "card 2");
            UNIT_ASSERT_EQUAL(&cardInfo.Card, &layout.GetCards(2));
        });
        inspector.InspectOutputSpeech([&](auto it) { callbacks.OnOutputSpeech(it); });
        inspector.InspectCards([&](auto it) { callbacks.OnCard(it); });
    }
}

}
