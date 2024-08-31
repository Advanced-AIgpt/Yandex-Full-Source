package ru.yandex.alice.paskill.dialogovo.service.appmetrica;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.List;
import java.util.Optional;

import com.google.common.io.Files;
import com.google.protobuf.TextFormat;
import okhttp3.mockwebserver.MockResponse;
import okhttp3.mockwebserver.MockWebServer;
import okhttp3.mockwebserver.RecordedRequest;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.converter.json.MappingJackson2HttpMessageConverter;
import org.springframework.util.MimeTypeUtils;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.LocationInfo;
import ru.yandex.alice.kronstadt.server.http.CustomProtobufHttpMessageConverter;
import ru.yandex.alice.paskill.dialogovo.config.AppMetricaConfig;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.utils.CryptoUtils;
import ru.yandex.alice.paskill.dialogovo.utils.executor.TestExecutorsFactory;
import ru.yandex.metrika.appmetrica.proto.AppMetricaProto;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;

class AppMetricaServiceImplTest {

    private static final String ACCEPTED_RESPONSE = "{\"status\":\"accepted\"}";
    private AppMetricaServiceImpl appMetricaService;
    private MockWebServer server;
    private String secret = "secret";
    private String wrongSecret = "wrongSecret";
    private String appMetricaApiKey = "key";
    private String uuid = "uuid";
    private RestTemplate restTemplate;
    private InMemoryAppMetricaFirstUserEventDaoImpl appMetricaFirstUserEventDao;
    private static final String SKILL_ID = "SKILL_ID";

    @BeforeEach
    void setUp() throws IOException {
        server = new MockWebServer();
        server.start();

        restTemplate = Mockito.spy(new RestTemplate(List.of(new CustomProtobufHttpMessageConverter(null, null),
                new MappingJackson2HttpMessageConverter())));

        appMetricaFirstUserEventDao = new InMemoryAppMetricaFirstUserEventDaoImpl();
        appMetricaService = new AppMetricaServiceImpl(
                new AppMetricaConfig(server.url("/").url().toString(), 2000, 200), "secret",
                TestExecutorsFactory.newSingleThreadExecutor(), restTemplate, appMetricaFirstUserEventDao, 0
        );
    }

    @AfterEach
    void tearDown() throws IOException {
        server.shutdown();
        appMetricaFirstUserEventDao.clear();
    }

    @Test
    void testEventWithNewSession() throws InterruptedException, IOException, AppMetricaServiceException {
        server.enqueue(new MockResponse()
                .setBody(ACCEPTED_RESPONSE)
                .setHeader(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE));

        String appMetricaApiKeyEncrypted = CryptoUtils.encrypt(secret, appMetricaApiKey);
        appMetricaFirstUserEventDao.saveFirstUserEvent(appMetricaApiKeyEncrypted, SKILL_ID, uuid, Instant.now());

        var appMetricaEvent = AppMetricaEvent.builder()
                .events(List.of(new MetricaEvent("event")))
                .uid(Optional.empty())
                .clientInfo(ClientInfo.builder()
                        .lang("ru")
                        .osVersion("1.2.3")
                        .deviceModel("model1")
                        .deviceManufacturer("manufac")
                        .appVersion("appVer")
                        .deviceId(Optional.empty())
                        .platform("platform")
                        .build())
                .session(Session.create(
                        "10000",
                        0,
                        Instant.ofEpochMilli(1000),
                        false,
                        Session.ProactiveSkillExitState.createEmpty(),
                        ActivationSourceType.DIRECT))
                .eventTime(Instant.ofEpochMilli(1000000))
                .locationInfo(Optional.of(new LocationInfo(1.0d, 1.0d, 500)))
                .skillId(SKILL_ID)
                .skillLook(SkillInfo.Look.INTERNAL)
                .build();

        List<AppMetricaProto.ReportMessage.Session.Event> events = List.of(
                AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                        .setNumberInSession(1)
                        .setTime(0)
                        .setType(2)
                        .build(),
                AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                        .setNumberInSession(2)
                        .setTime(0)
                        .setType(4)
                        .setName("event")
                        .build()
        );

        AppMetricaServiceImpl.AppMetricaResponse appMetricaResponse = appMetricaService.sendEvents(
                uuid,
                appMetricaApiKeyEncrypted,
                appMetricaEvent,
                events,
                false);

        RecordedRequest recordedRequest = server.takeRequest();

        CharSequence protoRequestText = Files.asCharSource(new File("src/test/resources/appmetrica/new_session_proto" +
                ".txt"), StandardCharsets.UTF_8).read();

        AppMetricaProto.ReportMessage.Builder builder = AppMetricaProto.ReportMessage.newBuilder();

        TextFormat.getParser().merge(protoRequestText, builder);
        AppMetricaProto.ReportMessage reportMessage = builder.build();

        HttpHeaders headers = new HttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, "application/x-protobuf");
        HttpEntity<AppMetricaProto.ReportMessage> reportMessageHttpEntity = new HttpEntity<>(reportMessage, headers);

        Mockito.verify(restTemplate).postForObject(
                any(),
                eq(reportMessageHttpEntity),
                eq(AppMetricaServiceImpl.AppMetricaResponse.class));

        assertEquals(recordedRequest.getMethod(), "POST");
        assertEquals(recordedRequest.getPath(), "/report?api_key_128=key&uuid=uuid&deviceid" +
                "=ef7c876f00f3acddd00fa671f52d0b1f&analytics_sdk_version=123&locale=ru&app_id=ru.yandex.alice.paskill" +
                ".dialogovo&app_platform=platform&os_version=1.2" +
                ".3&model=model1&manufacturer=manufac&device_type=phone&app_version_name=appVer");

        assertEquals(new AppMetricaServiceImpl.AppMetricaResponse("accepted"), appMetricaResponse);
    }

    @Test
    void testEventSessionContinues() throws InterruptedException, IOException, AppMetricaServiceException {
        server.enqueue(new MockResponse()
                .setBody(ACCEPTED_RESPONSE)
                .setHeader(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE));

        String appMetricaApiKeyEncrypted = CryptoUtils.encrypt(secret, appMetricaApiKey);
        appMetricaFirstUserEventDao.saveFirstUserEvent(appMetricaApiKeyEncrypted, SKILL_ID, uuid, Instant.now());

        var appMetricaEvent = AppMetricaEvent.builder()
                .events(List.of(new MetricaEvent("event")))
                .uid(Optional.empty())
                .clientInfo(ClientInfo.builder()
                        .lang("ru")
                        .osVersion("1.2.3")
                        .deviceModel("model1")
                        .deviceManufacturer("manufac")
                        .appVersion("appVer")
                        .platform("platform")
                        .deviceId(Optional.of("device_id"))
                        .build())
                .locationInfo(Optional.empty())
                .session(Session.create(
                        "10000",
                        3,
                        Instant.ofEpochMilli(1000),
                        false,
                        Session.ProactiveSkillExitState.createEmpty(),
                        ActivationSourceType.DIRECT))
                .eventTime(Instant.ofEpochMilli(20000))
                .skillId(SKILL_ID)
                .skillLook(SkillInfo.Look.INTERNAL)
                .build();

        List<AppMetricaProto.ReportMessage.Session.Event> events = List.of(
                AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                        .setNumberInSession(1)
                        .setTime(19)
                        .setType(7)
                        .build(),
                AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                        .setNumberInSession(2)
                        .setTime(19)
                        .setType(4)
                        .setName("event")
                        .build()
        );

        AppMetricaServiceImpl.AppMetricaResponse appMetricaResponse = appMetricaService.sendEvents(
                uuid,
                appMetricaApiKeyEncrypted,
                appMetricaEvent,
                events,
                false);

        RecordedRequest recordedRequest = server.takeRequest();

        CharSequence protoRequestText = Files.asCharSource(
                new File("src/test/resources/appmetrica/session_continues_proto.txt"), StandardCharsets.UTF_8
        ).read();

        AppMetricaProto.ReportMessage.Builder builder = AppMetricaProto.ReportMessage.newBuilder();

        TextFormat.getParser().merge(protoRequestText, builder);
        AppMetricaProto.ReportMessage reportMessage = builder.build();

        HttpHeaders headers = new HttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, "application/x-protobuf");
        HttpEntity<AppMetricaProto.ReportMessage> reportMessageHttpEntity = new HttpEntity<>(reportMessage, headers);

        Mockito.verify(restTemplate).postForObject(
                any(),
                eq(reportMessageHttpEntity),
                eq(AppMetricaServiceImpl.AppMetricaResponse.class));

        assertEquals(recordedRequest.getMethod(), "POST");
        assertEquals(recordedRequest.getPath(), "/report?api_key_128=key&uuid=uuid&deviceid=device_id" +
                "&analytics_sdk_version=123&locale=ru&app_id=ru.yandex.alice.paskill" +
                ".dialogovo&app_platform=platform&os_version=1.2" +
                ".3&model=model1&manufacturer=manufac&device_type=phone&app_version_name=appVer");

        assertEquals(new AppMetricaServiceImpl.AppMetricaResponse("accepted"), appMetricaResponse);
    }

    @Test
    void testEventSessionEnds() throws InterruptedException, IOException, AppMetricaServiceException {
        server.enqueue(new MockResponse()
                .setBody(ACCEPTED_RESPONSE)
                .setHeader(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE));

        String appMetricaApiKeyEncrypted = CryptoUtils.encrypt(secret, appMetricaApiKey);
        appMetricaFirstUserEventDao.saveFirstUserEvent(appMetricaApiKeyEncrypted, SKILL_ID, uuid, Instant.now());

        var appMetricaEvents = AppMetricaEvent.builder()
                .events(List.of(new MetricaEvent("event")))
                .uid(Optional.empty())
                .clientInfo(ClientInfo.builder()
                        .lang("ru")
                        .osVersion("1.2.3")
                        .deviceModel("model1")
                        .deviceManufacturer("manufac")
                        .appVersion("appVer")
                        .platform("platform")
                        .deviceId(Optional.of("device_id"))
                        .build())
                .locationInfo(Optional.empty())
                .session(Session.create(
                        "10000",
                        3, Instant.ofEpochMilli(1000),
                        false,
                        Session.ProactiveSkillExitState.createEmpty(),
                        ActivationSourceType.DIRECT))
                .eventTime(Instant.ofEpochMilli(20000))
                .skillId(SKILL_ID)
                .skillLook(SkillInfo.Look.INTERNAL)
                .endSession(true)
                .build();

        List<AppMetricaProto.ReportMessage.Session.Event> events = List.of(
                AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                        .setNumberInSession(1)
                        .setTime(19)
                        .setType(7)
                        .build()
        );

        AppMetricaServiceImpl.AppMetricaResponse appMetricaResponse = appMetricaService.sendEvents(
                uuid,
                appMetricaApiKeyEncrypted,
                appMetricaEvents,
                events,
                false);

        RecordedRequest recordedRequest = server.takeRequest();

        CharSequence protoRequestText = Files.asCharSource(
                new File("src/test/resources/appmetrica/end_session_proto.txt"), StandardCharsets.UTF_8
        ).read();

        AppMetricaProto.ReportMessage.Builder builder = AppMetricaProto.ReportMessage.newBuilder();

        TextFormat.getParser().merge(protoRequestText, builder);
        AppMetricaProto.ReportMessage reportMessage = builder.build();

        HttpHeaders headers = new HttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, "application/x-protobuf");
        HttpEntity<AppMetricaProto.ReportMessage> reportMessageHttpEntity = new HttpEntity<>(reportMessage, headers);

        Mockito.verify(restTemplate).postForObject(
                any(),
                eq(reportMessageHttpEntity),
                eq(AppMetricaServiceImpl.AppMetricaResponse.class));

        assertEquals(recordedRequest.getMethod(), "POST");
        assertEquals(recordedRequest.getPath(), "/report?api_key_128=key&uuid=uuid&deviceid=device_id" +
                "&analytics_sdk_version=123&locale=ru&app_id=ru.yandex.alice.paskill" +
                ".dialogovo&app_platform=platform&os_version=1.2" +
                ".3&model=model1&manufacturer=manufac&device_type=phone&app_version_name=appVer");

        assertEquals(new AppMetricaServiceImpl.AppMetricaResponse("accepted"), appMetricaResponse);
    }

    @Test
    void testEventWithNewUser() throws InterruptedException, IOException, AppMetricaServiceException {
        server.enqueue(new MockResponse()
                .setBody(ACCEPTED_RESPONSE)
                .setHeader(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE));

        var appMetricaEvent = AppMetricaEvent.builder()
                .events(List.of(new MetricaEvent("event")))
                .uid(Optional.empty())
                .clientInfo(ClientInfo.builder()
                        .lang("ru")
                        .osVersion("1.2.3")
                        .deviceModel("model1")
                        .deviceManufacturer("manufac")
                        .appVersion("appVer")
                        .deviceId(Optional.empty())
                        .platform("platform")
                        .build())
                .session(Session.create(
                        "10000",
                        0,
                        Instant.ofEpochMilli(1000),
                        false,
                        Session.ProactiveSkillExitState.createEmpty(),
                        ActivationSourceType.DIRECT))
                .eventTime(Instant.ofEpochMilli(1000000))
                .locationInfo(Optional.of(new LocationInfo(1.0d, 1.0d, 500)))
                .skillId(SKILL_ID)
                .skillLook(SkillInfo.Look.INTERNAL)
                .build();

        List<AppMetricaProto.ReportMessage.Session.Event> events = List.of(
                AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                        .setNumberInSession(1)
                        .setTime(0)
                        .setType(2)
                        .build(),
                AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                        .setNumberInSession(2)
                        .setTime(0)
                        .setType(4)
                        .setName("event")
                        .build()
        );

        String appMetricaApiKeyEncrypted = CryptoUtils.encrypt(secret, appMetricaApiKey);
        AppMetricaServiceImpl.AppMetricaResponse appMetricaResponse = appMetricaService.sendEvents(
                uuid,
                appMetricaApiKeyEncrypted,
                appMetricaEvent,
                events,
                false);

        RecordedRequest recordedRequest = server.takeRequest();

        CharSequence protoRequestText = Files.asCharSource(
                new File("src/test/resources/appmetrica/new_user_proto.txt"), StandardCharsets.UTF_8
        ).read();

        AppMetricaProto.ReportMessage.Builder builder = AppMetricaProto.ReportMessage.newBuilder();

        TextFormat.getParser().merge(protoRequestText, builder);
        AppMetricaProto.ReportMessage reportMessage = builder.build();

        HttpHeaders headers = new HttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, "application/x-protobuf");
        HttpEntity<AppMetricaProto.ReportMessage> reportMessageHttpEntity = new HttpEntity<>(reportMessage, headers);

        Mockito.verify(restTemplate).postForObject(
                any(),
                eq(reportMessageHttpEntity),
                eq(AppMetricaServiceImpl.AppMetricaResponse.class));

        assertEquals(recordedRequest.getMethod(), "POST");
        assertEquals(recordedRequest.getPath(), "/report?api_key_128=key&uuid=uuid&deviceid" +
                "=ef7c876f00f3acddd00fa671f52d0b1f&analytics_sdk_version=123&locale=ru&app_id=ru.yandex.alice.paskill" +
                ".dialogovo&app_platform=platform&os_version=1.2" +
                ".3&model=model1&manufacturer=manufac&device_type=phone&app_version_name=appVer");

        assertEquals(new AppMetricaServiceImpl.AppMetricaResponse("accepted"), appMetricaResponse);
    }

    @Test
    void testEventsWithTimestampCounter() throws InterruptedException, IOException, AppMetricaServiceException {
        server.enqueue(new MockResponse()
                .setBody(ACCEPTED_RESPONSE)
                .setHeader(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE));

        String appMetricaApiKeyEncrypted = CryptoUtils.encrypt(secret, appMetricaApiKey);
        appMetricaFirstUserEventDao.saveFirstUserEvent(appMetricaApiKeyEncrypted, SKILL_ID, uuid, Instant.now());
        Instant eventTime = Instant.ofEpochMilli(20000);

        var appMetricaEvent = AppMetricaEvent.builder()
                .events(List.of(new MetricaEvent("event")))
                .uid(Optional.empty())
                .clientInfo(ClientInfo.builder()
                        .lang("ru")
                        .osVersion("1.2.3")
                        .deviceModel("model1")
                        .deviceManufacturer("manufac")
                        .appVersion("appVer")
                        .platform("platform")
                        .deviceId(Optional.of("device_id"))
                        .build())
                .locationInfo(Optional.empty())
                .session(Session.create(
                        "10000",
                        3,
                        Instant.ofEpochMilli(1000),
                        false,
                        Session.ProactiveSkillExitState.createEmpty(),
                        ActivationSourceType.DIRECT))
                .eventTime(Instant.ofEpochMilli(20000))
                .skillId(SKILL_ID)
                .skillLook(SkillInfo.Look.INTERNAL)
                .build();

        List<AppMetricaProto.ReportMessage.Session.Event> events = List.of(
                AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                        .setNumberInSession(eventTime.toEpochMilli() + 1)
                        .setTime(19)
                        .setType(7)
                        .build(),
                AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                        .setNumberInSession(eventTime.toEpochMilli() + 2)
                        .setTime(19)
                        .setType(4)
                        .setName("event")
                        .build()
        );

        AppMetricaServiceImpl.AppMetricaResponse appMetricaResponse = appMetricaService.sendEvents(
                uuid,
                appMetricaApiKeyEncrypted,
                appMetricaEvent,
                events,
                true);

        RecordedRequest recordedRequest = server.takeRequest();

        CharSequence protoRequestText = Files.asCharSource(
                new File("src/test/resources/appmetrica/timestamp_counter_proto.txt"), StandardCharsets.UTF_8
        ).read();

        AppMetricaProto.ReportMessage.Builder builder = AppMetricaProto.ReportMessage.newBuilder();

        TextFormat.getParser().merge(protoRequestText, builder);
        AppMetricaProto.ReportMessage reportMessage = builder.build();

        HttpHeaders headers = new HttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, "application/x-protobuf");
        HttpEntity<AppMetricaProto.ReportMessage> reportMessageHttpEntity = new HttpEntity<>(reportMessage, headers);

        Mockito.verify(restTemplate).postForObject(
                any(),
                eq(reportMessageHttpEntity),
                eq(AppMetricaServiceImpl.AppMetricaResponse.class));

        assertEquals(recordedRequest.getMethod(), "POST");
        assertEquals(recordedRequest.getPath(), "/report?api_key_128=key&uuid=uuid&deviceid=device_id" +
                "&analytics_sdk_version=123&locale=ru&app_id=ru.yandex.alice.paskill" +
                ".dialogovo&app_platform=platform&os_version=1.2" +
                ".3&model=model1&manufacturer=manufac&device_type=phone&app_version_name=appVer");

        assertEquals(new AppMetricaServiceImpl.AppMetricaResponse("accepted"), appMetricaResponse);
    }

    @Test
    void testThrowsExceptionWithWrongSecret() {

        String appMetricaApiKeyEncrypted = CryptoUtils.encrypt(wrongSecret, appMetricaApiKey);

        var appMetricaEvent = AppMetricaEvent.builder().build();

        List<AppMetricaProto.ReportMessage.Session.Event> events = List.of(
                AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                        .setNumberInSession(2)
                        .setTime(0)
                        .setType(4)
                        .build()
        );

        assertThrows(AppMetricaServiceException.class, () -> appMetricaService.sendEvents(
                uuid,
                appMetricaApiKeyEncrypted,
                appMetricaEvent,
                events,
                false));

        assertThrows(AppMetricaServiceException.class, () -> appMetricaService.prepareClientEventsForCommit(
                uuid,
                appMetricaApiKeyEncrypted,
                appMetricaEvent,
                events)
        );
    }
}
