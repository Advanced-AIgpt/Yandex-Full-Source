package ru.yandex.alice.memento.storage;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;

import com.google.protobuf.Any;

import static java.util.Collections.emptyMap;
import static java.util.stream.Collectors.toMap;

public class InMemoryStorageDao implements StorageDao {

    private final Map<String, Map<String, Any>> userSettingsDB = new HashMap<>();
    private final Map<String, Map<String, Map<String, Any>>> userDevicesDB = new HashMap<>();
    private final Map<String, Map<String, Any>> scenariosDB = new HashMap<>();
    private final Map<String, Map<String, Map<String, Any>>> surfaceScenariosDB = new HashMap<>();

    private final Map<String, Map<String, Any>> userSettingsDBAnon = new HashMap<>();
    private final Map<String, Map<String, Map<String, Any>>> userDevicesDBAnon = new HashMap<>();
    private final Map<String, Map<String, Any>> scenariosDBAnon = new HashMap<>();
    private final Map<String, Map<String, Map<String, Any>>> surfaceScenariosDBAnon = new HashMap<>();

    public void clear() {
        userSettingsDB.clear();
        userDevicesDB.clear();
        scenariosDB.clear();
        surfaceScenariosDB.clear();
        userSettingsDBAnon.clear();
        userDevicesDBAnon.clear();
        scenariosDBAnon.clear();
        surfaceScenariosDBAnon.clear();
    }

    @Override
    public StoredData fetch(String uid, KeysToFetch keysToFetch, boolean anonymous) {
        Set<String> userKeys = keysToFetch.getUserKeys();
        Map<String, Set<String>> devicesKeys = keysToFetch.getDevicesKeys();
        Set<String> scenarios = keysToFetch.getScenarios();
        Map<String, Set<String>> surfaceScenarios = keysToFetch.getSurfaceScenarios();

        Map<String, Any> userSettings =
                fetchPerUserData(uid, userKeys, anonymous ? userSettingsDBAnon : userSettingsDB);
        Map<String, Map<String, Any>> surfacesSettings =
                fetchSurfaceData(uid, devicesKeys, anonymous ? userDevicesDBAnon : userDevicesDB);

        Map<String, Any> scenarioData =
                fetchPerUserData(uid, scenarios, anonymous ? scenariosDBAnon : scenariosDB);
        Map<String, Map<String, Any>> surfaceScenarioData =
                fetchSurfaceData(uid, surfaceScenarios, anonymous ? surfaceScenariosDBAnon : surfaceScenariosDB);

        return new StoredData(userSettings, surfacesSettings, scenarioData, surfaceScenarioData);
    }

    @Override
    public StoredData fetchAll(String uid, Set<String> surfaces, boolean anonymous) {
        Map<String, Any> userSettings = userSettingsDB.getOrDefault(uid, emptyMap());
        Map<String, Map<String, Any>> surfacesSettings = userDevicesDB.getOrDefault(uid, emptyMap()).entrySet().stream()
                .filter(entry -> surfaces.contains(entry.getKey()))
                .collect(toMap(Map.Entry::getKey, Map.Entry::getValue));

        Map<String, Any> scenarioData = scenariosDB.getOrDefault(uid, emptyMap());
        Map<String, Map<String, Any>> surfaceScenarioData = surfaceScenariosDB.getOrDefault(uid, emptyMap())
                .entrySet().stream()
                .filter(entry -> surfaces.contains(entry.getKey()))
                .collect(toMap(Map.Entry::getKey, Map.Entry::getValue));

        return new StoredData(userSettings, surfacesSettings, scenarioData, surfaceScenarioData);
    }

    private Map<String, Map<String, Any>> fetchSurfaceData(
            String uid,
            Map<String, Set<String>> devicesKeys,
            Map<String, Map<String, Map<String, Any>>> db
    ) {
        Map<String, Map<String, Any>> fetchedData = db.getOrDefault(uid, emptyMap());

        Map<String, Map<String, Any>> result = devicesKeys.entrySet().stream()
                .filter(entry -> fetchedData.containsKey(entry.getKey()))
                .collect(toMap(Map.Entry::getKey,
                        entry -> fetchedData.getOrDefault(entry.getKey(), emptyMap())
                                .entrySet()
                                .stream()
                                .filter(e2 -> entry.getValue().contains(e2.getKey()))
                                .collect(toMap(Map.Entry::getKey, Map.Entry::getValue))
                ));
        return result;
    }

    private Map<String, Any> fetchPerUserData(String uid, Set<String> userKeys, Map<String, Map<String, Any>> db) {
        Map<String, Any> fetched = db.getOrDefault(uid, emptyMap());

        Map<String, Any> result = userKeys.stream()
                .filter(fetched::containsKey)
                .collect(toMap(Function.identity(), fetched::get));
        return result;
    }

    @Override
    public void update(String userId, StoredData storedData, boolean anonymous) {
        var userSettings =
                userSettingsDB.computeIfAbsent(userId, key -> new HashMap<>(storedData.getUserSettings().size()));
        userSettings.putAll(storedData.getUserSettings());

        var scenarioData =
                scenariosDB.computeIfAbsent(userId, key -> new HashMap<>(storedData.getScenariosData().size()));
        scenarioData.putAll(storedData.getScenariosData());

        var userDevices =
                userDevicesDB.computeIfAbsent(userId, key -> new HashMap<>(storedData.getDeviceSettings().size()));

        for (Map.Entry<String, Map<String, Any>> deviceChange : storedData.getDeviceSettings().entrySet()) {

            Map<String, Any> deviceConfig = userDevices.computeIfAbsent(deviceChange.getKey(),
                    key -> new HashMap<>(deviceChange.getValue().size()));

            deviceConfig.putAll(deviceChange.getValue());
        }

        var scenarioSurfaces =
                surfaceScenariosDB.computeIfAbsent(userId,
                        key -> new HashMap<>(storedData.getSurfaceScenariosData().size()));

        for (Map.Entry<String, Map<String, Any>> surfaceChange : storedData.getSurfaceScenariosData().entrySet()) {

            Map<String, Any> surfaceData = scenarioSurfaces.computeIfAbsent(surfaceChange.getKey(),
                    key -> new HashMap<>(surfaceChange.getValue().size()));

            surfaceData.putAll(surfaceChange.getValue());
        }
    }

    @Override
    public void removeAllObjects(String uid) {
        userSettingsDB.remove(uid);
        userDevicesDB.remove(uid);
        scenariosDB.remove(uid);
        surfaceScenariosDB.remove(uid);
    }
}
