package ru.yandex.alice.memento.controller;

import java.util.Map;

import com.google.protobuf.Any;
import com.google.protobuf.Message;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.test.context.ActiveProfiles;

import ru.yandex.alice.memento.proto.DeviceConfigsProto.TDummyDeviceConfig;
import ru.yandex.alice.memento.proto.MementoApiProto;
import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey;
import ru.yandex.alice.memento.proto.MementoApiProto.EDeviceConfigKey;
import ru.yandex.alice.memento.proto.MementoApiProto.TRespGetAllObjects;
import ru.yandex.alice.memento.proto.MementoApiProto.TSurfaceScenarioData;
import ru.yandex.alice.memento.proto.MementoApiProto.VersionInfo;
import ru.yandex.alice.memento.proto.UserConfigsProto.TConfigForTests;
import ru.yandex.alice.memento.proto.UserConfigsProto.TMorningShowTopicsConfig;
import ru.yandex.alice.memento.proto.UserConfigsProto.TTtsWhisperConfig;
import ru.yandex.alice.memento.scanner.KeyMappingScanner;
import ru.yandex.alice.memento.settings.SettingsStorage;
import ru.yandex.alice.memento.storage.InMemoryStorageDao;
import ru.yandex.alice.memento.storage.StorageDao;
import ru.yandex.alice.memento.storage.TestStorageConfiguration;
import ru.yandex.alice.memento.storage.ydb.TestYdbConfiguration;
import ru.yandex.alice.memento.tvm.TestTvmConfiguration;
import ru.yandex.alice.memento.ydb.YdbTestUtils;
import ru.yandex.alice.paskills.common.ydb.YdbClient;
import ru.yandex.alice.protos.data.scenario.music.Topic.TTopic;

import static java.util.Collections.emptyMap;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

@SpringBootTest(classes = {TestTvmConfiguration.class, TestStorageConfiguration.class, TestYdbConfiguration.class},
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@ActiveProfiles("ut")
abstract class AbstractIntegrationTest {

    protected static final String DUMMY_USER_ID = "1";
    protected static final String DUMMY_DEVICE_ID = "dev1";
    protected static final String NEWS_KEY =
            EConfigKey.CK_CONFIG_FOR_TESTS.getValueDescriptor().getOptions().getExtension(MementoApiProto.dbKey);
    protected static final String MORNING_SHOW_KEY =
            EConfigKey.CK_MORNING_SHOW.getValueDescriptor().getOptions().getExtension(MementoApiProto.dbKey);

    protected static final TConfigForTests DEFAULT_NEWS_CONFIG = TConfigForTests.newBuilder()
            .setDefaultSource("a")
            .build();

    @Autowired
    protected YdbClient ydbClient;
    @Autowired
    protected SettingsStorage settingsStorage;
    @Autowired
    protected StorageDao storageDao;
    @Autowired
    protected VersionInfo versionInfo;
    @Autowired
    protected KeyMappingScanner scanner;

    private MementoApiProto.TUserConfigs defaultUserConfigs;

    private MementoApiProto.TSurfaceConfig defaultSurfaceConfigs;

    @BeforeEach
    void setUp() {
        clear();

        var builder = MementoApiProto.TUserConfigs.newBuilder();

        scanner.getUserConfigExplicitDefaults().forEach((key, value) -> scanner.setValue(key, builder, value));
        defaultUserConfigs = builder.build();

        var surfaceBuilder = MementoApiProto.TSurfaceConfig.newBuilder();

        scanner.getDeviceConfigExplicitDefaults().forEach((key, value) -> scanner.setValue(key, surfaceBuilder, value));
        defaultSurfaceConfigs = surfaceBuilder.build();
    }

    void tearDown() {
        clear();
    }

    private void clear() {
        if (storageDao instanceof InMemoryStorageDao) {
            ((InMemoryStorageDao) storageDao).clear();
        } else {
            YdbTestUtils.clearTable(ydbClient, "user_settings");
            YdbTestUtils.clearTable(ydbClient, "user_device_settings");
            YdbTestUtils.clearTable(ydbClient, "user_scenario_data");
            YdbTestUtils.clearTable(ydbClient, "user_surface_scenario_data");
            YdbTestUtils.clearTable(ydbClient, "user_settings_anonymous");
            YdbTestUtils.clearTable(ydbClient, "user_device_settings_anonymous");
            YdbTestUtils.clearTable(ydbClient, "user_scenario_data_anonymous");
            YdbTestUtils.clearTable(ydbClient, "user_surface_scenario_data_anonymous");
        }
    }

    @Test
    void testDefaultsAreSet() {
        assertNotEquals(MementoApiProto.TUserConfigs.getDefaultInstance(), defaultUserConfigs);
    }

    // TODO: remove the test after WHISPER default changes to false
    @Test
    void testWhisperDefault() {
        TRespGetAllObjects actual = getAllSettings(true);

        assertTrue(actual.getUserConfigs().getTtsWhisperConfig().getEnabled());
    }

    @Test
    void testWhisperEnabled() {

        var initial = Map.of(
                EConfigKey.CK_TTS_WHISPER,
                Any.pack(TTtsWhisperConfig.newBuilder().setEnabled(true).build())
        );

        settingsStorage.updateUserSettings(DUMMY_DEVICE_ID, initial, true);

        TRespGetAllObjects actual = getAllSettings(true);

        assertTrue(actual.getUserConfigs().getTtsWhisperConfig().getEnabled());
    }

    @Test
    void testWhisperDisabled() {

        var initial = Map.of(
                EConfigKey.CK_TTS_WHISPER,
                Any.pack(TTtsWhisperConfig.newBuilder().setEnabled(false).build())
        );

        settingsStorage.updateUserSettings(DUMMY_DEVICE_ID, initial, true);

        TRespGetAllObjects actual = getAllSettings(true);

        assertFalse(actual.getUserConfigs().getTtsWhisperConfig().getEnabled());
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    void testGetNewsAndShowAfterDelete(boolean anonymous) {
        var userId = anonymous ? DUMMY_DEVICE_ID : DUMMY_USER_ID;
        var initial = Map.of(
                EConfigKey.CK_CONFIG_FOR_TESTS,
                Any.pack(TConfigForTests.newBuilder().setDefaultSource("test_source").build()),
                EConfigKey.CK_MORNING_SHOW_TOPICS, Any.pack(
                        TMorningShowTopicsConfig.newBuilder()
                                .addTopics(TTopic.newBuilder()
                                        .setPodcast("1")
                                )
                                .build()
                )
        );
        settingsStorage.updateUserSettings(userId, initial, anonymous);

        // only for logged in users
        settingsStorage.removeAllObjects(userId);
        TRespGetAllObjects allSettings = getAllSettings(anonymous);
        TRespGetAllObjects defaultInstance = TRespGetAllObjects.newBuilder()
                .setUserConfigs(defaultUserConfigs)
                .build();
        assertEquals(defaultInstance.getUserConfigs(), allSettings.getUserConfigs());
        assertEquals(defaultInstance.getScenarioDataCount(), allSettings.getScenarioDataCount());
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testStoreUserAndDeviceSettingsAndScenariosForProxy(boolean anonymous) {
        TMorningShowTopicsConfig morningShowConfig = TMorningShowTopicsConfig.newBuilder()
                .addTopics(TTopic.newBuilder()
                        .setPodcast("1")
                )
                .build();

        TDummyDeviceConfig dummyDeviceConfig =
                TDummyDeviceConfig.newBuilder().setSomeField(1).build();

        storeUserAndDeviceSettings(
                anonymous,
                Map.of(EConfigKey.CK_MORNING_SHOW_TOPICS, morningShowConfig),
                Map.of(EDeviceConfigKey.DCK_DUMMY, dummyDeviceConfig),
                Map.of("test_scenario", morningShowConfig),
                Map.of("test_scenario", morningShowConfig)
        );

        var actualDeviceSettings = getAllSettings(anonymous);

        var expected = TRespGetAllObjects.newBuilder()
                .setUserConfigs(defaultUserConfigs.toBuilder()
                        .setMorningShowTopicsConfig(morningShowConfig)
                )
                .putAllSurfaceConfigs(Map.of(DUMMY_DEVICE_ID, defaultSurfaceConfigs.toBuilder()
                        .setDummyDeviceConfig(dummyDeviceConfig).build())
                )
                .putScenarioData("test_scenario", Any.pack(morningShowConfig))
                .putSurfaceScenarioData(DUMMY_DEVICE_ID, TSurfaceScenarioData.newBuilder()
                        .putScenarioData("test_scenario", Any.pack(morningShowConfig))
                        .build()
                )
                .setVersion(versionInfo)
                .build();
        assertEquals(expected, actualDeviceSettings);
    }

    @Test
    void testClearAllData() {
        testStoreUserAndDeviceSettingsAndScenariosForProxy(false);

        clearUserData();

        var actualDeviceSettings = getAllSettings(false);

        var expected = TRespGetAllObjects.newBuilder()
                .putAllSurfaceConfigs(Map.of(DUMMY_DEVICE_ID, defaultSurfaceConfigs))
                .setUserConfigs(defaultUserConfigs)
                .setVersion(versionInfo)
                .build();
        assertEquals(expected, actualDeviceSettings);
    }


    protected abstract TRespGetAllObjects getAllSettings(boolean anonymous);

    protected abstract void clearUserData();

    protected void storeUserSettings(boolean anonymous, Map<EConfigKey, Message> userChanges) {
        storeUserAndDeviceSettings(anonymous, userChanges, emptyMap());
    }

    protected void storeUserAndDeviceSettings(
            boolean anonymous,
            Map<EConfigKey, Message> userChanges, Map<EDeviceConfigKey, Message> deviceChanges
    ) {
        storeUserAndDeviceSettings(anonymous, userChanges, deviceChanges, emptyMap(), emptyMap());
    }

    protected abstract void storeUserAndDeviceSettings(
            boolean anonymous,
            Map<EConfigKey, Message> userChanges, Map<EDeviceConfigKey, Message> deviceChanges,
            Map<String, Message> scenariosData, Map<String, Message> surfaceScenariosData
    );

}
