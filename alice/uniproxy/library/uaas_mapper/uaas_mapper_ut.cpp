#include <library/cpp/testing/unittest/registar.h>

#include "uaas_mapper.h"


const TString STATION_APP_ID_FOR_UNIPROXY = "ru.yandex.quasar.app";
const TString STATION_MINI_APP_ID_FOR_UNIPROXY = "aliced";


Y_UNIT_TEST_SUITE(UaasMapper) {
    Y_UNIT_TEST(UaasTest) {
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo(STATION_APP_ID_FOR_UNIPROXY, NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", browser);
            UNIT_ASSERT_VALUES_EQUAL("android", mobile);
            UNIT_ASSERT_VALUES_EQUAL("station", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo(STATION_MINI_APP_ID_FOR_UNIPROXY, NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", browser);
            UNIT_ASSERT_VALUES_EQUAL("android", mobile);
            UNIT_ASSERT_VALUES_EQUAL("station_mini", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo(STATION_MINI_APP_ID_FOR_UNIPROXY, "Linux", browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", browser);
            UNIT_ASSERT_VALUES_EQUAL("android", mobile);
            UNIT_ASSERT_VALUES_EQUAL("station_mini", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("ru.yandex.searchplugin", NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexSearch", browser);
            UNIT_ASSERT_VALUES_EQUAL("android", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("ru.yandex.mobile", NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexSearch", browser);
            UNIT_ASSERT_VALUES_EQUAL("iphone", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("ru.yandex.mobile.dev", NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexSearchBeta", browser);
            UNIT_ASSERT_VALUES_EQUAL("iphone", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("com.yandex.browser", NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexBrowser", browser);
            UNIT_ASSERT_VALUES_EQUAL("android", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("ru.yandex.mobile.search", "iphone", browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexBrowser", browser);
            UNIT_ASSERT_VALUES_EQUAL("iphone", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("YaBro", NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexBrowser", browser);
            UNIT_ASSERT_VALUES_EQUAL("wp", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("ru.yandex.yandexnavi", "android", browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexNavigator", browser);
            UNIT_ASSERT_VALUES_EQUAL("android", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("ru.yandex.mobile.navigator", "iphone", browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexNavigator", browser);
            UNIT_ASSERT_VALUES_EQUAL("iphone", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("yandex.auto", NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexAuto", browser);
            UNIT_ASSERT_VALUES_EQUAL("android", mobile);
            UNIT_ASSERT_VALUES_EQUAL("auto", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("ru.yandex.iosdk.elariwatch", NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", browser);
            UNIT_ASSERT_VALUES_EQUAL("android", mobile);
            UNIT_ASSERT_VALUES_EQUAL("watch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("com.yandex.launcher", "android", browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexLauncher", browser);
            UNIT_ASSERT_VALUES_EQUAL("android", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("winsearchbar", "Windows", browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexBar", browser);
            UNIT_ASSERT_VALUES_EQUAL("wp", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("yandex.cloud.ai", NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", browser);
            UNIT_ASSERT_VALUES_EQUAL("", mobile);
            UNIT_ASSERT_VALUES_EQUAL("cloud", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("ru.yandex.mobile.translate", "android", browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexApplications", browser);
            UNIT_ASSERT_VALUES_EQUAL("android", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("com.yandex.browser.beta", "android", browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("YandexBrowserBeta", browser);
            UNIT_ASSERT_VALUES_EQUAL("android", mobile);
            UNIT_ASSERT_VALUES_EQUAL("touch", device);
        }
    }
    Y_UNIT_TEST(StrangeTest) {
       {
            TString browser, mobile, device;
            NVoice::GetUaasInfo(NULL, NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", browser);
            UNIT_ASSERT_VALUES_EQUAL("", mobile);
            UNIT_ASSERT_VALUES_EQUAL("", device);
        }
        {
            TString browser, mobile, device;
            NVoice::GetUaasInfo("com.dusiassistant", NULL, browser, mobile, device);
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", browser);
            UNIT_ASSERT_VALUES_EQUAL("", mobile);
            UNIT_ASSERT_VALUES_EQUAL("", device);
        }
    }

    Y_UNIT_TEST(GetUaasInfoByClientInfoTest) {
        {
            const NVoice::TUaasInfo uaasInfo = NVoice::GetUaasInfoByClientInfo({
                .AppInfo{
                    .AppId = STATION_APP_ID_FOR_UNIPROXY,
                },
                .DeviceInfo = {
                    .DeviceModel = "station",
                    .Platform = NULL,
                }
            });
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", uaasInfo.BrowserName);
            UNIT_ASSERT_VALUES_EQUAL("android", uaasInfo.MobilePlatform);
            UNIT_ASSERT_VALUES_EQUAL("station", uaasInfo.DeviceType);
            UNIT_ASSERT_VALUES_EQUAL("yandexstation", uaasInfo.DeviceModel);
        }
        {
            const NVoice::TUaasInfo uaasInfo = NVoice::GetUaasInfoByClientInfo({
                .AppInfo{
                    .AppId = STATION_APP_ID_FOR_UNIPROXY,
                },
                .DeviceInfo = {
                    .DeviceModel = "station_2",
                    .Platform = NULL,
                    .DeviceModification = "bla-bla",
                }
            });
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", uaasInfo.BrowserName);
            UNIT_ASSERT_VALUES_EQUAL("android", uaasInfo.MobilePlatform);
            UNIT_ASSERT_VALUES_EQUAL("station", uaasInfo.DeviceType);
            UNIT_ASSERT_VALUES_EQUAL("yandexstation_2.revision_1", uaasInfo.DeviceModel);
        }
        {
            const NVoice::TUaasInfo uaasInfo = NVoice::GetUaasInfoByClientInfo({
                .AppInfo{
                    .AppId = STATION_APP_ID_FOR_UNIPROXY,
                },
                .DeviceInfo = {
                    .DeviceModel = "station_2",
                    .Platform = NULL,
                    .DeviceModification = "rev2",
                }
            });
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", uaasInfo.BrowserName);
            UNIT_ASSERT_VALUES_EQUAL("android", uaasInfo.MobilePlatform);
            UNIT_ASSERT_VALUES_EQUAL("station", uaasInfo.DeviceType);
            UNIT_ASSERT_VALUES_EQUAL("yandexstation_2.revision_2", uaasInfo.DeviceModel);
        }
        {
            const NVoice::TUaasInfo uaasInfo = NVoice::GetUaasInfoByClientInfo({
               .AppInfo{
                    .AppId = STATION_MINI_APP_ID_FOR_UNIPROXY,
                },
               .DeviceInfo = {
                   .DeviceModel = "yandexmini_2",
                   .Platform = NULL,
               }
            });
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", uaasInfo.BrowserName);
            UNIT_ASSERT_VALUES_EQUAL("android", uaasInfo.MobilePlatform);
            UNIT_ASSERT_VALUES_EQUAL("station_mini", uaasInfo.DeviceType);
            UNIT_ASSERT_VALUES_EQUAL("yandexmini_2", uaasInfo.DeviceModel);
        }
        {
            const TString uaasInfoJson = NVoice::GetUaasInfoJsonByClientInfo({
                .AppInfo{
                    .AppId = "ru.yandex.mobile.translate",
                },
                .DeviceInfo = {
                    .DeviceModel = NULL,
                    .Platform = "android",
                }
            });
            UNIT_ASSERT_VALUES_EQUAL(uaasInfoJson,
                "eyJicm93c2VyTmFtZSI6IllhbmRleEFwcGxpY2F0aW9ucyIsImRldmljZVR5cGUiOiJ0"
                "b3VjaCIsImRldmljZU1vZGVsIjoiIiwibW9iaWxlUGxhdGZvcm0iOiJhbmRyb2lkIn0="
            );
        }
        {
            const NVoice::TUaasInfo uaasInfo = NVoice::GetUaasInfoByClientInfo({
               .AppInfo{
                    .AppId = "ru.yandex.yandexmaps",
                    .AppVersion = "some app version",
                },
               .DeviceInfo = {
                   .DeviceModel = "some model",
                   .Platform = "some platform",
               }
            });
            UNIT_ASSERT_VALUES_EQUAL("YandexMaps", uaasInfo.BrowserName);
            UNIT_ASSERT_VALUES_EQUAL("touch", uaasInfo.DeviceType);
        }
        {
            const NVoice::TUaasInfo uaasInfo = NVoice::GetUaasInfoByClientInfo({
               .AppInfo{
                    .AppId = "com.yandex.alice",
                    .AppVersion = "some app version",
                },
               .DeviceInfo = {
                   .DeviceModel = "some model",
                   .Platform = "some platform",
               }
            });
            UNIT_ASSERT_VALUES_EQUAL("YandexStandaloneAlice", uaasInfo.BrowserName);
        }
        {
            const NVoice::TUaasInfo uaasInfo = NVoice::GetUaasInfoByClientInfo({
               .AppInfo{
                    .AppId = "yandex.auto.old",
                    .AppVersion = "some app version",
                },
               .DeviceInfo = {
                   .DeviceModel = "some model",
                   .Platform = "some platform",
               }
            });
            UNIT_ASSERT_VALUES_EQUAL("YandexAutoOld", uaasInfo.BrowserName);
            UNIT_ASSERT_VALUES_EQUAL("android", uaasInfo.MobilePlatform);
            UNIT_ASSERT_VALUES_EQUAL("auto", uaasInfo.DeviceType);
        }
        {
            const NVoice::TUaasInfo uaasInfo = NVoice::GetUaasInfoByClientInfo({
               .AppInfo{
                    .AppId = "com.yandex.tv.alice",
                },
               .DeviceInfo = {
                   .DeviceModel = "not-yandexmodule_2",
               }
            });
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", uaasInfo.BrowserName);
            UNIT_ASSERT_VALUES_EQUAL("android", uaasInfo.MobilePlatform);
            UNIT_ASSERT_VALUES_EQUAL("tv", uaasInfo.DeviceType);
            UNIT_ASSERT_VALUES_EQUAL("yandex_tv", uaasInfo.DeviceModel);
        }
        {
            const NVoice::TUaasInfo uaasInfo = NVoice::GetUaasInfoByClientInfo({
               .AppInfo{
                    .AppId = "com.yandex.tv.alice",
                },
               .DeviceInfo = {
                   .DeviceModel = "yandexmodule_2",
               }
            });
            UNIT_ASSERT_VALUES_EQUAL("OtherApplications", uaasInfo.BrowserName);
            UNIT_ASSERT_VALUES_EQUAL("android", uaasInfo.MobilePlatform);
            UNIT_ASSERT_VALUES_EQUAL("tv", uaasInfo.DeviceType);
            UNIT_ASSERT_VALUES_EQUAL("yandexmodule_2", uaasInfo.DeviceModel);
        }
        {
            const NVoice::TUaasInfo uaasInfo = NVoice::GetUaasInfoByClientInfo({
               .AppInfo{
                    .AppId = "ru.yandex.mobile.inhouse",
                    .AppVersion = "-1",
                },
               .DeviceInfo = {
                   .DeviceModel = "100500_G",
                   .Platform = "iphone_100500_G",
               }
            });
            UNIT_ASSERT_VALUES_EQUAL("YandexSearchBeta", uaasInfo.BrowserName);
            UNIT_ASSERT_VALUES_EQUAL("iphone", uaasInfo.MobilePlatform);
            UNIT_ASSERT_VALUES_EQUAL("touch", uaasInfo.DeviceType);
            UNIT_ASSERT_VALUES_EQUAL("", uaasInfo.DeviceModel);
        }
    }
}
