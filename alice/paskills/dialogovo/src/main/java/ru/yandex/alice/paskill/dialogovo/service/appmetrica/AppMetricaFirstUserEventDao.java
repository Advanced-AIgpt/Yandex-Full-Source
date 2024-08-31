package ru.yandex.alice.paskill.dialogovo.service.appmetrica;

import java.time.Instant;

public interface AppMetricaFirstUserEventDao {
    void saveFirstUserEvent(String appMetricaApiKey, String skillId, String uuid, Instant timestamp);

    boolean firstUserEventExists(String appMetricaApiKey, String skillId, String uuid);
}
