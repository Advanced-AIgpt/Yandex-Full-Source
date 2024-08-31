#include <library/cpp/testing/unittest/registar.h>

#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/framework/unittest/test_environment.h>
#include <alice/hollywood/library/testing/mock_global_context.h>
#include <alice/library/geo/protos/user_location.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include "../util.h"

namespace NAlice::NVideoCommon {

    namespace {

        void SetupRequestLocation(NScenarios::TScenarioRunRequest& request, int region = 213, TString tld = "ru") {
            TUserLocationProto userLocationProto;
            userLocationProto.SetUserRegion(region);
            userLocationProto.SetUserTld(tld);
            {
                auto& dataSource = (*request.MutableDataSources())[EDataSourceType::USER_LOCATION];
                *dataSource.MutableUserLocation() = userLocationProto;
            }
        }

        Y_UNIT_TEST_SUITE(CgiHelpersTests) {
            Y_UNIT_TEST(IpregWithTCgiParameters) {
                NScenarios::TScenarioRunRequest request;
                SetupRequestLocation(request);

                NAppHost::NService::TTestContext serviceCtx;
                NAlice::NHollywood::TScenarioRunRequestWrapper wrapper{request, serviceCtx};

                TCgiParameters cgi;

                AddIpregParam(cgi, wrapper);

                UNIT_ASSERT(cgi.Print().Contains("ipreg=213"));
            }

            Y_UNIT_TEST(IpregWithTProxyRequestBuilder) {
                NScenarios::TScenarioRunRequest request;
                SetupRequestLocation(request);

                NAppHost::NService::TTestContext serviceCtx;
                NAlice::NHollywood::TScenarioRunRequestWrapper wrapper{request, serviceCtx};

                NScenarios::TRequestMeta meta;
                NHollywood::TMockGlobalContext globalCtx;
                NHollywood::TContext ctx_{globalCtx, TRTLogger::StderrLogger(), nullptr, nullptr};
                NJson::TJsonValue appHostParams;
                TFakeRng rng;
                NHollywood::TScenarioHandleContext ctx{serviceCtx, meta, ctx_, rng, ELanguage::LANG_RUS, ELanguage::LANG_RUS, appHostParams, nullptr};

                TProxyRequestBuilder requestBuilder{ctx, wrapper};
                requestBuilder.SetEndpoint("yandex.ru");

                AddIpregParam(requestBuilder, wrapper);

                const auto httpProxyRequest = requestBuilder.Build();

                UNIT_ASSERT(httpProxyRequest.Request.GetPath().Contains("ipreg=213"));
            }
        }

    } // namespace

}
