#include <alice/joker/library/core/request.h>
#include <alice/joker/library/core/requests_history.h>

#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace testing;
using namespace NAlice::NJoker;

namespace {

constexpr auto EXPECTED_SIMPLE_HISTORY = TStringBuf(R"([
  {
    "action":"http://yandex-vip.ru:1337/get/freemoney",
    "body":"Er wartet auf den Mittagswind\nDie Welle kommt und legt sich matt\nMit einem Fächer jeden Tag\nDer Alte macht das Wasser glatt\n",
    "headers":
      {
        "x-best-korea":"NorthKorea",
        "x-yandex-joke":"YourSpineIsWhite",
        "x-yandex-team":"Megamind"
      },
    "query":
      {
        "color":"magenta",
        "fruit":"banana",
        "number":"thirteen",
        "uno":"dos_cuatro"
      }
  }
])");

constexpr auto EXPECTED_PROTOBUF_BODY_HISTORY = TStringBuf(R"([
  {
    "action":"http://yandex-fallout-shelter.net:2020/need/vault",
    "body":"<protobuf of size 125>",
    "headers":
      {
        "content-type":"aPpLiCaTiOn/PrOtObUf",
        "x-yandex-team":"Megamind"
      },
    "query":
      {
        "reason":"covid 19"
      }
  }
])");

class TMockHttpContext : public IHttpContext {
public:
    MOCK_METHOD(const NUri::TUri&, Uri, (), (const));
    MOCK_METHOD(const THeaders&, Headers, (), (const));
    MOCK_METHOD(const TString&, Body, (), (const));
};

Y_UNIT_TEST_SUITE(RequestsHistory) {
    Y_UNIT_TEST(Common) {
        TMockHttpContext ctx;

        NUri::TUri uri("yandex-vip.ru", 1337, "/get/freemoney", "fruit=banana&color=magenta&number=thirteen&uno=dos_cuatro");
        EXPECT_CALL(ctx, Uri()).WillRepeatedly(ReturnRef(uri));

        IHttpContext::THeaders headers = {
            IHttpContext::THeader("x-yandex-joke: YourSpineIsWhite"),
            IHttpContext::THeader("x-yandex-proxy-header-x-yandex-fake-time: 1584300913"), // ignored
            IHttpContext::THeader("x-yandex-team: Megamind"),
            IHttpContext::THeader("x-best-korea: NorthKorea"),
            IHttpContext::THeader("x-yandex-via-proxy: localhost:12999") // ignored
        };
        EXPECT_CALL(ctx, Headers()).WillRepeatedly(ReturnRef(headers));

        TString body = "Er wartet auf den Mittagswind\n"
                    "Die Welle kommt und legt sich matt\n"
                    "Mit einem Fächer jeden Tag\n"
                    "Der Alte macht das Wasser glatt\n";
        EXPECT_CALL(ctx, Body()).WillRepeatedly(ReturnRef(body));

        // Check answer format
        {
            TRequestsHistory reqHist(/* maxSize =*/ 10);
            reqHist.Add("fake-group-id", ctx);

            auto hist = reqHist.Get("fake-group-id");
            UNIT_ASSERT(hist.Defined());

            TString histJson = TRequestsHistory::ToJson(hist.GetRef());
            UNIT_ASSERT_EQUAL(histJson, EXPECTED_SIMPLE_HISTORY);

            TVector<TString> wrongGroupIds = {
                "",
                "fak-group-id",
                "not-a-group-id",
                "13071999",
            };
            for (const auto& it : wrongGroupIds) {
                auto wrongHist = reqHist.Get(it);
                UNIT_ASSERT(wrongHist.Empty());
            }
        }

        // Check cache rewriting
        {
            TRequestsHistory reqHist(/* maxSize =*/ 10);
            reqHist.Add("fake-group-id", ctx);

            for (int count = 2; count < 100; ++count) {
                TString groupId = "fake-group-id-" + TString{std::to_string(count)};
                reqHist.Add(groupId, ctx);

                if (count > 10) {
                    auto hist = reqHist.Get("fake-group-id");
                    UNIT_ASSERT(hist.Empty());
                }
            }
        }

        // Check cache not rewriting
        {
            TRequestsHistory reqHist(/* maxSize =*/ 313);
            reqHist.Add("fake-group-id", ctx);

            for (int count = 2; count < 313; ++count) {
                TString groupId = "fake-group-id-" + TString{std::to_string(count)};
                reqHist.Add(groupId, ctx);
            }

            auto hist = reqHist.Get("fake-group-id");
            UNIT_ASSERT(hist.Defined());

            TString histJson = TRequestsHistory::ToJson(hist.GetRef());
            UNIT_ASSERT_EQUAL(histJson, EXPECTED_SIMPLE_HISTORY);
        }
    }

    Y_UNIT_TEST(Protobuf) {
        TMockHttpContext ctx;

        NUri::TUri uri("yandex-fallout-shelter.net", 2020, "/need/vault", "reason=covid%2019");
        EXPECT_CALL(ctx, Uri()).WillRepeatedly(ReturnRef(uri));

        IHttpContext::THeaders headers = {
            IHttpContext::THeader("CoNtEnT-TyPe: aPpLiCaTiOn/PrOtObUf"),
            IHttpContext::THeader("x-yandex-team: Megamind"),
        };
        EXPECT_CALL(ctx, Headers()).WillRepeatedly(ReturnRef(headers));

        TString body = "Er wartet auf den Mittagswind\n"
                    "Die Welle kommt und legt sich matt\n"
                    "Mit einem Fächer jeden Tag\n"
                    "Der Alte macht das Wasser glatt\n";
        EXPECT_CALL(ctx, Body()).WillRepeatedly(ReturnRef(body));

        // Check answer format
        {
            TRequestsHistory reqHist(/* maxSize =*/ 1);
            reqHist.Add("fake-group-id", ctx);

            auto hist = reqHist.Get("fake-group-id");
            UNIT_ASSERT(hist.Defined());

            TString histJson = TRequestsHistory::ToJson(hist.GetRef());
            UNIT_ASSERT_EQUAL(histJson, EXPECTED_PROTOBUF_BODY_HISTORY);
        }
    }
}

} // namespace
