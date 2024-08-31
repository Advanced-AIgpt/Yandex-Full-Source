#include "utils.h"

#include <alice/bass/libs/globalctx/globalctx.h>

#include <alice/bass/libs/video_common/video_ut_helpers.h>

#include <alice/bass/libs/video_content/common.h>

#include <alice/bass/libs/ydb_config/config.h>
#include <alice/bass/libs/ydb_helpers/path.h>

#include <alice/bass/ut/helpers.h>

#include <alice/bass/ydb_config.h>

#include <ydb/public/sdk/cpp/client/ydb_scheme/scheme.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>

using namespace NBASS;
using namespace NBASS::NVideo;
using namespace NTestingHelpers;
using namespace NVideoCommon;
using namespace NYdbHelpers;

namespace {

class TAvailGlobalContext : public TTestGlobalContext {
public:
    explicit TAvailGlobalContext(TBassContextFixture& fixture)
        : Fixture{fixture}
        , LocalConfig{TTestGlobalContext::Config().DeepClone()}
        , TableClient{Fixture.LocalYdb()}
    {
        const auto& ydbConfig = Fixture.LocalYdbConfig();
        LocalConfig.YDb().DataBase() = ydbConfig.DataBase();
        LocalConfig.YDb().Endpoint() = ydbConfig.Endpoint();
    }

    NYdb::NTable::TTableClient& YdbClient() override {
        return TableClient;
    }

    const TConfig& Config() const override {
        return LocalConfig;
    }

private:
    TBassContextFixture& Fixture;
    TConfig LocalConfig;
    NYdb::NTable::TTableClient TableClient;
};

class TAvailContextFixture : public TBassContextFixture {
protected:
    TGlobalContextPtr MakeGlobalCtx() override {
        return IGlobalContext::MakePtr<TAvailGlobalContext>(*this);
    }
};

void FillContentDb(TDbVideoDirectory& videoDir) {
    TSerialDescriptor breakingBadSerial;
    breakingBadSerial.Id = "breaking_bad";
    TSeasonDescriptor breakingBadS1 =
        NTestingHelpers::MakeSeason(PROVIDER_KINOPOISK, breakingBadSerial.Id, "bb_s1" /* id */, 10, 4);
    breakingBadSerial.Seasons.push_back(std::move(breakingBadS1));

    TSerialDescriptor dexterSerial;
    dexterSerial.Id = "dexter";
    TSeasonDescriptor dexterS1 =
        NTestingHelpers::MakeSeason(PROVIDER_KINOPOISK, dexterSerial.Id, "dxt_s1" /* id */, 0, 3);
    dexterSerial.Seasons.push_back(std::move(dexterS1));

    videoDir.WriteSerialDescriptor(breakingBadSerial, PROVIDER_KINOPOISK);
    videoDir.WriteSerialDescriptor(dexterSerial, PROVIDER_KINOPOISK);
}

Y_UNIT_TEST_SUITE_F(VideoAvailabilityTestSuite, TAvailContextFixture) {
    Y_UNIT_TEST(FillAvailabilityInfoSmoke) {
        const auto input = NSc::TValue::FromJson(R"({
            "items": [{
                "type": "movie",
                "provider_name": "ivi",
                "provider_item_id": "12345"
            }, {
                "type": "movie",
                "provider_name": "ivi",
                "provider_item_id": "67890",
                "provider_info": [{
                    "type": "movie",
                    "provider_name": "ivi",
                    "provider_item_id": "67890"
                }, {
                    "type": "movie",
                    "provider_name": "amediateka",
                    "provider_item_id": "1234567890"
                }, {
                    "provider_item_id": "000",
                    "provider_name": "kinopoisk",
                    "type": "movie"
                }]
            }]
        })");

        TVideoGallery gallery(input);
        const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(TRequestJson{}));
        FillAvailabilityInfo(gallery, *ctx);

        const auto target = NSc::TValue::FromJson(R"({
            "items": [{
                "availability_request" : {
                    "ivi" : {
                        "id": "12345"
                    },
                    "type": "film"
                },
                "type": "movie",
                "provider_name": "ivi",
                "provider_item_id": "12345"
            }, {
                "availability_request": {
                    "amediateka": {
                        "id": "1234567890"
                    },
                    "ivi": {
                        "id": "67890"
                    },
                    "kinopoisk": {
                        "id": "000"
                    },
                    "type": "film"
                },
                "provider_info": [{
                        "provider_item_id": "67890",
                        "provider_name": "ivi",
                        "type": "movie"
                    }, {
                        "provider_item_id": "1234567890",
                        "provider_name": "amediateka",
                        "type": "movie"
                    }, {
                        "provider_item_id": "000",
                        "provider_name": "kinopoisk",
                        "type": "movie"
                    }
                ],
                "provider_item_id": "67890",
                "provider_name": "ivi",
                "type": "movie"
            }]
        })");

        UNIT_ASSERT(NTestingHelpers::EqualJson(target, *gallery->GetRawValue()));
    }

    Y_UNIT_TEST(FillAvailabilityInfoForSerials) {
        const auto ctx = MakeContext(NSc::TValue(TRequestJson{}));
        IGlobalContext& globalCtx = ctx->GlobalCtx();
        NYdb::NTable::TTableClient tableClient(globalCtx.YdbClient());
        NYdb::NScheme::TSchemeClient schemeClient(LocalYdb());

        const TString database = TString{*LocalYdbConfig().DataBase()};
        const TString dirName{"video_20190101T000000"};

        NYdbConfig::TConfig ydbConfig(tableClient, database);
        ydbConfig.Create();
        ydbConfig.Set(TString{NYdbConfig::KEY_VIDEO_LATEST}, dirName);
        globalCtx.YdbConfig().Update();

        const TTablePath videoDirPath(database, dirName);

        TDbVideoDirectory videoDir(tableClient, schemeClient, videoDirPath);
        FillContentDb(videoDir);

        const auto input = NSc::TValue::FromJson(R"({
            "items": [{
                "type": "tv_show",
                "provider_name": "kinopoisk",
                "provider_item_id": "breaking_bad"
            }, {
                "type": "tv_show",
                "provider_name": "kinopoisk",
                "provider_item_id": "dexter"
            }, {
                "type": "tv_show",
                "provider_name": "kinopoisk",
                "provider_item_id": "12121212121"
            }]
        })");

        TVideoGallery gallery(input);
        FillAvailabilityInfo(gallery, *ctx);

        const auto target = NSc::TValue::FromJson(R"(
          {
              "items": [
                  {
                    "availability_request": {
                        "kinopoisk" : {
                            "id" : "breaking_bad_s11e1",
                            "season_id" : "bb_s1",
                            "tv_show_id" : "breaking_bad"
                        },
                        "type" : "episode"
                    },
                    "provider_item_id" : "breaking_bad",
                    "provider_name":"kinopoisk",
                    "type" : "tv_show"
                  },
                  {
                      "availability_request" : {
                          "kinopoisk" : {
                              "id" : "dexter_s1e1",
                              "season_id" : "dxt_s1",
                              "tv_show_id" : "dexter"
                          },
                          "type" : "episode"
                      },
                      "provider_item_id" : "dexter",
                      "provider_name" : "kinopoisk",
                      "type" : "tv_show"
                  },
                  {
                      "provider_item_id" : "12121212121",
                      "provider_name" : "kinopoisk",
                      "type" : "tv_show"
                  }
              ]
          })");

        UNIT_ASSERT(NTestingHelpers::EqualJson(target, *gallery->GetRawValue()));
    }
}

} // namespace
