#include "parser_util.h"

#include <alice/library/proto/ut/protos/test.pb.h>
#include <alice/library/proto/proto_adapter.h>
#include <alice/library/proto/proto_struct.h>
#include <alice/library/json/json.h>
#include <google/protobuf/util/json_util.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/file.h>

namespace NAlice {
    const TStringBuf CINEMA_STRING_START = R"(
    {
        "struct": {
            "embed_url": "",
            "cinema_name": "START",
            "tv_package_name": "ru.start.androidmobile",
            "variants": [
                {
                    "subscription_name": "",
                    "price": 399,
                    "type": "svod",
                    "quality": "",
                    "embed_url": ""
                }
            ],
            "code": "start",
            "tv_fallback_link": "home-app://market_item?package=ru.start.androidmobile",
            "favicon": "//avatars.mds.yandex.net/get-ott/239697/1a632675-0d99-4268-bd5e-d5f3dd800174/orig",
            "link": "https://start.ru/watch/smychok?utm_source=kinopoisk&utm_medium=feed_watch&utm_campaign=smychok",
            "tv_deeplink": "startru://start/series/smychok",
            "duration": 0,
            "hide_price": false
        },
    }
)";

    const TStringBuf CINEMA_STRING_OKKO_FAKE = R"(
    {
        "struct": {
            "embed_url": "",
            "cinema_name": "Okko",
            "tv_package_name": "",
            "variants": [
                {
                    "subscription_name": "",
                    "price": 399,
                    "type": "svod",
                    "quality": "",
                    "embed_url": ""
                }
            ],
            "code": "okko",
        },
    }
)";

    Y_UNIT_TEST_SUITE(ParserUtil) {
        Y_UNIT_TEST(ParserUtilCinemaData) {
            {
                TTestStruct testStruct;

                auto status = google::protobuf::util::JsonStringToMessage(CINEMA_STRING_START.Data(), &testStruct);
                UNIT_ASSERT(status.ok());

                TProtoAdapter protoValue(testStruct.GetStruct());
                UNIT_ASSERT_EQUAL(protoValue["tv_deeplink"].GetString(), "startru://start/series/smychok");
                UNIT_ASSERT_EQUAL(protoValue["hide_price"].GetBoolean(), false);

                const auto packageName = NHollywood::NVideo::CinemaDataTools::GetPackageName(protoValue);
                UNIT_ASSERT_EQUAL(packageName, "ru.start.androidmobile");
            }
            {
                TTestStruct testStruct;

                auto status = google::protobuf::util::JsonStringToMessage(CINEMA_STRING_OKKO_FAKE.Data(), &testStruct);
                UNIT_ASSERT(status.ok());

                TProtoAdapter protoValue(testStruct.GetStruct());
                UNIT_ASSERT_EQUAL(protoValue["code"].GetString(), "okko");

                const auto packageName = NHollywood::NVideo::CinemaDataTools::GetPackageName(protoValue);
                UNIT_ASSERT_EQUAL(packageName, "ru.more.play");
            }
        }

        Y_UNIT_TEST(ParserUtilMakeCinemaDataFromProtoAndJson) {
            TFileInput inputData("fixtures/Smychok.json");
            TString snippetString = inputData.ReadAll();

            NJson::TJsonValue smychok = JsonFromString(snippetString);
            const NProtoBuf::RepeatedPtrField<TOttVideoItem_TCinema> cinemaDataFromJson = NHollywood::NVideo::MakeCinemaData(smychok["data"]["rich_info"]["cinema_data"]);

            google::protobuf::Struct entitySnippet;
            google::protobuf::util::JsonParseOptions options;
            google::protobuf::util::JsonStringToMessage(snippetString, &entitySnippet, options);
            TProtoStructParser protoParser;
            google::protobuf::Struct cinemaDataStruct = protoParser.GetKey(entitySnippet, "data.rich_info.cinema_data");

            TProtoAdapter smychokProto(cinemaDataStruct);

            const NProtoBuf::RepeatedPtrField<TOttVideoItem_TCinema> cinemaDataFromProto = NHollywood::NVideo::MakeCinemaData(smychokProto);

            UNIT_ASSERT(cinemaDataFromProto.size() == cinemaDataFromJson.size());
            for (int i = 0; i < cinemaDataFromProto.size(); ++i) {
                UNIT_ASSERT(cinemaDataFromProto[i].ShortUtf8DebugString() == cinemaDataFromJson[i].ShortUtf8DebugString());
            }
        }
    }
}
