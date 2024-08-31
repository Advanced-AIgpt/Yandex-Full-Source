package ru.yandex.alice.memento.controller;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

import com.google.protobuf.Any;
import com.google.protobuf.Message;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.http.converter.HttpMessageConverter;

import ru.yandex.alice.memento.proto.DeviceConfigsProto;
import ru.yandex.alice.memento.proto.MementoApiProto;
import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey;
import ru.yandex.alice.memento.proto.MementoApiProto.EDeviceConfigKey;
import ru.yandex.alice.memento.proto.MementoApiProto.TConfigKeyAnyPair;
import ru.yandex.alice.memento.proto.MementoApiProto.TDeviceConfigs;
import ru.yandex.alice.memento.proto.MementoApiProto.TDeviceConfigsKeyAnyPair;
import ru.yandex.alice.memento.proto.MementoApiProto.TReqChangeUserObjects;
import ru.yandex.alice.memento.proto.MementoApiProto.TReqGetAllObjects;
import ru.yandex.alice.memento.proto.MementoApiProto.TReqGetUserObjects;
import ru.yandex.alice.memento.proto.MementoApiProto.TRespGetAllObjects;
import ru.yandex.alice.memento.proto.MementoApiProto.TRespGetUserObjects;
import ru.yandex.alice.memento.proto.MementoApiProto.TSurfaceScenarioData;
import ru.yandex.alice.memento.proto.UserConfigsProto;
import ru.yandex.alice.memento.storage.StoredData;
import ru.yandex.alice.memento.tvm.UnitTestTvmClient;
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders;
import ru.yandex.alice.protos.data.scenario.music.Topic;

import static java.util.Collections.emptyMap;
import static java.util.Collections.emptySet;
import static java.util.Comparator.comparing;
import static java.util.stream.Collectors.toList;
import static java.util.stream.Collectors.toMap;
import static org.junit.jupiter.api.Assertions.assertEquals;

class HttpIntegrationTest extends AbstractIntegrationTest {

    @Autowired
    private HttpMessageConverter<Message> messageConverter;
    private TestRestTemplate restTemplate;

    @LocalServerPort
    private int port;

    @Override
    @BeforeEach
    void setUp() {
        restTemplate = new TestRestTemplate(new RestTemplateBuilder().additionalMessageConverters(messageConverter));
        super.setUp();
    }

    @Override
    @AfterEach
    void tearDown() {
        super.tearDown();
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testGetAllSortOfData(boolean anonymous) {

        var actual = getSettings(anonymous,
                Set.of(EConfigKey.CK_CONFIG_FOR_TESTS),
                Set.of(EDeviceConfigKey.DCK_DUMMY),
                Set.of("test_scenario"),
                Set.of("test_scenario")
        );

        var expected = TRespGetUserObjects.newBuilder()
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_CONFIG_FOR_TESTS)
                        .setValue(Any.pack(DEFAULT_NEWS_CONFIG))
                )
                .addDevicesConfigs(TDeviceConfigs.newBuilder()
                        .setDeviceId(DUMMY_DEVICE_ID)
                        .addDeviceConfigs(TDeviceConfigsKeyAnyPair.newBuilder()
                                .setKey(EDeviceConfigKey.DCK_DUMMY)
                                .setValue(Any.newBuilder()
                                        .setTypeUrl("type.googleapis.com/ru.yandex.alice.memento.proto" +
                                                ".TDummyDeviceConfig")
                                        .build())
                                .build())
                        .build())
                .setVersion(versionInfo)
                .build();

        assertEquals(expected, actual);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testGetConfigForTestsNoRecords(boolean anonymous) {

        UserConfigsProto.TConfigForTests newsConfig = UserConfigsProto.TConfigForTests.newBuilder().setDefaultSource(
                "test_source").build();

        var actual = getUserSettings(anonymous, EConfigKey.CK_CONFIG_FOR_TESTS);

        var expected = TRespGetUserObjects.newBuilder()
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_CONFIG_FOR_TESTS)
                        .setValue(Any.pack(DEFAULT_NEWS_CONFIG))
                )
                .setVersion(versionInfo)
                .build();

        assertEquals(expected, actual);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testGetConfigForTests(boolean anonymous) {

        UserConfigsProto.TConfigForTests newsConfig = UserConfigsProto.TConfigForTests.newBuilder().setDefaultSource(
                "test_source").build();

        var userId = anonymous ? DUMMY_DEVICE_ID : DUMMY_USER_ID;
        storageDao.update(userId, new StoredData(Map.of(NEWS_KEY, Any.pack(newsConfig)), emptyMap(),
                emptyMap(), emptyMap()), anonymous);

        var actual = getUserSettings(anonymous, EConfigKey.CK_CONFIG_FOR_TESTS);

        var expected = TRespGetUserObjects.newBuilder()
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_CONFIG_FOR_TESTS)
                        .setValue(Any.pack(newsConfig))
                )
                .setVersion(versionInfo)
                .build();

        assertEquals(expected, actual);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testGetConfigForTestsOnly(boolean anonymous) {

        var userId = anonymous ? DUMMY_DEVICE_ID : DUMMY_USER_ID;

        var initial = Map.of(
                EConfigKey.CK_CONFIG_FOR_TESTS,
                Any.pack(UserConfigsProto.TConfigForTests.newBuilder().setDefaultSource("test_source").build()),
                EConfigKey.CK_MORNING_SHOW_TOPICS, Any.pack(
                        UserConfigsProto.TMorningShowTopicsConfig.newBuilder()
                                .addTopics(Topic.TTopic.newBuilder()
                                        .setPodcast("1")
                                ).build()
                )
        );

        settingsStorage.updateUserSettings(userId, initial, anonymous);

        TRespGetUserObjects actual = getUserSettings(anonymous, EConfigKey.CK_CONFIG_FOR_TESTS);

        var expected = TRespGetUserObjects.newBuilder()
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_CONFIG_FOR_TESTS)
                        .setValue(initial.get(EConfigKey.CK_CONFIG_FOR_TESTS))
                )
                .setVersion(versionInfo)
                .build();
        assertEquals(expected, actual);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testGetNewsAndShowConfigs(boolean anonymous) {
        var userId = anonymous ? DUMMY_DEVICE_ID : DUMMY_USER_ID;

        var initial = Map.of(
                EConfigKey.CK_CONFIG_FOR_TESTS,
                Any.pack(UserConfigsProto.TConfigForTests.newBuilder().setDefaultSource("test_source").build()),
                EConfigKey.CK_MORNING_SHOW_TOPICS, Any.pack(
                        UserConfigsProto.TMorningShowTopicsConfig.newBuilder()
                                .addTopics(Topic.TTopic.newBuilder()
                                        .setPodcast("1")
                                )
                                .build()
                )
        );

        settingsStorage.updateUserSettings(userId, initial, anonymous);

        TRespGetUserObjects actual =
                getUserSettings(anonymous, EConfigKey.CK_CONFIG_FOR_TESTS, EConfigKey.CK_MORNING_SHOW_TOPICS);

        var expected = TRespGetUserObjects.newBuilder()
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_MORNING_SHOW_TOPICS)
                        .setValue(initial.get(EConfigKey.CK_MORNING_SHOW_TOPICS))
                )
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_CONFIG_FOR_TESTS)
                        .setValue(initial.get(EConfigKey.CK_CONFIG_FOR_TESTS))
                )
                .setVersion(versionInfo)
                .build();
        assertEquals(expected.getUserConfigsList().stream()
                        .sorted(comparing(TConfigKeyAnyPair::getKey))
                        .collect(toList()),
                actual.getUserConfigsList().stream()
                        .sorted(comparing(TConfigKeyAnyPair::getKey))
                        .collect(toList()));
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testGetNewsAndShowConfigsWithEmptyShow(boolean anonymous) {
        var userId = anonymous ? DUMMY_DEVICE_ID : DUMMY_USER_ID;
        var initial = Map.of(
                EConfigKey.CK_CONFIG_FOR_TESTS,
                Any.pack(UserConfigsProto.TConfigForTests.newBuilder().setDefaultSource("test_source").build()),
                EConfigKey.CK_MORNING_SHOW_TOPICS,
                Any.pack(UserConfigsProto.TMorningShowTopicsConfig.newBuilder().build())
        );

        settingsStorage.updateUserSettings(userId, initial, anonymous);

        TRespGetUserObjects actual =
                getUserSettings(anonymous, EConfigKey.CK_CONFIG_FOR_TESTS, EConfigKey.CK_MORNING_SHOW_TOPICS);

        var expected = TRespGetUserObjects.newBuilder()
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_MORNING_SHOW_TOPICS)
                        .setValue(initial.get(EConfigKey.CK_MORNING_SHOW_TOPICS))
                )
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_CONFIG_FOR_TESTS)
                        .setValue(initial.get(EConfigKey.CK_CONFIG_FOR_TESTS))
                )
                .setVersion(versionInfo)
                .build();
        assertEquals(expected.getUserConfigsList().stream()
                        .sorted(comparing(TConfigKeyAnyPair::getKey))
                        .collect(toList()),
                actual.getUserConfigsList().stream()
                        .sorted(comparing(TConfigKeyAnyPair::getKey))
                        .collect(toList()));
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testStoreShowWithExistingNewsConfig(boolean anonymous) {

        var userId = anonymous ? DUMMY_DEVICE_ID : DUMMY_USER_ID;
        UserConfigsProto.TConfigForTests newsConfig = UserConfigsProto.TConfigForTests.newBuilder().setDefaultSource(
                "test_source").build();
        var initial = Map.of(EConfigKey.CK_CONFIG_FOR_TESTS, Any.pack(newsConfig));

        settingsStorage.updateUserSettings(userId, initial, anonymous);

        UserConfigsProto.TMorningShowTopicsConfig morningShowConfig =
                UserConfigsProto.TMorningShowTopicsConfig.newBuilder()
                        .addTopics(Topic.TTopic.newBuilder()
                                .setPodcast("1")
                        )
                        .build();

        storeUserSettings(anonymous, Map.of(EConfigKey.CK_MORNING_SHOW_TOPICS, morningShowConfig));

        TRespGetUserObjects actual =
                getUserSettings(anonymous, EConfigKey.CK_CONFIG_FOR_TESTS, EConfigKey.CK_MORNING_SHOW_TOPICS);

        var expected = TRespGetUserObjects.newBuilder()
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_MORNING_SHOW_TOPICS)
                        .setValue(Any.pack(morningShowConfig))
                )
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_CONFIG_FOR_TESTS)
                        .setValue(Any.pack(newsConfig))
                )
                .setVersion(versionInfo)
                .build();
        assertEquals(expected.getUserConfigsList().stream()
                        .sorted(comparing(TConfigKeyAnyPair::getKey)).collect(toList()),
                actual.getUserConfigsList().stream()
                        .sorted(comparing(TConfigKeyAnyPair::getKey)).collect(toList()));
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testStoreShowWithExistingNewsConfigXUid(boolean anonymous) {
        var userId = anonymous ? DUMMY_DEVICE_ID : DUMMY_USER_ID;

        UserConfigsProto.TConfigForTests newsConfig = UserConfigsProto.TConfigForTests.newBuilder().setDefaultSource(
                "test_source").build();
        var initial = Map.of(EConfigKey.CK_CONFIG_FOR_TESTS, Any.pack(newsConfig));

        settingsStorage.updateUserSettings(userId, initial, anonymous);

        UserConfigsProto.TMorningShowTopicsConfig morningShowConfig =
                UserConfigsProto.TMorningShowTopicsConfig.newBuilder()
                        .addTopics(Topic.TTopic.newBuilder()
                                .setPodcast("1")
                        )
                        .build();

        storeUserSettings(anonymous, Map.of(EConfigKey.CK_MORNING_SHOW_TOPICS, morningShowConfig));

        TRespGetUserObjects actual =
                getUserSettings(anonymous, EConfigKey.CK_CONFIG_FOR_TESTS, EConfigKey.CK_MORNING_SHOW_TOPICS);

        var expected = TRespGetUserObjects.newBuilder()
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_MORNING_SHOW_TOPICS)
                        .setValue(Any.pack(morningShowConfig))
                )
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_CONFIG_FOR_TESTS)
                        .setValue(Any.pack(newsConfig))
                )
                .setVersion(versionInfo)
                .build();
        assertEquals(expected.getUserConfigsList().stream()
                        .sorted(comparing(TConfigKeyAnyPair::getKey)).collect(toList()),
                actual.getUserConfigsList().stream()
                        .sorted(comparing(TConfigKeyAnyPair::getKey)).collect(toList()));
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testStoreUserAndDeviceSettings(boolean anonymous) {
        UserConfigsProto.TMorningShowTopicsConfig morningShowConfig =
                UserConfigsProto.TMorningShowTopicsConfig.newBuilder()
                        .addTopics(Topic.TTopic.newBuilder()
                                .setPodcast("1")
                        )
                        .build();

        DeviceConfigsProto.TDummyDeviceConfig dummyDeviceConfig =
                DeviceConfigsProto.TDummyDeviceConfig.newBuilder().setSomeField(1).build();

        storeUserAndDeviceSettings(
                anonymous,
                Map.of(EConfigKey.CK_MORNING_SHOW_TOPICS, morningShowConfig),
                Map.of(EDeviceConfigKey.DCK_DUMMY, dummyDeviceConfig)
        );

        var actualDeviceSettings = getDeviceSettings(anonymous, EDeviceConfigKey.DCK_DUMMY);

        var expected = TRespGetUserObjects.newBuilder()
                .addDevicesConfigs(TDeviceConfigs.newBuilder()
                        .setDeviceId(DUMMY_DEVICE_ID)
                        .addDeviceConfigs(TDeviceConfigsKeyAnyPair.newBuilder()
                                .setKey(EDeviceConfigKey.DCK_DUMMY)
                                .setValue(Any.pack(dummyDeviceConfig))
                        )
                )
                .setVersion(versionInfo)
                .build();
        assertEquals(expected, actualDeviceSettings);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testStoreUserAndDeviceSettingsAndScenarios(boolean anonymous) {
        UserConfigsProto.TMorningShowTopicsConfig morningShowConfig =
                UserConfigsProto.TMorningShowTopicsConfig.newBuilder()
                        .addTopics(Topic.TTopic.newBuilder()
                                .setPodcast("1")
                        )
                        .build();

        DeviceConfigsProto.TDummyDeviceConfig dummyDeviceConfig =
                DeviceConfigsProto.TDummyDeviceConfig.newBuilder().setSomeField(1).build();

        storeUserAndDeviceSettings(
                anonymous,
                Map.of(EConfigKey.CK_MORNING_SHOW_TOPICS, morningShowConfig),
                Map.of(EDeviceConfigKey.DCK_DUMMY, dummyDeviceConfig),
                Map.of("test_scenario", morningShowConfig),
                Map.of("test_scenario", morningShowConfig)
        );

        var actualDeviceSettings = getSettings(
                anonymous,
                Set.of(EConfigKey.CK_MORNING_SHOW_TOPICS),
                Set.of(EDeviceConfigKey.DCK_DUMMY),
                Set.of("test_scenario"),
                Set.of("test_scenario")
        );

        var expected = TRespGetUserObjects.newBuilder()
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_MORNING_SHOW_TOPICS)
                        .setValue(Any.pack(morningShowConfig))
                )
                .addDevicesConfigs(TDeviceConfigs.newBuilder()
                        .setDeviceId(DUMMY_DEVICE_ID)
                        .addDeviceConfigs(TDeviceConfigsKeyAnyPair.newBuilder()
                                .setKey(EDeviceConfigKey.DCK_DUMMY)
                                .setValue(Any.pack(dummyDeviceConfig))
                        )
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

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testStoreWithoutTvm(boolean anonymous) {

        HttpHeaders headers = new HttpHeaders();
        if (anonymous) {
            headers.add(SecurityHeaders.USER_TICKET_HEADER, "test-ticket");
        }
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        //headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, UnitTestTvmClient.SERVICE_TICKET);

        var changeBody = TReqChangeUserObjects.newBuilder().build();
        ResponseEntity<String> exchange = restTemplate.exchange(
                "http://localhost:" + port + "/update_objects",
                HttpMethod.POST, new HttpEntity<>(changeBody, headers), String.class);


        assertEquals(HttpStatus.FORBIDDEN, exchange.getStatusCode());
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testGetConfigForTestsWithoutTvm(boolean anonymous) {

        HttpHeaders headers = new HttpHeaders();
        if (anonymous) {
            headers.add(SecurityHeaders.USER_TICKET_HEADER, "test-ticket");
        }
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        //headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, UnitTestTvmClient.SERVICE_TICKET);
        UserConfigsProto.TConfigForTests newsConfig = UserConfigsProto.TConfigForTests.newBuilder().setDefaultSource(
                "test_source").build();

        var body = TReqGetUserObjects.newBuilder().build();
        ResponseEntity<TRespGetUserObjects> exchange = restTemplate.exchange(
                "http://localhost:" + port + "/get_objects",
                HttpMethod.POST, new HttpEntity<>(body, headers), TRespGetUserObjects.class);

        assertEquals(HttpStatus.FORBIDDEN, exchange.getStatusCode());
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    void testGetConfigForTestsOnlyJson(boolean anonymous) {
        var userId = anonymous ? DUMMY_DEVICE_ID : DUMMY_USER_ID;

        var initial = Map.of(
                EConfigKey.CK_CONFIG_FOR_TESTS,
                Any.pack(UserConfigsProto.TConfigForTests.newBuilder().setDefaultSource("test_source").build()),
                EConfigKey.CK_MORNING_SHOW_TOPICS, Any.pack(
                        UserConfigsProto.TMorningShowTopicsConfig.newBuilder()
                                .addTopics(Topic.TTopic.newBuilder()
                                        .setPodcast("1")
                                ).build()
                )
        );

        settingsStorage.updateUserSettings(userId, initial, anonymous);

        HttpHeaders headers = new HttpHeaders();
        if (!anonymous) {
            headers.add(SecurityHeaders.USER_TICKET_HEADER, "test-ticket");
        }
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        headers.add(HttpHeaders.CONTENT_TYPE, MediaType.APPLICATION_JSON_VALUE);
        headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, UnitTestTvmClient.SERVICE_TICKET);
        var body = TReqGetUserObjects.newBuilder()
                .addKeys(EConfigKey.CK_CONFIG_FOR_TESTS)
                .setCurrentSurfaceId(DUMMY_DEVICE_ID)
                .build();
        ResponseEntity<TRespGetUserObjects> exchange = restTemplate.exchange(
                "http://localhost:" + port + "/get_objects",
                HttpMethod.POST, new HttpEntity<>(body, headers), TRespGetUserObjects.class);

        var expected = TRespGetUserObjects.newBuilder()
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_CONFIG_FOR_TESTS)
                        .setValue(initial.get(EConfigKey.CK_CONFIG_FOR_TESTS))
                )
                .setVersion(versionInfo)
                .build();
        assertEquals(HttpStatus.OK, exchange.getStatusCode());
        assertEquals(expected, exchange.getBody());
    }


    protected TRespGetUserObjects getUserSettings(boolean anonymous, EConfigKey... userKeys) {
        return getSettings(anonymous, new HashSet<>(Arrays.asList(userKeys)), emptySet(), emptySet(), emptySet());
    }

    protected TRespGetUserObjects getDeviceSettings(boolean anonymous, EDeviceConfigKey... userKeys) {
        return getSettings(anonymous, emptySet(), new HashSet<>(Arrays.asList(userKeys)), emptySet(), emptySet());
    }

    protected TRespGetUserObjects getSettings(
            boolean anonymous,
            Set<EConfigKey> configKeys, Set<EDeviceConfigKey> deviceConfigKeys,
            Set<String> scenarios, Set<String> surfaceScenarios) {
        var builder = TReqGetUserObjects.newBuilder()
                .setCurrentSurfaceId(DUMMY_DEVICE_ID)
                .addAllKeys(configKeys)
                .addAllScenarioKeys(scenarios);
        if (!deviceConfigKeys.isEmpty()) {
            builder.addDevicesKeys(MementoApiProto.TDeviceKeys.newBuilder()
                    .setDeviceId(DUMMY_DEVICE_ID)
                    .addAllKeys(deviceConfigKeys));
        }

        if (!surfaceScenarios.isEmpty()) {
            builder.putSurfaceScenarioNames(DUMMY_DEVICE_ID, MementoApiProto.TScenarioNames.newBuilder()
                    .addAllScenarioName(surfaceScenarios)
                    .build());
        }

        HttpHeaders headers = new HttpHeaders();
        if (!anonymous) {
            headers.add("X-Uid", DUMMY_USER_ID);
        }
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, UnitTestTvmClient.SERVICE_TICKET);

        ResponseEntity<TRespGetUserObjects> exchange = restTemplate.exchange(
                "http://localhost:" + port + "/get_objects",
                HttpMethod.POST, new HttpEntity<>(builder.build(), headers), TRespGetUserObjects.class);

        assertEquals(HttpStatus.OK, exchange.getStatusCode());
        return Objects.requireNonNullElse(exchange.getBody(), TRespGetUserObjects.getDefaultInstance());
    }

    @Override
    protected TRespGetAllObjects getAllSettings(boolean anonymous) {
        var builder = TReqGetAllObjects.newBuilder()
                .addSurfaceId(DUMMY_DEVICE_ID)
                .setCurrentSurfaceId(DUMMY_DEVICE_ID);

        HttpHeaders headers = new HttpHeaders();
        if (!anonymous) {
            headers.add("X-Uid", DUMMY_USER_ID);
        }
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, UnitTestTvmClient.SERVICE_TICKET);

        ResponseEntity<TRespGetAllObjects> exchange = restTemplate.exchange(
                "http://localhost:" + port + "/get_all_objects",
                HttpMethod.POST, new HttpEntity<>(builder.build(), headers), TRespGetAllObjects.class);

        assertEquals(HttpStatus.OK, exchange.getStatusCode());
        return Objects.requireNonNullElse(exchange.getBody(), TRespGetAllObjects.getDefaultInstance());
    }

    @Override
    protected void clearUserData() {
        var builder = MementoApiProto.TClearUserData.newBuilder()
                .setPuid(Long.parseLong(DUMMY_USER_ID));

        HttpHeaders headers = new HttpHeaders();
        //headers.add("X-Uid", DUMMY_USER_ID);
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, UnitTestTvmClient.SERVICE_TICKET);

        ResponseEntity<String> exchange = restTemplate.exchange(
                "http://localhost:" + port + "/clear_user_data",
                HttpMethod.POST, new HttpEntity<>(builder.build(), headers), String.class);

        assertEquals(HttpStatus.OK, exchange.getStatusCode());
    }

    @Override
    protected void storeUserAndDeviceSettings(
            boolean anonymous,
            Map<EConfigKey, Message> userChanges, Map<EDeviceConfigKey, Message> deviceChanges,
            Map<String, Message> scenariosData, Map<String, Message> surfaceScenariosData
    ) {
        HttpHeaders headers = new HttpHeaders();

        if (!anonymous) {
            headers.add(SecurityHeaders.USER_TICKET_HEADER, "test-ticket");
        }
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, UnitTestTvmClient.SERVICE_TICKET);

        var changeBody = TReqChangeUserObjects.newBuilder()
                .setCurrentSurfaceId(DUMMY_DEVICE_ID)
                .addAllUserConfigs(userChanges.entrySet().stream()
                        .map(entry -> TConfigKeyAnyPair.newBuilder()
                                .setKey(entry.getKey())
                                .setValue(Any.pack(entry.getValue()))
                                .build()
                        ).collect(toList())
                )
                .addDevicesConfigs(deviceChanges.isEmpty() ? TDeviceConfigs.newBuilder() :
                        TDeviceConfigs.newBuilder()
                                .setDeviceId(DUMMY_DEVICE_ID)
                                .addAllDeviceConfigs(deviceChanges.entrySet().stream()
                                        .map(entry -> TDeviceConfigsKeyAnyPair.newBuilder()
                                                .setKey(entry.getKey())
                                                .setValue(Any.pack(entry.getValue()))
                                                .build()
                                        )
                                        .collect(Collectors.toList()))
                )
                .putAllScenarioData(scenariosData.entrySet().stream()
                        .collect(toMap(Map.Entry::getKey, e -> Any.pack(e.getValue())))
                )
                .putAllSurfaceScenarioData(surfaceScenariosData.isEmpty() ? emptyMap() :
                        Map.of(DUMMY_DEVICE_ID,
                                TSurfaceScenarioData.newBuilder()
                                        .putAllScenarioData(
                                                surfaceScenariosData.entrySet().stream()
                                                        .collect(toMap(Map.Entry::getKey, e -> Any.pack(e.getValue())))
                                        )
                                        .build())
                )
                .build();

        ResponseEntity<String> exchange = restTemplate.exchange(
                "http://localhost:" + port + "/update_objects",
                HttpMethod.POST, new HttpEntity<>(changeBody, headers), String.class);

        assertEquals(HttpStatus.OK, exchange.getStatusCode());

    }

}
