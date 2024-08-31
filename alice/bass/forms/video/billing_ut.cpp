#include <alice/bass/forms/video/billing.h>
#include <alice/bass/forms/video/billing_api.h>
#include <alice/bass/forms/video/defs.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/bass/forms/video/video_provider.h>

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/video_ut_helpers.h>
#include <alice/library/unittest/fake_fetcher.h>

#include <alice/bass/util/error.h>
#include <alice/bass/ut/helpers.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/system/yassert.h>

using namespace NBASS;
using namespace NBASS::NVideo;
using namespace NTestingHelpers;

namespace {
auto ITEM_GAME_OF_THRONES = NSc::TValue::FromJson(R"(
{
    "provider_item_id": "270fcf57b5d2fb65031c63fd0ce3d4f5_RU",
    "tv_show_item_id": "ighra_priestolov_93b7fddb-b084-489f-b7bf-49c80661a2b0",
    "tv_show_season_id": "siezon_1_11ebfc70-fcdb-42d8-b07e-886d7a3aa500",
    "provider_name": "amediateka",
    "type": "tv_show_episode",
    "available": "true"
}
)");

auto ITEM_MASHA_AND_THE_BEAR = NSc::TValue::FromJson(R"(
{
    "provider_item_id": "43854",
    "tv_show_item_id": "7312",
    "provider_name": "ivi",
    "type": "tv_show_episode",
    "available": "false"
}
)");

auto ITEM_BROTHER_2 = NSc::TValue::FromJson(R"(
{
    "provider_item_id": "33560",
    "provider_name": "ivi",
    "type": "movie",
    "available": "false"
}
)");

auto ITEM_THE_HATEFUL_8_IVI = NSc::TValue::FromJson(R"({
    "provider_item_id": "136338",
    "provider_name": "ivi",
    "type": "movie",
    "available": false,
    "price_from": 1000
})");

auto ITEM_THE_HATEFUL_8_AMEDIATEKA = NSc::TValue::FromJson(R"({
    "provider_item_id": "5553f744-fb67-4304-b018-300fb4339158",
    "provider_name": "amediateka",
    "type": "movie",
    "available": false,
})");

const auto BILLING_RESPONSE = NSc::TValue::FromJson(R"([
    {
        "available" : true,
        "contentIds" : {
            "ivi" : "136338"
        },
        "contentItem" : {
            "providerEntries" : [
                {
                    "ivi" : {
                        "contentType" : "film",
                        "id" : "136338"
                    }
                }
            ],
            "type" : "film"
        },
        "contentType" : "film",
        "minPrice" : null,
        "providersAvailable" : [
            "ivi"
        ],
        "providersThatRequireAccountBinding" : [ ]
    },
    {
        "available" : false,
        "contentIds" : {
            "amediateka" : "5553f744-fb67-4304-b018-300fb4339158"
        },
        "contentItem" : {
            "providerEntries" : [
                {
                    "amediateka" : {
                        "contentType" : "film",
                        "id" : "5553f744-fb67-4304-b018-300fb4339158"
                    }
                }
            ],
            "type" : "film"
        },
        "contentType" : "film",
        "minPrice" : 599,
        "providersAvailable" : [ ],
        "providersThatRequireAccountBinding" : [ ]
    }
])");

class TFakeBillingAPI : public IBillingAPI {
public:
    TFakeBillingAPI(int code, TStringBuf data)
        : Code(code)
        , Data(data) {
    }

    // IBillingAPI overrides:
    NHttpFetcher::THandle::TRef GetPlusPromoAvailability() override {
        Y_ASSERT(false);
        return {};
    }

protected:
    NHttpFetcher::THandle::TRef RequestContent(const TRequestContentOptions& /* options */,
                                               const NSc::TValue& /* contentItem */,
                                               const NSc::TValue& /* contentPlayPayload */) override {
        Y_ASSERT(false);
        return {};
    }

protected:
    const int Code;
    const TString Data;
};

class TFakeContentPlayAPI : public TFakeBillingAPI {
public:
    TFakeContentPlayAPI(int code, TStringBuf data, const NSc::TValue& contentItem)
        : TFakeBillingAPI(code, data)
        , ContentItem(contentItem) {
    }

protected:
    NHttpFetcher::THandle::TRef RequestContent(const TRequestContentOptions& /* options */,
                                               const NSc::TValue& contentItem,
                                               const NSc::TValue& /* contentPlayPayload */) override {
        UNIT_ASSERT(NTestingHelpers::EqualJson(ContentItem, contentItem));
        return MakeIntrusive<NAlice::NTestingHelpers::TFakeHandle>(Code, Data);
    }

private:
    const NSc::TValue ContentItem;
};

Y_UNIT_TEST_SUITE(BillingContentPlayUnitTests) {
    Y_UNIT_TEST(Smoke) {
        const auto contentItem = NSc::TValue::FromJson(R"({
            "ivi": {
                "contentType": "film",
                "id": "1234";
            }
        })");

        TShowPayScreenCommandData commandData;

        {
            TFakeContentPlayAPI api(HttpCodes::HTTP_UNAUTHORIZED, "" /* data */, contentItem);

            TContentRequestResponse response;
            const auto error =
                RequestContent(api, TRequestContentOptions{}, contentItem, commandData.Scheme(), response);
            UNIT_ASSERT(error);
        }

        {
            TFakeContentPlayAPI api(HttpCodes::HTTP_OK, R"({"status": "available"})", contentItem);

            TContentRequestResponse response;
            const auto error =
                RequestContent(api, TRequestContentOptions{}, contentItem, commandData.Scheme(), response);

            UNIT_ASSERT(error);
        }

        {
            const auto data = TStringBuf(R"({
                "status": "available",
                "providers": {
                    "amediateka": {"url": "http://amedia_url", "payload": "data"},
                    "ivi": {"url": "http://ivi_url", "payload": {"session": "ivi_session"}}
                }
            })");

            TFakeContentPlayAPI api(HttpCodes::HTTP_OK, data, contentItem);

            TContentRequestResponse response;
            const auto error =
                RequestContent(api, TRequestContentOptions{}, contentItem, commandData.Scheme(), response);
            UNIT_ASSERT_C(!error, error->Msg);

            UNIT_ASSERT_VALUES_EQUAL(response.Status, TContentRequestResponse::EStatus::Available);
            UNIT_ASSERT(response.PersonalCard.IsNull());
            auto* payload = std::get_if<TPlayDataPayload>(&response);
            UNIT_ASSERT(payload);
            UNIT_ASSERT_VALUES_EQUAL(payload->size(), 2u);
            UNIT_ASSERT_VALUES_EQUAL((*payload)[0], (TPlayData{"amediateka", NSc::TValue::FromJson("data"),
                                                               "http://amedia_url", Nothing() /* sessionToken */}));
            UNIT_ASSERT_VALUES_EQUAL((*payload)[1],
                                     (TPlayData{"ivi", NSc::TValue::FromJson(R"({"session": "ivi_session"})"),
                                                "http://ivi_url", "ivi_session"}));
        }

        {
            const auto data = TStringBuf(R"({
                "status": "available",
                "providers": ["amediateka", "ivi"]
            })");

            TFakeContentPlayAPI api(HttpCodes::HTTP_OK, data, contentItem);

            TContentRequestResponse response;
            const auto error =
                RequestContent(api, TRequestContentOptions{}, contentItem, commandData.Scheme(), response);
            UNIT_ASSERT(error);
        }

        {
            const auto data = TStringBuf(R"({
                "status": "provider_login_required",
                "providers_to_login": ["ivi"]
            })");

            TFakeContentPlayAPI api(HttpCodes::HTTP_OK, data, contentItem);

            TContentRequestResponse response;
            const auto error = RequestContent(api, TRequestContentOptions{}, contentItem,
                                              commandData.Scheme(), response);
            UNIT_ASSERT_C(!error, error->Msg);

            UNIT_ASSERT_VALUES_EQUAL(response.Status, TContentRequestResponse::EStatus::ProviderLoginRequired);
            UNIT_ASSERT(response.PersonalCard.IsNull());
            auto* payload = std::get_if<TProviderNamePayload>(&response);
            UNIT_ASSERT(payload);
            UNIT_ASSERT_VALUES_EQUAL(payload->size(), 1u);
            UNIT_ASSERT_VALUES_EQUAL((*payload)[0], "ivi");
        }

        {
            const auto data = TStringBuf(R"({
                "status": "provider_login_required",
                "providers_to_login": ["ivi"],
                "personal_card": {
                    "card": {
                        "card_id": "station_billing_12345",
                        "button_url": "https://yandex.ru/quasar/id/kinopoisk/promoperiod",
                        "text": "Активировать Яндекс.Плюс",
                        "date_from": 1596398659,
                        "date_to": 1596405859,
                        "yandex.station_film": {
                            "min_price": 0
                        }
                    },
                    "remove_existing_cards": True
                }
            })");

            const NSc::TValue jsonData = NSc::TValue::FromJson(data);

            TFakeContentPlayAPI api(HttpCodes::HTTP_OK, data, contentItem);

            TContentRequestResponse response;
            const auto error = RequestContent(api, TRequestContentOptions{}, contentItem,
                                              commandData.Scheme(), response);
            UNIT_ASSERT_C(!error, error->Msg);

            UNIT_ASSERT_VALUES_EQUAL(response.Status, TContentRequestResponse::EStatus::ProviderLoginRequired);
            UNIT_ASSERT_VALUES_EQUAL(response.PersonalCard, jsonData["personal_card"]);
            auto* payload = std::get_if<TProviderNamePayload>(&response);
            UNIT_ASSERT(payload);
            UNIT_ASSERT_VALUES_EQUAL(payload->size(), 1u);
            UNIT_ASSERT_VALUES_EQUAL((*payload)[0], "ivi");
        }

        {
            const auto data = TStringBuf(R"({
                "status": "payment_required",
                "provider_rejection_reasons": {"kinopoisk": "GEO_CONSTRAINT_VIOLATION"},
            })");

            TFakeContentPlayAPI api(HttpCodes::HTTP_OK, data, contentItem);

            TContentRequestResponse response;
            const auto error =
                RequestContent(api, TRequestContentOptions{}, contentItem, commandData.Scheme(), response);
            UNIT_ASSERT_C(!error, error->Msg);

            UNIT_ASSERT_VALUES_EQUAL(response.Status, TContentRequestResponse::EStatus::PaymentRequired);
            UNIT_ASSERT(response.PersonalCard.IsNull());
            auto* payload = std::get_if<TErrorPayload>(&response);
            UNIT_ASSERT(payload);
            UNIT_ASSERT_VALUES_EQUAL(payload->size(), 1u);
            UNIT_ASSERT_VALUES_EQUAL((*payload)[0],
                                     (TBillingError{"kinopoisk", NVideoCommon::EPlayError::GEO_CONSTRAINT_VIOLATION}));
        }

        {
        {
            const auto data = TStringBuf(R"({
                "status": "payment_required",
                "provider_rejection_reasons": {"kinopoisk": "GEO_CONSTRAINT_VIOLATION"},
                "personal_card": {
                    "card": {
                        "card_id": "station_billing_12345",
                        "button_url": "https://yandex.ru/quasar/id/kinopoisk/promoperiod",
                        "text": "Активировать Яндекс.Плюс",
                        "date_from": 1596398659,
                        "date_to": 1596405859,
                        "yandex.station_film": {
                            "min_price": 0
                        }
                    },
                    "remove_existing_cards": True
                }
            })");

            const NSc::TValue jsonData = NSc::TValue::FromJson(data);

            TFakeContentPlayAPI api(HttpCodes::HTTP_OK, data, contentItem);

            TContentRequestResponse response;
            const auto error =
                RequestContent(api, TRequestContentOptions{}, contentItem, commandData.Scheme(), response);
            UNIT_ASSERT_C(!error, error->Msg);

            UNIT_ASSERT_VALUES_EQUAL(response.Status, TContentRequestResponse::EStatus::PaymentRequired);
            UNIT_ASSERT_VALUES_EQUAL(response.PersonalCard, jsonData["personal_card"]);
            auto* payload = std::get_if<TErrorPayload>(&response);
            UNIT_ASSERT(payload);
            UNIT_ASSERT_VALUES_EQUAL(payload->size(), 1u);
            UNIT_ASSERT_VALUES_EQUAL((*payload)[0],
                                     (TBillingError{"kinopoisk", NVideoCommon::EPlayError::GEO_CONSTRAINT_VIOLATION}));
        }
        }
    }
}
} // namespace
