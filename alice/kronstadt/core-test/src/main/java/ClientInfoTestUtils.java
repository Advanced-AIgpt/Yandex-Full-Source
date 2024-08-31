package ru.yandex.alice.kronstadt.test;

import java.util.Optional;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;

public interface ClientInfoTestUtils {

    String DEFAULT_UUID = "6422fb5c1da844d2bd7fba0cf5be9f8b";

    default ClientInfo searchApp(String uuid) {
        return ClientInfo.builder()
                .appId("ru.yandex.searchplugin")
                .appVersion("")
                .deviceId(Optional.empty())
                .osVersion("12.1.2")
                .platform("ios")
                .uuid(uuid)
                .lang("ru-RU")
                .timezone("Europe/Moscow")
                .build();
    }

    // TODO: compare to real Client Info
    default ClientInfo station(String uuid) {
        return ClientInfo.builder()
                .appId("ru.yandex.quasar")
                .appVersion("1.0")
                .osVersion("1.0")
                .platform("android")
                .uuid(uuid)
                .deviceId(Optional.of("e10d6d4f3ea2b1826bba2e17b758a653"))
                .lang("ru-RU")
                .timezone("Europe/Moscow")
                .deviceModel("yandexstation")
                .deviceManufacturer("Yandex")
                .build();
    }

    default ClientInfo stationMini(String uuid) {
        return ClientInfo.builder()
                .appId("aliced")
                .appVersion("1.0")
                .osVersion("1.0")
                .platform("Linux")
                .uuid(uuid)
                .deviceId(Optional.of("e10d6d4f3ea2b1826bba2e17b758a653"))
                .lang("ru-RU")
                .timezone("Europe/Moscow")
                .deviceModel("yandexmini")
                .deviceManufacturer("Yandex")
                .build();
    }
}
