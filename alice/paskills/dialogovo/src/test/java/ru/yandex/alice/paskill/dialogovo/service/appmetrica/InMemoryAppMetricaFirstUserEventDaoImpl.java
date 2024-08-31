package ru.yandex.alice.paskill.dialogovo.service.appmetrica;

import java.time.Instant;
import java.util.HashSet;
import java.util.Set;

public class InMemoryAppMetricaFirstUserEventDaoImpl implements AppMetricaFirstUserEventDao {

    private final Set<AppMetricaServiceImpl.ApiKeySkillUserCacheKey> events = new HashSet<>();

    @Override
    public void saveFirstUserEvent(String appMetricaApiKey, String skillId, String uuid, Instant timestamp) {
        events.add(new AppMetricaServiceImpl.ApiKeySkillUserCacheKey(appMetricaApiKey, skillId, uuid));
    }

    @Override
    public boolean firstUserEventExists(String appMetricaApiKey, String skillId, String uuid) {
        return events.contains(new AppMetricaServiceImpl.ApiKeySkillUserCacheKey(appMetricaApiKey, skillId, uuid));
    }

    public void clear() {
        events.clear();
    }
}
