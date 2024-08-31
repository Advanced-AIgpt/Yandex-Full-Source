#include "mapper.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NVoice;
using namespace NVoice::NSurfaceMapper;

Y_UNIT_TEST_SUITE(MapperTest) {
    Y_UNIT_TEST(TestMap) {
        TMapper mapper;
        const auto item = mapper.Map({
            .AppInfo = {
                .AppId = "ru.mts.music.android",
            },
        });
        UNIT_ASSERT_EQUAL(TPlatforms(P_ANDROID), item.Platforms);
        UNIT_ASSERT_EQUAL(TVector<EPlatform>({P_ANDROID}), item.ListPlatforms());
        UNIT_ASSERT_EQUAL(P_ANDROID, item.GetPlatform());
        UNIT_ASSERT_EQUAL(TTypes(T_PROD), item.Types);
        UNIT_ASSERT_EQUAL(TVector<EType>({T_PROD}), item.ListTypes());
        UNIT_ASSERT_EQUAL(V_OTHER, item.Vendor);
        UNIT_ASSERT_EQUAL(ESurface::MtsMusic, item.Surface);
    }

    Y_UNIT_TEST(TestDefault) {
        const TMapper& mapper = GetMapperRef();
        const auto item = mapper.Map({
            .AppInfo = {
                .AppId = "fake.fake",
            },
        });
        UNIT_ASSERT_EQUAL(TPlatforms(), item.Platforms);
        UNIT_ASSERT_EQUAL(TVector<EPlatform>({}), item.ListAllPlatforms());
        UNIT_ASSERT_EQUAL(TVector<EPlatform>({P_UNKNOWN}), item.ListPlatforms());
        UNIT_ASSERT_EQUAL(P_UNKNOWN, item.GetPlatform());
        UNIT_ASSERT_EQUAL(TTypes(), item.Types);
        UNIT_ASSERT_EQUAL(TVector<EType>({}), item.ListAllTypes());
        UNIT_ASSERT_EQUAL(TVector<EType>({T_UNKNOWN}), item.ListTypes());
        UNIT_ASSERT_EQUAL(V_UNKNOWN, item.Vendor);
        UNIT_ASSERT_EQUAL(ESurface::Unknown, item.Surface);
    }

    Y_UNIT_TEST(TestMaybe) {
        const TMapper& mapper = GetMapperRef();
        const auto item = mapper.TryMap({
            .AppInfo = {
                .AppId = "ru.yandex.traffic.inhouse",
            },
        });
        UNIT_ASSERT(item);
        UNIT_ASSERT_EQUAL(TPlatforms(P_IOS), item->Platforms);
        UNIT_ASSERT_EQUAL(TVector<EPlatform>({P_IOS}), item->ListPlatforms());
        UNIT_ASSERT_EQUAL(P_IOS, item->GetPlatform());
        UNIT_ASSERT_EQUAL(TTypes(T_BETA), item->Types);
        UNIT_ASSERT_EQUAL(TVector<EType>({T_BETA}), item->ListTypes());
        UNIT_ASSERT_EQUAL(V_YANDEX, item->Vendor);
        UNIT_ASSERT_EQUAL(ESurface::maps, item->Surface);
    }

    Y_UNIT_TEST(TestPlatformUnion) {
        const TMapper& mapper = GetMapperRef();
        const auto item = mapper.Map({
            .AppInfo = {
                .AppId = "ru.yandex.takt",
            },
        });
        UNIT_ASSERT_EQUAL(item.Platforms, TPlatforms(P_ANDROID | P_IOS));
        UNIT_ASSERT_EQUAL(item.GetPlatform(), P_MOBILE);
        UNIT_ASSERT_EQUAL(item.ListPlatforms(), TVector<EPlatform>({P_MOBILE}));
        UNIT_ASSERT_EQUAL(item.ListAllPlatforms(), TVector<EPlatform>({P_MOBILE, P_IOS, P_ANDROID}));
        UNIT_ASSERT_EQUAL(item.Types, TTypes(T_DEV));
        UNIT_ASSERT_EQUAL(item.GetType(), T_DEV);
        UNIT_ASSERT_EQUAL(item.ListTypes(), TVector<EType>({T_DEV}));
        UNIT_ASSERT_EQUAL(item.Vendor, V_YANDEX);
        UNIT_ASSERT_EQUAL(item.Surface, ESurface::taxi);
    }

    Y_UNIT_TEST(TestTypeUnion) {
        const TMapper& mapper = GetMapperRef();
        const auto item = mapper.Map({
            .AppInfo = {
                .AppId = "ru.yandex.quasar.app",
            },
        });
        UNIT_ASSERT_EQUAL(item.Platforms, TPlatforms(P_LINUX));
        UNIT_ASSERT_EQUAL(item.GetPlatform(), P_LINUX);
        UNIT_ASSERT_EQUAL(item.ListPlatforms(), TVector<EPlatform>({P_LINUX}));
        UNIT_ASSERT_EQUAL(item.ListAllPlatforms(), TVector<EPlatform>({P_LINUX}));
        UNIT_ASSERT_EQUAL(item.Types, TTypes(T_DEV | T_BETA | T_PROD));
        UNIT_ASSERT_EQUAL(item.GetType(), T_ALL);
        UNIT_ASSERT_EQUAL(item.ListTypes(), TVector<EType>({T_ALL}));
        UNIT_ASSERT_EQUAL(item.ListAllTypes(), TVector<EType>({T_ALL, T_PUBLIC, T_PROD, T_NONPROD, T_BETA, T_DEV}));
        UNIT_ASSERT_EQUAL(item.Vendor, V_YANDEX);
        UNIT_ASSERT_EQUAL(item.Surface, ESurface::quasar);
    }

    Y_UNIT_TEST(TestTvAdHoc) {
        const TMapper& mapper = GetMapperRef();

        UNIT_ASSERT_EQUAL(TTypes(T_PROD), mapper.Map({
            .AppInfo = {
                .AppId = "com.yandex.tv.alice",
            },
        }).Types);
        UNIT_ASSERT_EQUAL(TTypes(T_PROD), mapper.Map({
            .AppInfo = {
                .AppId = "com.yandex.tv.alice",
                .AppVersion = "1.2.45",
            },
        }).Types);
        UNIT_ASSERT_EQUAL(TTypes(T_BETA), mapper.Map({
            .AppInfo = {
                .AppId = "com.yandex.tv.alice",
                .AppVersion = "1.3.0-beta",
            },
        }).Types);
    }

    Y_UNIT_TEST(TestYandexModuleAsQuasarNotTv_VOICESERV_4053) {
        const TMapper& mapper = GetMapperRef();

        {
            const TSurfaceInfo& info = mapper.Map({
                .AppInfo = {
                    .AppId = "com.yandex.tv.alice",
                    .AppVersion = "any",
                },
                .DeviceInfo = {
                    .DeviceModel = "yandexmodule_2"
                },
            });
            UNIT_ASSERT_EQUAL(info.Surface, ESurface::tv);
            UNIT_ASSERT_EQUAL(info.Types, TTypes(T_PROD));
            UNIT_ASSERT_EQUAL(info.UaasInfo.DeviceModel, "yandexmodule_2");
        }

        {
            const TSurfaceInfo& info = mapper.Map({
                .AppInfo = {
                    .AppId = "com.yandex.tv.alice",
                    .AppVersion = "any-beta",
                },
                .DeviceInfo = {
                    .DeviceModel = "yandexmodule_2"
                },
            });
            UNIT_ASSERT_EQUAL(info.Surface, ESurface::tv);
            UNIT_ASSERT_EQUAL(info.Types, TTypes(T_PROD));
            UNIT_ASSERT_EQUAL(info.UaasInfo.DeviceModel, "yandexmodule_2");
        }

        {
            const TSurfaceInfo& info = mapper.Map({
                .AppInfo = {
                    .AppId = "com.yandex.tv.alice",
                    .AppVersion = "any-beta",
                },
                .DeviceInfo = {
                    .DeviceModel = "not_yandexmodule_2"
                },
            });
            UNIT_ASSERT_EQUAL(info.Surface, ESurface::tv);
            UNIT_ASSERT_EQUAL(info.Types, TTypes(T_BETA));
            UNIT_ASSERT_EQUAL(info.UaasInfo.DeviceModel, "yandex_tv");
        }

        {
            // Mapper uses first strictly matching entry.
            // If DeviceModel is not set, entry for yandexmodule_2 should not be selected.
            const TSurfaceInfo& info = mapper.Map({
                .AppInfo = {
                    .AppId = "com.yandex.tv.alice",
                },
            });
            UNIT_ASSERT_EQUAL(info.Surface, ESurface::tv);
            UNIT_ASSERT_EQUAL(info.UaasInfo.DeviceModel, "yandex_tv");
        }
    }

    Y_UNIT_TEST(TestOtherMobiles) {
        const TMapper& mapper = GetMapperRef();
        {
            const TSurfaceInfo& info = mapper.Map({
                .AppInfo = {
                    .AppId = "zazaza",
                    .AppVersion = "",
                },
                .DeviceInfo = {
                    .Platform = "android"
                },
            });
            UNIT_ASSERT_EQUAL(info.Surface, ESurface::other_mobiles);
            UNIT_ASSERT_EQUAL(info.Platforms, P_ANDROID);
        } {
            const TSurfaceInfo& info = mapper.Map({
                .AppInfo = {
                    .AppId = "zuzuzu",
                    .AppVersion = "",
                },
                .DeviceInfo = {
                    .Platform = "iphone"
                },
            });
            UNIT_ASSERT_EQUAL(info.Surface, ESurface::other_mobiles);
            UNIT_ASSERT_EQUAL(info.Platforms, P_IOS);
        }
    }

    Y_UNIT_TEST(TestKeysWhiteList) {
        const TMapper& mapper = GetMapperRef();
        {
            const TSurfaceInfo& info = mapper.Map({
                .AppInfo = {
                    .AppId = "ru.yandex.quasar.app",
                },
            });
            UNIT_ASSERT_UNEQUAL(info.KeysWhiteList.size(), 0);
        } {
            const TSurfaceInfo& info = mapper.Map({
                .AppInfo = {
                    .AppId = "ru.yandex.searchplugin",
                    .AppVersion = "",
                },
                .DeviceInfo = {
                    .Platform = "android"
                },
            });
            UNIT_ASSERT_UNEQUAL(info.KeysWhiteList.size(), 0);
        }
    }
}
