#include <library/cpp/testing/unittest/registar.h>

#include <alice/begemot/lib/session_conversion/session_conversion.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

using namespace NBg;

static TVector<TString> ExtractTextHistory(const TVector<NVins::TSample>& history) {
    TVector<TString> textHistory;
    for (const auto& sample : history) {
        textHistory.push_back(sample.Text);
    }
    return textHistory;
}

Y_UNIT_TEST_SUITE(TSessionConversionTestSuite) {
    Y_UNIT_TEST(ExtractDialoguePhrasesForRewriterTest) {
        {
            NProto::TAliceSessionResult session;

            session.AddDialoguePhrases()->AddTokens("request_0");
            session.AddDialoguePhrases()->AddTokens("reply_0");
            session.AddDialoguePhrases()->AddTokens("request_1");
            session.AddDialoguePhrases()->AddTokens("reply_1");
            session.AddDialoguePhrases()->AddTokens("request_2");
            session.AddDialoguePhrases()->AddTokens("reply_2");

            session.MutableNormalizedRequest()->AddTokens("request_3");

            session.AddPreviousRewrittenRequests()->AddTokens("rewritten_request_0");
            session.AddPreviousRewrittenRequests()->AddTokens("rewritten_request_1");

            const auto history = ExtractDialoguePhrasesForRewriter(session);

            const TVector<TString> expectedHistory = {
                "request_0", "reply_0",
                "rewritten_request_0", "reply_1",
                "rewritten_request_1", "reply_2",
                "request_3"
            };

            UNIT_ASSERT_VALUES_EQUAL(ExtractTextHistory(history), expectedHistory);
        }
        {
            NProto::TAliceSessionResult session;

            session.AddDialoguePhrases()->AddTokens("request_0");
            session.AddDialoguePhrases()->AddTokens("reply_0");
            session.AddDialoguePhrases()->AddTokens("request_1");
            session.AddDialoguePhrases()->AddTokens("reply_1");

            session.MutableNormalizedRequest()->AddTokens("request_2");

            session.AddPreviousRewrittenRequests()->AddTokens("rewritten_request_0");
            session.AddPreviousRewrittenRequests()->AddTokens("rewritten_request_1");

            const auto history = ExtractDialoguePhrasesForRewriter(session);

            const TVector<TString> expectedHistory = {
                "rewritten_request_0", "reply_0",
                "rewritten_request_1", "reply_1",
                "request_2"
            };

            UNIT_ASSERT_VALUES_EQUAL(ExtractTextHistory(history), expectedHistory);
        }
        {
            NProto::TAliceSessionResult session;

            session.AddDialoguePhrases()->AddTokens("request_0");
            session.AddDialoguePhrases()->AddTokens("reply_0");

            session.MutableNormalizedRequest()->AddTokens("request_1");

            session.AddPreviousRewrittenRequests()->AddTokens("rewritten_request_-1");
            session.AddPreviousRewrittenRequests()->AddTokens("rewritten_request_0");

            const auto history = ExtractDialoguePhrasesForRewriter(session);

            const TVector<TString> expectedHistory = {
                "rewritten_request_-1", "",
                "rewritten_request_0", "reply_0",
                "request_1"
            };

            UNIT_ASSERT_VALUES_EQUAL(ExtractTextHistory(history), expectedHistory);
        }
        {
            NProto::TAliceSessionResult session;

            session.AddDialoguePhrases()->AddTokens("request_0");
            session.AddDialoguePhrases()->AddTokens("reply_0");

            session.MutableNormalizedRequest()->AddTokens("request_1");

            session.AddPreviousRewrittenRequests()->AddTokens("");
            session.AddPreviousRewrittenRequests()->AddTokens("rewritten_request_0");

            const auto history = ExtractDialoguePhrasesForRewriter(session);

            const TVector<TString> expectedHistory = {
                "", "",
                "rewritten_request_0", "reply_0",
                "request_1"
            };

            UNIT_ASSERT_VALUES_EQUAL(ExtractTextHistory(history), expectedHistory);
        }
        {
            NProto::TAliceSessionResult session;

            session.MutableNormalizedRequest()->AddTokens("request_0");

            session.AddPreviousRewrittenRequests()->AddTokens("rewritten_request_-2");
            session.AddPreviousRewrittenRequests()->AddTokens("rewritten_request_-1");

            const auto history = ExtractDialoguePhrasesForRewriter(session);

            const TVector<TString> expectedHistory = {
                "rewritten_request_-2", "",
                "rewritten_request_-1", "",
                "request_0"
            };

            UNIT_ASSERT_VALUES_EQUAL(ExtractTextHistory(history), expectedHistory);
        }
        {
            NProto::TAliceSessionResult session;

            session.AddDialoguePhrases()->AddTokens("request_0");
            session.AddDialoguePhrases()->AddTokens("reply_0");
            session.AddDialoguePhrases()->AddTokens("request_1");
            session.AddDialoguePhrases()->AddTokens("reply_1");
            session.AddDialoguePhrases()->AddTokens("request_2");
            session.AddDialoguePhrases()->AddTokens("reply_2");

            session.MutableNormalizedRequest()->AddTokens("request_3");

            session.AddPreviousRewrittenRequests()->AddTokens("rewritten_request_0");

            const auto history = ExtractDialoguePhrasesForRewriter(session);

            const TVector<TString> expectedHistory = {
                "request_0", "reply_0",
                "request_1", "reply_1",
                "rewritten_request_0", "reply_2",
                "request_3"
            };

            UNIT_ASSERT_VALUES_EQUAL(ExtractTextHistory(history), expectedHistory);
        }
        {
            NProto::TAliceSessionResult session;

            session.AddDialoguePhrases()->AddTokens("request_0");
            session.AddDialoguePhrases()->AddTokens("reply_0");
            session.AddDialoguePhrases()->AddTokens("request_1");
            session.AddDialoguePhrases()->AddTokens("reply_1");
            session.AddDialoguePhrases()->AddTokens("request_2");
            session.AddDialoguePhrases()->AddTokens("reply_2");

            session.MutableNormalizedRequest()->AddTokens("request_3");

            const auto history = ExtractDialoguePhrasesForRewriter(session);

            const TVector<TString> expectedHistory = {
                "request_0", "reply_0",
                "request_1", "reply_1",
                "request_2", "reply_2",
                "request_3"
            };

            UNIT_ASSERT_VALUES_EQUAL(ExtractTextHistory(history), expectedHistory);
        }
        {
            NProto::TAliceSessionResult session;

            session.AddDialoguePhrases()->AddTokens("request_0");
            session.AddDialoguePhrases()->AddTokens("reply_0");
            session.AddDialoguePhrases()->AddTokens("request_1");
            session.AddDialoguePhrases()->AddTokens("reply_1");
            session.AddDialoguePhrases()->AddTokens("request_2");
            session.AddDialoguePhrases()->AddTokens("reply_2");

            session.MutableNormalizedRequest()->AddTokens("request_3");

            session.AddPreviousRewrittenRequests()->AddTokens("rewritten_request_0");
            session.AddPreviousRewrittenRequests()->AddTokens("");

            const auto history = ExtractDialoguePhrasesForRewriter(session);

            const TVector<TString> expectedHistory = {
                "request_0", "reply_0",
                "rewritten_request_0", "reply_1",
                "request_2", "reply_2",
                "request_3"
            };

            UNIT_ASSERT_VALUES_EQUAL(ExtractTextHistory(history), expectedHistory);
        }
    }
}
