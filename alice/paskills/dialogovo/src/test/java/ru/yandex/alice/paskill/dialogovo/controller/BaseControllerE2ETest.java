package ru.yandex.alice.paskill.dialogovo.controller;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Base64;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Queue;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.zip.GZIPInputStream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;
import javax.servlet.http.HttpServletRequest;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JavaType;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.fasterxml.jackson.databind.node.TextNode;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.google.common.io.Files;
import com.google.protobuf.util.JsonFormat;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.Getter;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.hamcrest.collection.IsIterableContainingInAnyOrder;
import org.json.JSONException;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.params.provider.Arguments;
import org.mockito.Mockito;
import org.skyscreamer.jsonassert.JSONAssert;
import org.skyscreamer.jsonassert.JSONCompareMode;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.web.client.RestTemplateCustomizer;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Primary;
import org.springframework.core.env.Environment;
import org.springframework.core.io.FileSystemResource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpRequest;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;
import org.springframework.http.client.support.HttpRequestWrapper;
import org.springframework.test.context.TestPropertySource;
import org.springframework.util.LinkedCaseInsensitiveMap;
import org.springframework.util.MimeTypeUtils;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.VersionProvider;
import ru.yandex.alice.kronstadt.core.domain.Voice;
import ru.yandex.alice.kronstadt.core.utils.AnythingToStringJacksonDeserializer;
import ru.yandex.alice.kronstadt.scenarios.alice4business.Alice4BusinessService;
import ru.yandex.alice.kronstadt.scenarios.alice4business.DeviceLockState;
import ru.yandex.alice.kronstadt.test.DynamicValueTokenComparator;
import ru.yandex.alice.paskill.dialogovo.config.SecretsConfig;
import ru.yandex.alice.paskill.dialogovo.config.ShowConfig;
import ru.yandex.alice.paskill.dialogovo.domain.Channel;
import ru.yandex.alice.paskill.dialogovo.domain.DeveloperType;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.UserAgreement;
import ru.yandex.alice.paskill.dialogovo.domain.show.MorningShowEpisodeEntity;
import ru.yandex.alice.paskill.dialogovo.domain.show.ShowInfo;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskill.dialogovo.processor.AsyncSkillRequestPoolConfiguration;
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.EventIdProvider;
import ru.yandex.alice.paskill.dialogovo.providers.skill.ShowProvider;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillCategoryKey;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillTagsKey;
import ru.yandex.alice.paskill.dialogovo.providers.skill.UserAgreementProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.InMemoryAppMetricaFirstUserEventDaoImpl;
import ru.yandex.alice.paskill.dialogovo.service.memento.MementoService;
import ru.yandex.alice.paskill.dialogovo.service.memento.NewsProviderSubscription;
import ru.yandex.alice.paskill.dialogovo.service.purchase.InMemoryPurchaseCompleteResponseDaoImpl;
import ru.yandex.alice.paskill.dialogovo.service.purchase.PurchaseCompleteResponseStruct;
import ru.yandex.alice.paskill.dialogovo.service.show.InMemoryShowEpisodeStoreDaoImpl;
import ru.yandex.alice.paskill.dialogovo.service.show.MorningShowEpisodeDao;
import ru.yandex.alice.paskill.dialogovo.service.show.ShowService;
import ru.yandex.alice.paskill.dialogovo.service.show.ShowServiceImpl;
import ru.yandex.alice.paskill.dialogovo.service.show.ShowStateStruct;
import ru.yandex.alice.paskill.dialogovo.service.state.InMemorySkillStateDaoImpl;
import ru.yandex.alice.paskill.dialogovo.service.state.TestStateStruct;
import ru.yandex.alice.paskill.dialogovo.service.xiva.XivaServiceConfiguration;
import ru.yandex.alice.paskill.dialogovo.utils.CryptoUtils;
import ru.yandex.alice.paskill.dialogovo.utils.Headers;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.alice.paskill.dialogovo.utils.executor.TestExecutorsFactory;
import ru.yandex.metrika.appmetrica.proto.AppMetricaProto;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.passport.tvmauth.BlackboxEnv;
import ru.yandex.passport.tvmauth.CheckedServiceTicket;
import ru.yandex.passport.tvmauth.CheckedUserTicket;
import ru.yandex.passport.tvmauth.ClientStatus;
import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.passport.tvmauth.Unittest;
import ru.yandex.passport.tvmauth.roles.Roles;

import static java.util.Optional.ofNullable;
import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.mapping;
import static java.util.stream.Collectors.toList;
import static java.util.stream.Collectors.toMap;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyList;
import static org.mockito.ArgumentMatchers.anySet;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static ru.yandex.alice.kronstadt.test.mockito.MockitoExtensionsKt.eq;
import static ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders.USER_TICKET_HEADER;


@TestPropertySource(properties = {
        "abuseConfig.url: http://localhost/e2e_test/abuseServer/",
        "yaFunctions.url: http://localhost/e2e_test/yaFunctionServer/",
        "nerApi.url: http://localhost/e2e_test/nerServer/",
        "billingConfig.url: http://localhost/e2e_test/billing/billing-rc/",
        "socialApi.url: http://localhost/e2e_test/socialServer/",
        "xivaConfig.url: http://localhost/e2e_test/xivaServer/",
        "wizard.url: http://localhost/e2e_test/wizardServer/",
        "appMetrica.url: http://localhost/e2e_test/metricaServer/",
        "apiConfig.url: http://localhost/e2e_test/apiServer/",
        "penguinaryConfig.url: http://localhost/e2e_test/penguinaryServer/",
        "recommenderConfig.url: http://localhost/e2e_test/recommenderServer/",
})
public class BaseControllerE2ETest {
    protected static final String TRUSTED_SERVICE_TVM_CLIENT_ID = "2000860";
    protected static final String TVM_USER_TICKET_PREFIX = "TVM-USER-";
    protected static final String EVENT_ID_MOCK = "65c94bc6-ab33-4098-b772-c5bebbfa7e6c";
    protected static final long MOCK_RESPONSE_DELAY = 4000;
    private static final Logger logger = LogManager.getLogger();
    private final Map<String, String> files = new HashMap<>();
    @Autowired
    protected RestTemplate restTemplate;
    @LocalServerPort
    protected int port;

    @Autowired
    protected SecretsConfig secretsConfig;

    @Autowired
    protected InMemorySkillStateDaoImpl skillStateDao;

    @Autowired
    protected InMemoryShowEpisodeStoreDaoImpl showEpisodeStoreDao;

    @Autowired
    protected InMemoryPurchaseCompleteResponseDaoImpl purchaseCompleteResponseDao;

    @MockBean
    protected SkillProvider skillProvider;

    @MockBean
    protected ShowProvider showProvider;

    @MockBean
    protected UserAgreementProvider userAgreementProvider;

    @Autowired
    protected RequestContext requestContext;
    @Autowired
    protected DialogovoRequestContext dialogovoRequestContext;

    @Autowired
    protected MockTvmClient tvmClient;

    @Autowired
    protected NewsSkillProviderMock newsSkillProviderMock;

    @Autowired
    protected Alice4BusinessServiceMock alice4BusinessService;

    @Autowired
    protected MementoServiceMock mementoService;

    @Autowired
    protected JsonFormat.Parser protoJsonParser;
    @Autowired
    protected ObjectMapper objectMapper;
    @Autowired
    private MockServer mockServer;
    @Autowired
    private InMemoryAppMetricaFirstUserEventDaoImpl appMetricaFirstUserEventDao;

    protected static MorningShowEpisodeDao morningShowEpisodeDao;

    private void readMockFiles(File dir) throws IOException {
        readFiles(dir);
        readFiles(getClassPathDir(getDefaultMocksFilesPath()));
    }

    private void readFiles(File dir) throws IOException {
        File[] dirFiles = dir.listFiles();
        if (dirFiles == null) {
            return;
        }

        for (File file : dirFiles) {
            this.files.put(file.getName(), readFile(file));
        }
    }

    protected String getDefaultMocksFilesPath() {
        return "defaults";
    }

    @Nullable
    private File getClassPathDir(String path) throws IOException {

        return Arrays.stream(new PathMatchingResourcePatternResolver().getResources(path))
                .map(resourse -> {
                    try {
                        return resourse.getFile();
                    } catch (IOException e) {
                        return null;
                    }
                })
                .filter(Objects::nonNull)
                .filter(File::isDirectory)
                .findFirst()
                .orElse(null);
    }

    @Nullable
    protected String readContent(String name) {
        return files.get(name);
    }

    @Nullable
    protected String readContentOrDefault(String name, String defaultName) {
        return ofNullable(readContent(name)).orElseGet(() -> readContent(defaultName));
    }

    @Nullable
    protected <T> T readContent(String name, Class<T> clazz) throws IOException {
        var content = readContent(name);
        if (content != null) {
            return objectMapper.readValue(content, clazz);
        }

        return null;
    }

    @Nullable
    protected <T> T readContent(String name, JavaType type, ObjectMapper mapper) throws IOException {
        var content = readContent(name);
        if (content != null) {
            return mapper.readValue(content, type);
        }
        return null;
    }

    @Nullable
    protected <T> List<T> readList(
            String name,
            ObjectMapper mapper,
            Class<T> elementType
    )
            throws IOException {
        var content = readContent(name);
        if (content != null) {
            return mapper.readValue(
                    content,
                    mapper.getTypeFactory().constructCollectionType(List.class, elementType));
        }
        return null;
    }


    static String readFile(File file) throws IOException {
        return Files.asCharSource(file, StandardCharsets.UTF_8).read();
    }

    protected static List<Arguments> getTestDataFromDir(String locationPattern) throws IOException {
        var resolver = new PathMatchingResourcePatternResolver().getResources(locationPattern);

        return Arrays.stream(resolver)
                .map(r -> (FileSystemResource) r)
                .filter(fsr -> fsr.getFile().isDirectory())
                .flatMap(BaseControllerE2ETest::resourceToArguments)
                .collect(toList());
    }

    @Nonnull
    private static Stream<Arguments> resourceToArguments(FileSystemResource x) {
        String filename = x.getFilename();

        File[] subdirs = x.getFile().listFiles(File::isDirectory);
        if (subdirs == null || subdirs.length == 0) {
            return Stream.of(Arguments.of(filename, x.getPath(), "/megamind"));
        } else {
            String base = "/" + filename.substring(1);
            return Arrays.stream(subdirs)
                    .map(file -> Arguments.of(file.getName(), file.getPath(), base));
        }
    }

    @AfterEach
    void tearDown() {
        logger.debug("base teardown started");
        mockServer.clearAll();
        files.clear();

        requestContext.clear();
        skillStateDao.clear();
        showEpisodeStoreDao.clear();
        purchaseCompleteResponseDao.clear();
        dialogovoRequestContext.clear();
        newsSkillProviderMock.skillsDb.clear();
        newsSkillProviderMock.phrasesDb.clear();
        mementoService.newsProviderSubscription = Optional.empty();
        appMetricaFirstUserEventDao.clear();
        Mockito.reset(morningShowEpisodeDao);
        logger.debug("base teardown finished");
    }

    @BeforeEach
    void setUp() throws IOException {
        logger.debug("base setup_started");
        requestContext.clear();
        dialogovoRequestContext.clear();
        files.clear();

        logger.debug("base setup finished");

    }

    protected String urlForStub(String stub, String path) {
        return "http://localhost:" + port + "/e2e_test/" + stub + path;
    }

    protected void validateState(String expectedResultingState) throws JsonProcessingException, JSONException {
        var actual = new TestStateStruct(skillStateDao.getSessions(), skillStateDao.getUsers(),
                skillStateDao.getApplications());

        JSONAssert.assertEquals(
                expectedResultingState,
                objectMapper.writeValueAsString(actual),
                new DynamicValueTokenComparator(JSONCompareMode.STRICT));
    }

    protected void validateShowState(String expectedResultingState) throws JsonProcessingException, JSONException {
        var actual = new ShowStateStruct(showEpisodeStoreDao.getEpisodes());

        JSONAssert.assertEquals(
                expectedResultingState,
                objectMapper.writeValueAsString(actual),
                new DynamicValueTokenComparator(JSONCompareMode.STRICT));
    }

    private void validatePurchaseCompleteResponses(String expectedResponsesFilename) throws JsonProcessingException {
        var expectedResponses = objectMapper.readValue(
                expectedResponsesFilename,
                new TypeReference<List<PurchaseCompleteResponseStruct>>() {
                }
        );
        if (expectedResponses.isEmpty()) {
            return;
        }
        assertThat(
                purchaseCompleteResponseDao.getResponsesAsList(),
                IsIterableContainingInAnyOrder.containsInAnyOrder(expectedResponses.toArray())
        );
    }

    protected void mockServices(File dir, String uuidReplacement, Instant now) throws IOException {

        logger.debug("mocking started");
        readMockFiles(dir);
        logger.debug("all files read");
        String timestampReplacement = String.valueOf(now.toEpochMilli());

        mockContext();
        List<SkillInfo> skills = getMockSkills();
        logger.debug("skills fetched");
        mockSkillInfo(skills);
        logger.debug("skills mocked");

        Optional<NewsSkillInfo> newsSkill1 = getMockNewsSkill("news_skill.json");
        Optional<NewsSkillInfo> newsSkill2 = getMockNewsSkill("news_skill2.json");
        logger.debug("NewsSkil fetched");

        mockNewsSkillInfo(newsSkill1, "news_contents.json");
        mockNewsSkillInfo(newsSkill2, "news_contents2.json");
        logger.debug("NewsSkil mocked");

        mockMementoService();
        logger.debug("memento mocked");

        String webhookResponse;
        if ((webhookResponse = readContent("webhook_delayed_response.json")) != null) {
            mockServer.enqueueDelayedResponse("webhookServer",
                    webhookResponse.replaceAll("<UUID>", uuidReplacement));
        } else {
            mockServer.enqueueResponse("webhookServer",
                    ofNullable(readContent("webhook_response.json"))
                            .map(v -> v.replaceAll("<UUID>", uuidReplacement))
                            .orElse(null));
        }
        mockServer.enqueueResponse("socialServer", readContent("social_response.json"));
        mockServer.enqueueResponse("nerServer", readContentOrDefault("ner_response.json", "ner_response_default" +
                ".json"));
        mockServer.enqueueResponse("abuseServer", readContentOrDefault("abuse_response.json",
                "abuse_response_default.json"));
        mockServer.enqueueResponse("xivaServer", readContent("xiva_list_response.json"),
                readContent("xiva_send_response.json"));
        mockServer.enqueueResponse("yaFunctionServer", readContent("ya_function_response.json"));
        mockServer.enqueueResponse("wizardServer", readContent("wizard_response.json"),
                readContent("wizard_response_2.json"),
                readContent("wizard_response_3.json"));
        mockServer.enqueueResponse("penguinaryServer", readContent("penguinary_response.json"));
        mockServer.enqueueResponse("recommenderServer", readContent("recommender_response.json"));
        mockServer.enqueueResponse("apiServer", readContent("api_response.json"));
        mockServer.enqueueResponse("billing", readContent("billing_response.json"),
                readContent("billing_response_2.json"));
        mockServer.enqueueResponse("metricaServer", readContent("appmetrica_response.json"));
        logger.debug("responses mocked");

        mockAlice4Business(readContent("alice4business_devices_lock_state.json"));
        logger.debug("a4b mocked");
        mockSkillProvider(skills);
        logger.debug("skillProvider mocked");
        mockShowProvider(getMockShows(skills));
        logger.debug("showProvider mocked");
        mockMorningShowEpisodeDao(
                readContent("morning_show_personalized_episode.json"),
                readContent("morning_show_unpersonalized_episode.json")
        );
        logger.debug("morningShowEpisodeDao mocked");
        mockNewsSkillActivation(List.of(newsSkill1, newsSkill2)
                .stream()
                .flatMap(Optional::stream)
                .collect(toList()));
        logger.debug("newsSkillActivation mocked");
        mockSkillsActivationIntents(readContent("skill_activation_intents.json"));
        logger.debug("skillsActivationIntents mocked");
        mockUserAgreements();
        logger.debug("agreements mocked");

        initState(timestampReplacement);
        logger.debug("initState mocked");
        initShowEpisodeState(timestampReplacement);
        logger.debug("initShowEpisodeState mocked");
        initPurchaseCompleteResponse();
        logger.debug("last mocks mocked");

        logger.debug("mocking finished");
    }

    private void initState(String timestampReplacement) throws IOException {
        String initialState = readContent("initial_state.json");
        if (initialState != null) {
            initialState = initialState.replaceAll("getWizard\"<TIMESTAMP>\"", timestampReplacement);
            TestStateStruct states = objectMapper.readValue(initialState, TestStateStruct.class);
            skillStateDao.load(states.getSessions(), states.getUsers(), states.getApplications());
        }
    }

    private void initShowEpisodeState(String timestampReplacement) throws IOException {
        String initialState = readContent("show_initial_state.json");
        if (initialState != null) {
            initialState = initialState.replaceAll("getWizard\"<TIMESTAMP>\"", timestampReplacement);
            ShowStateStruct episodes = objectMapper.readValue(initialState, ShowStateStruct.class);
            showEpisodeStoreDao.load(episodes.getEpisodes());
        }
    }

    private void initPurchaseCompleteResponse() throws IOException {
        String row = readContent("initial_purchase_complete_responses.json");
        if (row != null) {
            var responses = objectMapper.readValue(row, new TypeReference<List<PurchaseCompleteResponseStruct>>() {
            });
            for (PurchaseCompleteResponseStruct response : responses) {
                purchaseCompleteResponseDao.storeResponse(response.key(), Instant.now(), response.response());
            }
        }
    }

    protected HttpHeaders getHttpHeadersWithJson() {
        var headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        return headers;
    }

    protected HttpHeaders getHttpHeaders() {
        HttpHeaders headers = new HttpHeaders();
        headers.add(Headers.X_DEVELOPER_TRUSTED_TOKEN, "TRUSTED");
        headers.add(Headers.X_TRUSTED_SERVICE_TVM_CLIENT_ID, TRUSTED_SERVICE_TVM_CLIENT_ID);
        headers.add(Headers.X_REQUEST_ID, "C630BCA9-2FF7-4D4E-A18C-805FAC3DA8AC");
        if (requestContext.getCurrentUserId() != null) {
            headers.add(USER_TICKET_HEADER, TVM_USER_TICKET_PREFIX + requestContext.getCurrentUserId());
        }
        return headers;
    }

    protected void validateRequest(String stub, String name)
            throws IOException, JSONException {

        var expectedReq = readContent(name, RequestWrapper.class);

        if (expectedReq != null) {
            logger.debug("taking request from server: {}", name);
            List<RecordedRequest> requests = mockServer.getRecordedRequests(stub);
            assertFalse(requests.isEmpty(), "no requests recorded for " + stub);
            logger.debug("taking request from server finished: {}", name);
            var req = requests.remove(0);

            if (expectedReq.getPath() != null) {
                assertEquals(expectedReq.getPath(), req.getPath());
            }

            if (expectedReq.getMethod() != null) {
                assertEquals(expectedReq.getMethod(), req.getMethod());
            }

            if (expectedReq.getHeaders() != null) {
                for (var key : expectedReq.getHeaders().keySet()) {
                    assertEquals(expectedReq.getHeaders().get(key), req.getHeaders().get(key));
                }
            }

            if (expectedReq.getBody() != null) {
                var actualRequest = req.readUtf8();
                logger.debug("actual webhook request:\n{}",
                        objectMapper.readTree(actualRequest).toPrettyString());
                JSONAssert.assertEquals(expectedReq.getBody(),
                        actualRequest, new DynamicValueTokenComparator(JSONCompareMode.STRICT));
            }
        } else if (name.equals("webhook_request.json")) {
            assertTrue(mockServer.getRecordedRequests("webhookServer").isEmpty(),
                    "no request to webhook expected");
        }
    }

    private void validateAppmetricaProtobufRequest(String name) throws IOException, JSONException {
        var expectedReq = readContent(name, RequestWrapper.class);

        logger.debug("taking request from server: {}", name);
        var requests = mockServer.getRecordedRequests("metricaServer");
        logger.debug("taking request from server finished: {}", name);

        if (expectedReq != null) {
            assertFalse(requests.isEmpty(), "no request is recorded for metricaServer");
            assertEquals(requests.size(), 1, "few requests are recorded for metricaServer");
            var req = requests.remove(0);

            if (expectedReq.getPath() != null) {
                assertEquals(expectedReq.getPath(), req.getPath());
            }

            if (expectedReq.getMethod() != null) {
                assertEquals(expectedReq.getMethod(), req.getMethod());
            }

            if (expectedReq.getHeaders() != null) {
                for (var key : expectedReq.getHeaders().keySet()) {
                    assertEquals(expectedReq.getHeaders().get(key), req.getHeaders().get(key));
                }
            }

            if (expectedReq.getBody() != null) {
                var reportMessage = AppMetricaProto.ReportMessage.parseFrom(ungzip(req.getBody()));
                var actualJsonRequest = objectMapper.readTree(JsonFormat.printer().print(reportMessage));
                var events = actualJsonRequest.get("sessions").get(0).withArray("events");
                for (int i = 0; i < events.size(); ++i) {
                    if (events.get(i).has("value")) {
                        var bytes = Base64.getDecoder().decode(events.get(i).get("value").asText());
                        var jsonObj = objectMapper.readTree(bytes);
                        ((ObjectNode) events.get(i)).set("value", jsonObj);
                    }
                    Assertions.assertTrue(Long.parseLong(events.get(i).get("time").asText()) >=
                            (i == 0 ? 0L : Long.parseLong(events.get(i - 1).get("time").asText())));
                }

                JSONAssert.assertEquals(expectedReq.getBody(), actualJsonRequest.toString(),
                        new DynamicValueTokenComparator(JSONCompareMode.STRICT));
            }
        } else {
            assertTrue(requests.isEmpty(), "unexpected request for metricaServer");
        }
    }

    private byte[] ungzip(byte[] msg) throws IOException {
        var is = new ByteArrayInputStream(msg);
        var gzin = new GZIPInputStream(is);
        java.io.ByteArrayOutputStream byteOut = new java.io.ByteArrayOutputStream();

        int res = 0;
        byte[] buf = new byte[1024];
        while (res >= 0) {
            res = gzin.read(buf, 0, buf.length);
            if (res > 0) {
                byteOut.write(buf, 0, res);
            }
        }
        return byteOut.toByteArray();
    }

    protected void mockContext() throws IOException {
        var context = readContent("context.json", ContextWrapper.class);
        if (context != null) {
            requestContext.setCurrentUserId(context.currentUserId);
            requestContext.setCurrentUserTicket(context.currentUserTicket);
        }
    }

    private List<SkillInfo> getMockSkills() throws IOException {

        var citySkillInfo = SkillInfo.builder()
                .id("672f7477-d3f0-443d-9bd5-2487ab0b6a4c")
                .name("Города")
                .userId("564629782")
                .nameTts("Город+а")
                .channel(Channel.ALICE_SKILL)
                .category("category")
                .developerName("developer name")
                .developerType(DeveloperType.External)
                .description("description")
                .logoUrl("https://avatars.mds.yandex.net/get-dialogs/758954/1a309e8e7d6781214dc5/orig")
                .storeUrl("https://dialogs.yandex.ru/store/skills/skill1")
                .look(SkillInfo.Look.EXTERNAL)
                .voice(Voice.SHITOVA_US)
                .onAir(true)
                .salt("salt")
                .persistentUserIdSalt("salt2")
                .surfaces(Set.of())
                .socialAppName("xxx-social-app-name")
                .featureFlags(Set.of("send_skill_serviсe_ticket_for_direct"))
                .userFeatureFlags(Collections.emptyMap())
                .encryptedAppMetricaApiKey(null)
                .useStateStorage(false)
                .adBlockId(null)
                .useNlu(true)
                .slug("city-game")
                .build();

        var secondMemorySkillInfo = SkillInfo.builder()
                .id("059fe34d-f446-4fc2-b7e2-6504fb89c27b")
                .name("Вторая память")
                .userId("202060379")
                .inflectedActivationPhrases(List.of("2 память", "2 памяти"))
                .channel(Channel.ALICE_SKILL)
                .category("category")
                .developerName("developer name")
                .developerType(DeveloperType.External)
                .description("description")
                .logoUrl("https://avatars.mds.yandex.net/get-dialogs/758954/1a309e8e7d6781214dc5/orig")
                .storeUrl("https://dialogs.yandex.ru/store/skills/skill1")
                .look(SkillInfo.Look.EXTERNAL)
                .voice(Voice.SHITOVA_US)
                .onAir(true)
                .salt("salt")
                .persistentUserIdSalt("salt2")
                .surfaces(Set.of())
                .featureFlags(Set.of(SkillFeatureFlag.ALLOW_ACTIVATE_ANOTHER_SKILL))
                .userFeatureFlags(Collections.emptyMap())
                .encryptedAppMetricaApiKey(null)
                .useStateStorage(false)
                .adBlockId(null)
                .isRecommended(true)
                .automaticIsRecommended(true)
                .slug("second-memory")
                .build();

        List<SkillInfo> skills = new ArrayList<>();
        var mockSkill = readContent("skill.json", ObjectNode.class);
        if (mockSkill != null && !mockSkill.isEmpty()) {
            if (!mockSkill.hasNonNull("userId")) {
                mockSkill.set("userId", TextNode.valueOf("1"));
            }
            if (!mockSkill.hasNonNull("sharedAccess")) {
                mockSkill.set("sharedAccess", objectMapper.createArrayNode());
            }
            if (!mockSkill.hasNonNull("slug")) {
                mockSkill.set("slug", TextNode.valueOf("skill-slug"));
            }
            if (!mockSkill.hasNonNull("persistentUserIdSalt")) {
                mockSkill.set("persistentUserIdSalt", TextNode.valueOf("persistentUserIdSalt"));
            }

            if (!mockSkill.has("encryptedAppMetricaApiKey")) {
                mockSkill.set("encryptedAppMetricaApiKey",
                        TextNode.valueOf(CryptoUtils.encrypt(
                                secretsConfig.getAppMetricaEncryptionSecret(),
                                "0123456789101112")
                        ));
            }
            skills.add(objectMapper.treeToValue(mockSkill, SkillInfo.class));
        }
        skills.add(citySkillInfo);
        skills.add(secondMemorySkillInfo);
        return skills;
    }

    private static List<UserAgreement> findUserAgreements(
            @Nullable List<UserAgreement> userAgreements,
            UUID skillId
    ) {
        if (userAgreements == null) {
            return Collections.emptyList();
        }
        return userAgreements.stream()
                .filter(ua -> ua.getSkillId().equals(skillId))
                .collect(Collectors.toUnmodifiableList());
    }

    private void mockUserAgreements() throws IOException {
        List<UserAgreement> draftUserAgreements = readList(
                "draft_user_agreements.json",
                objectMapper,
                UserAgreement.class);
        when(userAgreementProvider.getDraftUserAgreements(any()))
                .thenAnswer(args -> findUserAgreements(draftUserAgreements, args.getArgument(0)));
        List<UserAgreement> publishedUserAgreements = readList(
                "published_user_agreements.json",
                objectMapper,
                UserAgreement.class);
        when(userAgreementProvider.getPublishedUserAgreements(any()))
                .thenAnswer(args -> findUserAgreements(publishedUserAgreements, args.getArgument(0)));
    }

    private List<ShowInfo> getMockShows(List<SkillInfo> mockedSkills) throws IOException {
        var mockShow = readContent("show.json", ObjectNode.class);
        if (mockShow != null) {
            mockShow.set("skillInfo", objectMapper.valueToTree(mockedSkills.get(0)));
            return List.of(
                    objectMapper.treeToValue(mockShow, ShowInfo.class)
            );
        } else {
            return Collections.emptyList();
        }
    }

    private Optional<NewsSkillInfo> getMockNewsSkill(String skillFileName) throws IOException {
        var mockSkill = readContent(skillFileName, ObjectNode.class);
        if (mockSkill != null) {
            if (!mockSkill.has("encryptedAppMetricaApiKey")) {
                mockSkill.put("encryptedAppMetricaApiKey",
                        CryptoUtils.encrypt(secretsConfig.getAppMetricaEncryptionSecret(), "0123456789101112"));
            }
        }
        return ofNullable(objectMapper.treeToValue(mockSkill, NewsSkillInfo.class));
    }

    protected void mockSkillInfo(List<SkillInfo> skills) {
        var webHookUrl = urlForStub("webhookServer", "/");
        Map<String, SkillInfo> skillMap = skills.stream()
                .map(skill -> skill.toBuilder().webhookUrl(webHookUrl).build())
                .collect(toMap(SkillInfo::getId, x -> x, (c1, c2) -> c1));

        when(skillProvider.getSkill(anyString()))
                .then(inv -> ofNullable(skillMap.get(inv.<String>getArgument(0))));
        when(skillProvider.getSkillDraft(anyString()))
                .then(inv -> ofNullable(skillMap.get(inv.<String>getArgument(0))));
    }

    protected void mockNewsSkillInfo(Optional<NewsSkillInfo> skillO, String newsContentsPath)
            throws IOException {
        if (skillO.isEmpty()) {
            return;
        }

        ObjectMapper mapper = objectMapper.registerModule(new JavaTimeModule());

        List<NewsContent> mockNewsContents = readContent(newsContentsPath,
                mapper.getTypeFactory().constructCollectionType(List.class, NewsContent.class), mapper);

        NewsSkillInfo skill = skillO.get();
        List<NewsFeed> feeds = new ArrayList<>();
        newsSkillProviderMock.skillsDb.put(skill.getId(), skill);

        skill.getDefaultFeed().ifPresent(feeds::add);
        feeds.addAll(skill.getFeeds());

        for (NewsFeed newsFeed : feeds) {
            mockNewsContents.stream()
                    .filter(content -> content.getFeedId().equals(newsFeed.getId()))
                    .sorted(Comparator.comparing(NewsContent::getPubDate).reversed())
                    .forEach(content -> newsFeed.getTopContents().add(content));
        }
    }

    private void mockMementoService() throws IOException {
        var mockNewsSubscription = readContent("memento_news_subscription.json", NewsProviderSubscription.class);
        if (mockNewsSubscription != null) {
            mementoService.newsProviderSubscription = Optional.of(mockNewsSubscription);
        }

    }

    protected void mockSkillProvider(List<SkillInfo> skills) {
        var db = new HashMap<String, String>();
        for (var skill : skills) {
            db.put(skill.getName().toLowerCase(), skill.getId());
            for (String activationPhrase : skill.getInflectedActivationPhrases()) {
                db.put(activationPhrase, skill.getId());
            }
        }
        when(skillProvider.findSkillsByPhrases(anySet()))
                .then(inv -> {
                    Set<String> phrases = inv.getArgument(0);
                    return db.entrySet().stream()
                            .filter(entry -> phrases.contains(entry.getKey()))
                            .collect(groupingBy(
                                    Map.Entry::getKey,
                                    mapping(Map.Entry::getValue, toList())));
                });

        when(skillProvider.getSkillsByTags(any(SkillTagsKey.class)))
                .then(params -> {
                    SkillTagsKey skillTagsKey = params.getArgument(0);

                    return skills.stream()
                            .filter(skill -> ofNullable(skill.getTags()).orElse(Set.of())
                                    .containsAll(skillTagsKey.getTags()))
                            .collect(Collectors.toList());
                });

        when(skillProvider.getSkillsByCategory(any(SkillCategoryKey.class)))
                .then(params -> {
                    SkillCategoryKey skillCategoryKey = params.getArgument(0);

                    return skills.stream()
                            .filter(skill -> skillCategoryKey.getCategory().equals(skill.getCategory()))
                            .collect(Collectors.toList());
                });
    }

    protected void mockShowProvider(List<ShowInfo> shows) {
        var db = new HashMap<String, ShowInfo>();
        var webHookUrl = urlForStub("webhookServer", "/");
        for (var show : shows) {
            db.put(show.getId(), new ShowInfo(
                    show.getId(),
                    show.getName(),
                    show.getNameTts(),
                    show.getDescription(),
                    show.getSkillInfo().toBuilder()
                            .webhookUrl(webHookUrl)
                            .build(),
                    show.getShowType(),
                    show.getOnAir(),
                    show.getPersonalizationEnabled()
            ));
        }

        when(showProvider.getActiveShowSkills(any())).thenReturn(db.values().stream()
                .filter(show -> !show.getPersonalizationEnabled()).toList());
        when(showProvider.getActivePersonalizedShowSkills(any())).thenReturn(db.values().stream()
                .filter(ShowInfo::getPersonalizationEnabled).toList());
        when(showProvider.getShowFeed(anyString(), any()))
                .then(inv -> {
                    String feedId = inv.getArgument(0, String.class);
                    return ofNullable(db.get(feedId));
                });
        when(showProvider.getShowFeedBySkillId(anyString(), any()))
                .then(inv -> {
                    String skillId = inv.getArgument(0, String.class);
                    return db.values().stream().filter(show -> show.getSkillInfo().getId().equals(skillId)).findFirst();
                });

        when(showProvider.getShowFeedsByIds(any()))
                .then(params -> {
                    Collection<String> feedIds = params.getArgument(0);
                    return feedIds.stream().map(db::get)
                            .filter(Objects::nonNull)
                            .collect(toList());
                });
    }

    protected void mockNewsSkillActivation(List<NewsSkillInfo> skills) {
        for (var skill : skills) {
            newsSkillProviderMock.phrasesDb.put(skill.getName().toLowerCase(), skill.getId());
            for (String activationPhrase : skill.getInflectedActivationPhrases()) {
                newsSkillProviderMock.phrasesDb.put(activationPhrase, skill.getId());
            }
        }
    }

    protected void mockSkillsActivationIntents(@Nullable String mockFileContent) throws JsonProcessingException {
        Map<String, Set<String>> data = mockFileContent == null
                ? Map.of()
                : objectMapper.readValue(mockFileContent, new TypeReference<>() {
        });

        when(skillProvider.getActivationIntentFormNames(anyList()))
                .then(inv -> {
                    List<String> skills = inv.getArgument(0);
                    return skills.stream()
                            .filter(data::containsKey)
                            .collect(toMap(x -> x, data::get));
                });
    }

    protected void mockMorningShowEpisodeDao(
            @Nullable String personalizedEpisodeJson, @Nullable String unpersonalizedEpisodeJson
    ) throws JsonProcessingException {

        if (personalizedEpisodeJson != null) {
            MorningShowEpisodeEntity personalizedEpisode = objectMapper.readValue(personalizedEpisodeJson,
                    MorningShowEpisodeEntity.class);
            when(morningShowEpisodeDao.getEpisode(eq(personalizedEpisode.getSkillId()),
                            eq(personalizedEpisode.getUserId()),
                            Mockito.argThat(episodeId -> episodeId == null ||
                                    episodeId.equals(personalizedEpisode.getEpisodeId()))
                    )
            ).thenReturn(personalizedEpisode);
        }

        if (unpersonalizedEpisodeJson != null) {
            MorningShowEpisodeEntity unpersonalizedEpisode = objectMapper.readValue(unpersonalizedEpisodeJson,
                    MorningShowEpisodeEntity.class);

            when(morningShowEpisodeDao.getEpisode(eq(unpersonalizedEpisode.getSkillId()), eq(null),
                            Mockito.argThat(episodeId -> episodeId == null ||
                                    episodeId.equals(unpersonalizedEpisode.getEpisodeId()))
                    )
            ).thenReturn(unpersonalizedEpisode);

            when(morningShowEpisodeDao.getUnpersonalizedEpisode(eq(unpersonalizedEpisode.getSkillId()),
                            Mockito.argThat(episodeId -> episodeId == null ||
                                    episodeId.equals(unpersonalizedEpisode.getEpisodeId()))
                    )
            ).thenReturn(unpersonalizedEpisode);
        }
    }

    protected void mockAlice4Business(@Nullable String devicesLockStateContent) throws JsonProcessingException {
        alice4BusinessService.config = devicesLockStateContent == null
                ? Map.of()
                : objectMapper.readValue(
                devicesLockStateContent,
                new TypeReference<HashMap<String, DeviceLockState>>() {
                });
    }

    protected void validateRequests(File dir) throws IOException, JSONException {
        logger.debug("requests validation started");
        validateRequest("webhookServer", "webhook_request.json");
        validateRequest("recommenderServer", "recommender_request.json");
        validateRequest("socialServer", "social_request.json");
        validateRequest("nerServer", "ner_request.json");
        validateRequest("abuseServer", "abuse_request.json");
        validateRequest("xivaServer", "xiva_list_request.json");
        validateRequest("xivaServer", "xiva_send_request.json");
        validateRequest("yaFunctionServer", "ya_function_request.json");
        validateRequest("wizardServer", "wizard_request.json");
        validateRequest("penguinaryServer", "penguinary_request.json");
        validateRequest("apiServer", "api_request.json");
        validateRequest("billing", "billing_request.json");

        validateAppmetricaProtobufRequest("appmetrica_request.json");

        var expectedResultingState = readContent("resulting_state.json");
        if (expectedResultingState != null) {
            validateState(expectedResultingState);
        }

        var expectedResultingShowState = readContent("resulting_show_state.json");
        if (expectedResultingShowState != null) {
            validateShowState(expectedResultingShowState);
        }

        var expectedPurchaseCompleteResponses = readContent("resulting_purchase_complete_responses.json");
        if (expectedPurchaseCompleteResponses != null) {
            validatePurchaseCompleteResponses(expectedPurchaseCompleteResponses);
        }
        logger.debug("requests validation finished");
    }

    protected void validateResponse(String expectedResponseFile, ResponseEntity<String> response) throws JSONException {
        var expectedRunResponse = ofNullable(readContent(expectedResponseFile))
                .orElseThrow(() -> new RuntimeException("no " + expectedResponseFile + " file"));
        try {
            JSONAssert.assertEquals(
                    expectedRunResponse,
                    response.getBody(),
                    new DynamicValueTokenComparator(JSONCompareMode.STRICT)
            );
        } catch (AssertionError e) {
            logger.warn("Assertion failure:" + response.getBody(), e);
            throw e;
        }
    }

    @TestConfiguration
    @Import({XivaServiceConfiguration.class, AsyncSkillRequestPoolConfiguration.class})
//    @ComponentScan(lazyInit = true)
    public static class SyncExecutorsConfiguration {

        @Autowired
        private Environment env;

        @Bean
        @Primary
        public Alice4BusinessService alice4BusinessServiceMock() {
            return new Alice4BusinessServiceMock();
        }

        @Bean
        @Primary
        public NewsSkillProvider newsSkillProviderMock() {
            return new NewsSkillProviderMock();
        }

        @Bean
        @Primary
        public MementoService mementoServiceMock() {
            return new MementoServiceMock();
        }

        @Bean("xivaServiceExecutor")
        @Primary
        public DialogovoInstrumentedExecutorService xivaServiceExecutor(
                RequestContext context,
                DialogovoRequestContext dialogovoRequestContext
        ) {
            return TestExecutorsFactory.syncExecutor(context, dialogovoRequestContext);
        }

        @Primary
        @Bean(value = "asyncSkillRequestServiceExecutor")
        public DialogovoInstrumentedExecutorService asyncSkillRequestServiceExecutor(
                RequestContext context,
                DialogovoRequestContext dialogovoRequestContext
        ) {
            return TestExecutorsFactory.syncExecutor(context, dialogovoRequestContext);
        }

        @Primary
        @Bean(value = "appMetricaServiceExecutor")
        public DialogovoInstrumentedExecutorService appMetricaServiceExecutor(
                RequestContext context,
                DialogovoRequestContext dialogovoRequestContext
        ) {
            return TestExecutorsFactory.syncExecutor(context, dialogovoRequestContext);
        }

        @Primary
        @Bean(value = "widgetGalleryServiceExecutor")
        public DialogovoInstrumentedExecutorService widgetGalleryServiceExecutor(
                RequestContext context,
                DialogovoRequestContext dialogovoRequestContext
        ) {
            return TestExecutorsFactory.syncExecutor(context, dialogovoRequestContext);
        }

        @Primary
        @Bean(value = "teaserServiceExecutor")
        public DialogovoInstrumentedExecutorService teasersServiceExecutor(
                RequestContext context,
                DialogovoRequestContext dialogovoRequestContext
        ) {
            return TestExecutorsFactory.syncExecutor(context, dialogovoRequestContext);
        }

        @Primary
        @Bean(value = "showEpisodeServiceExecutor")
        public DialogovoInstrumentedExecutorService showEpisodeServiceExecutor(
                RequestContext context,
                DialogovoRequestContext dialogovoRequestContext
        ) {
            return TestExecutorsFactory.syncExecutor(context, dialogovoRequestContext);
        }

        @Bean
        @Primary
        public EventIdProvider randomUuidProvider() {
            return new EventIdProviderMock();
        }

        @Bean
        @Primary
        public MorningShowEpisodeDao morningShowEpisodeDao() {
            morningShowEpisodeDao = mock(MorningShowEpisodeDao.class);
            return morningShowEpisodeDao;
        }

        @Bean
        @Primary
        @SuppressWarnings("ParameterNumber")
        public ShowService showService(
                ShowProvider showProvider,
                SkillRequestProcessor skillRequestProcessor,
                InMemoryShowEpisodeStoreDaoImpl storeDao,
                @Qualifier("showEpisodeFetchServiceExecutor") DialogovoInstrumentedExecutorService fetchService,
                ShowConfig showConfig,
                @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry,
                MorningShowEpisodeDao morningShowEpisodeDao
        ) {
            return new ShowServiceImpl(
                    showProvider, skillRequestProcessor, storeDao,
                    fetchService, showConfig, metricRegistry, morningShowEpisodeDao
            );
        }

        @Bean
        @Primary
        public InMemoryShowEpisodeStoreDaoImpl storeDao() {
            return new InMemoryShowEpisodeStoreDaoImpl();
        }

        @Bean("tvmClient")
        @Primary
        public MockTvmClient tvmClient() {
            return new MockTvmClient();
        }

        @Bean("versionProvider")
        public VersionProvider versionProvider() {
            return new TestVersionProvider();
        }

        @Configuration
        static class ShowEpisodeServiceExecutorConfig {
            @Bean(value = "showEpisodeFetchServiceExecutor", destroyMethod = "shutdownNow")
            @Qualifier("showEpisodeFetchServiceExecutor")
            public DialogovoInstrumentedExecutorService showEpisodeFetchServiceExecutor(
                    ExecutorsFactory executorsFactory
            ) {
                return executorsFactory.cachedBoundedThreadPool(2, 100, 100, "show-service.fetch-episodes");
            }
        }

        @Bean
        public RestTemplateCustomizer localPortCustomizer() {
            return new RestTemplateCustomizer() {
                @Override
                public void customize(RestTemplate restTemplate) {
                    restTemplate.getInterceptors().add(localPortInterceptor());
                }
            };
        }

        public ClientHttpRequestInterceptor localPortInterceptor() {
            return new ClientHttpRequestInterceptor() {
                @Override
                public ClientHttpResponse intercept(HttpRequest request, byte[] body,
                                                    ClientHttpRequestExecution execution) throws IOException {

                    if (request.getURI().toString().contains("localhost/e2e_test/")) {
                        String serverPort = env.getProperty("local.server.port");
                        var origUri = request.getURI();
                        HttpRequest requestWithFixedPort = new HttpRequestWrapper(request) {
                            @Override
                            public URI getURI() {
                                return UriComponentsBuilder.fromUri(origUri)
                                        .port(Integer.parseInt(serverPort))
                                        .build()
                                        .toUri();
                            }
                        };
                        return execution.execute(requestWithFixedPort, body);
                    }
                    return execution.execute(request, body);
                }
            };
        }

    }

    private static class MockTvmClient implements TvmClient {

        private static final ClientStatus STATUS = new ClientStatus(ClientStatus.Code.OK, "");

        @Override
        public ClientStatus getStatus() {
            return STATUS;
        }

        @Override
        public String getServiceTicketFor(String alias) {
            return "xxx-ticket";
        }

        @Override
        public String getServiceTicketFor(int tvmId) {
            return "xxx-ticket";
        }

        @Override
        public CheckedServiceTicket checkServiceTicket(String ticketBody) {
            return Unittest.createServiceTicket(TicketStatus.OK, 1);
        }

        @Override
        public CheckedUserTicket checkUserTicket(String ticket) {
            if (ticket.startsWith(TVM_USER_TICKET_PREFIX)) {
                return Unittest.createUserTicket(TicketStatus.OK,
                        Long.parseLong(ticket.substring(TVM_USER_TICKET_PREFIX.length())),
                        new String[0], new long[0]);
            } else {
                return Unittest.createUserTicket(TicketStatus.EXPIRED, 1, new String[0], new long[0]);
            }
        }

        @Override
        public CheckedUserTicket checkUserTicket(String ticketBody, BlackboxEnv overridedBbEnv) {
            return checkUserTicket(ticketBody);
        }

        @Override
        public Roles getRoles() {
            return null;
        }

        @Override
        public void close() {

        }
    }

    protected static class TestVersionProvider implements VersionProvider {
        @Nonnull
        @Override
        public String getVersion() {
            return "100";
        }

        @Nonnull
        @Override
        public String getBranch() {
            return "unknown-vcs-branch";
        }

        @Nonnull
        @Override
        public String getTag() {
            return "";
        }
    }

    @RestController
    protected static class MockServer {

        private final Map<String, Queue<MockResponse>> stubResponses = new ConcurrentHashMap<>();
        private final Map<String, List<RecordedRequest>> recordedRequests = new ConcurrentHashMap<>();

        public MockServer() {
        }

        @RequestMapping(path = {"/e2e_test/{stub}/**", "/e2e_test/{stub}"})
        public ResponseEntity<String> stub(
                HttpServletRequest request,
                @PathVariable("stub") String stub,
                @RequestBody(required = false) @Nullable byte[] body
        ) {
            LinkedCaseInsensitiveMap<String> headers = Collections.list(request.getHeaderNames())
                    .stream()
                    .collect(Collectors.toMap(h -> h,
                            request::getHeader,
                            (u, v) -> u,
                            LinkedCaseInsensitiveMap::new));
            recordedRequests.computeIfAbsent(stub, key -> new ArrayList<>())
                    .add(new RecordedRequest(headers,
                            request.getServletPath().replace("/e2e_test/" + stub, ""),
                            request.getMethod(),
                            body));

            @Nullable
            Queue<MockResponse> stubbedResponses = stubResponses.get(stub);

            MockResponse response;
            if (stubbedResponses != null && (response = stubbedResponses.poll()).responseBody != null) {
                if (response.delayed) {
                    try {
                        Thread.sleep(MOCK_RESPONSE_DELAY);
                    } catch (InterruptedException ignore) {
                    }
                }
                return ResponseEntity.ok().contentType(MediaType.APPLICATION_JSON).body(response.responseBody);
            } else {
                return ResponseEntity.status(500).build();
            }

        }

        public void clearAll() {
            stubResponses.clear();
            recordedRequests.clear();
        }

        public List<RecordedRequest> getRecordedRequests(String stub) {
            return recordedRequests.getOrDefault(stub, Collections.emptyList());
        }

        public void enqueueResponse(String stub, String... responses) {
            stubResponses.computeIfAbsent(stub, key -> new LinkedList<>())
                    .addAll(Arrays.stream(responses).map(MockResponse::mockResponse).collect(Collectors.toList()));
        }

        public void enqueueDelayedResponse(String stub, String... responses) {
            stubResponses.computeIfAbsent(stub, key -> new LinkedList<>())
                    .addAll(Arrays.stream(responses)
                            .map(MockResponse::mockDelayedResponse).collect(Collectors.toList()));
        }

        static class MockResponse {
            String responseBody;
            boolean delayed;

            MockResponse(String responseBody, boolean delayed) {
                this.responseBody = responseBody;
                this.delayed = delayed;
            }

            public static MockResponse mockResponse(String responseBody) {
                return new MockResponse(responseBody, false);
            }

            public static MockResponse mockDelayedResponse(String responseBody) {
                return new MockResponse(responseBody, true);
            }
        }
    }

    @Data
    protected static class MockServerRequest {
        private final HttpMethod method;
        private final URI uri;
        private final HttpHeaders headers;
        @Nullable
        private final String body;
    }

    private static class MementoServiceMock implements MementoService {
        Optional<NewsProviderSubscription> newsProviderSubscription = Optional.empty();

        @Override
        public void updateNewsProviderSubscription(String tvmUserTicket, NewsProviderSubscription subscription) {
            newsProviderSubscription = Optional.of(subscription);
        }

        @Override
        public Optional<NewsProviderSubscription> getUserNewsProviderSubscription(String tvmUserTicket) {
            return newsProviderSubscription;
        }
    }

    private static class EventIdProviderMock implements EventIdProvider {
        @Override
        public UUID generateEventId() {
            return UUID.fromString(EVENT_ID_MOCK);
        }
    }

    private static class NewsSkillProviderMock implements NewsSkillProvider {
        protected final Map<String, NewsSkillInfo> skillsDb = new HashMap<>();
        protected final Map<String, String> phrasesDb = new HashMap<>();

        @Override
        public List<NewsSkillInfo> findAllActive() {
            return List.copyOf(new ArrayList<>(skillsDb.values()));
        }

        @Override
        public Optional<NewsSkillInfo> getSkill(String skillId) {
            return ofNullable(skillsDb.get(skillId));
        }

        @Override
        public Optional<NewsSkillInfo> getSkillBySlug(String slug) {
            return skillsDb.values().stream().filter(skill -> skill.getSlug().equals(slug)).findFirst();
        }

        @Override
        public boolean isReady() {
            return true;
        }

        @Override
        public Map<String, List<String>> findSkillsByPhrases(Set<String> phrases) {
            return phrasesDb.entrySet().stream()
                    .filter(entry -> phrases.contains(entry.getKey()))
                    .collect(groupingBy(
                            Map.Entry::getKey,
                            mapping(Map.Entry::getValue, toList())));
        }
    }

    protected static class Alice4BusinessServiceMock implements Alice4BusinessService {
        protected volatile Map<String, DeviceLockState> config = Map.of();

        @Override
        public boolean isBusinessDevice(@Nullable String deviceId) {
            return config.containsKey(deviceId);
        }

        @Override
        @Nullable
        public DeviceLockState getDeviceLockState(@Nullable String deviceId, Instant serverTime) {
            return config.get(deviceId);
        }

        @Override
        public boolean isReady() {
            return false;
        }
    }

    @Getter
    @AllArgsConstructor
    @Nonnull
    public static class MainConfigTest {

        protected final String test;
    }

    @Data
    protected static class ContextWrapper {
        protected String currentUserId;
        protected String currentUserTicket;
    }

    @Data
    protected static class RequestWrapper {
        protected LinkedCaseInsensitiveMap<String> headers;
        protected String path;
        protected String method;

        @JsonDeserialize(using = AnythingToStringJacksonDeserializer.class)
        protected String body;
    }

    @Data
    protected static class RecordedRequest {
        protected final LinkedCaseInsensitiveMap<String> headers;
        protected final String path;
        protected final String method;

        protected final byte[] body;

        public String readUtf8() {
            return new String(body, StandardCharsets.UTF_8);
        }

    }
}
