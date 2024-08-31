package ru.yandex.alice.paskill.dialogovo.controller;

import java.io.IOException;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.UUID;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.JsonFormat;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.mockito.internal.verification.Times;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.util.MimeTypeUtils;
import org.springframework.web.HttpRequestMethodNotSupportedException;
import org.springframework.web.client.HttpClientErrorException;

import ru.yandex.alice.megamind.protos.scenarios.RequestProto;
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto;
import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.midleware.AccessLogger;
import ru.yandex.alice.paskill.dialogovo.proto.ApplyArgsProto;
import ru.yandex.alice.paskill.dialogovo.providers.skill.ShowProvider;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.providers.skill.UserAgreementProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminSkillsDao;
import ru.yandex.alice.paskill.dialogovo.test.TestSkill;
import ru.yandex.alice.paskill.dialogovo.test.TestSkills;
import ru.yandex.alice.paskill.dialogovo.utils.Headers;
import ru.yandex.alice.paskill.dialogovo.utils.IntegrationTestBase;
import ru.yandex.alice.paskill.dialogovo.utils.ResourceUtils;
import ru.yandex.alice.paskills.dialogovo.test.proto.InvalidRequestProto;
import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.passport.tvmauth.Unittest;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anySet;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static ru.yandex.alice.paskill.dialogovo.test.TestSkills.CITY_GAME_SKILL_ID;
import static ru.yandex.alice.paskill.dialogovo.test.TestSkills.SECOND_MEMORY_SKILL_ID;

@SpringBootTest(classes = TestConfigProvider.class, webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@AutoConfigureWebClient(registerRestTemplate = true)
class MegamindControllerTest extends IntegrationTestBase {

    private static final Integer TRUSTED_SERVICE_TVM_CLIENT_ID = 2000860;
    private final String userId = "123";
    @MockBean
    private SkillProvider skillProvider;
    @MockBean
    private ShowProvider showProvider;
    @MockBean
    private TvmClient tvmCli;
    @MockBean
    private ThereminSkillsDao thereminSkillsDao;
    @MockBean
    private AccessLogger accessLogger;
    @MockBean
    private UserAgreementProvider userAgreementProvider;


    @BeforeEach
    void setUp() {
        when(thereminSkillsDao.findThereminPrivateSkillsByUser(anyString()))
                .thenReturn(List.of());
        when(skillProvider.getSkill(anyString()))
                .thenReturn(Optional.empty());

        when(skillProvider.getSkill(eq(CITY_GAME_SKILL_ID)))
                .then(ans -> Optional.of(TestSkills.cityGameSkill(port, Collections.emptySet(),
                        Collections.emptyMap())));

        when(tvmCli.checkUserTicket(eq("user_ticket")))
                .thenReturn(Unittest.createUserTicket(
                        TicketStatus.OK, Long.parseLong(userId), new String[0], new long[0]));
        when(tvmCli.checkServiceTicket("service_ticket"))
                .thenReturn(Unittest.createServiceTicket(TicketStatus.OK, TRUSTED_SERVICE_TVM_CLIENT_ID));

        when(skillProvider.findSkillsByPhrases(any()))
                .thenReturn(Map.of("города", List.of(CITY_GAME_SKILL_ID)));
    }

    @Test
    void testProtobufRunResponseWithoutHeader() {
        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        String result = restTemplate.postForObject("http://localhost:" + port + "/megamind/run", new HttpEntity<>(
                "{\n" +
                        "    \"input\": {\n" +
                        "        \"text\": {\n" +
                        "            \"utterance\": \"продавец слонов\"\n" +
                        "        }\n" +
                        "    }\n" +
                        "}", headers), String.class);
    }

    private HttpHeaders getHttpHeaders() {
        HttpHeaders headers = new HttpHeaders();
        headers.add(Headers.X_DEVELOPER_TRUSTED_TOKEN, "TRUSTED");
        headers.add(Headers.X_TRUSTED_SERVICE_TVM_CLIENT_ID, "2000860");
        return headers;
    }

    @Test
    void testProtobufRunResponseWithHeader() {

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        String result = restTemplate.postForObject("http://localhost:" + port + "/megamind/run", new HttpEntity<>(
                "{\n" +
                        "    \"input\": {\n" +
                        "        \"text\": {\n" +
                        "            \"utterance\": \"продавец слонов\"\n" +
                        "        }\n" +
                        "    }\n" +
                        "}", headers), String.class);
    }

    @Test
    void testProtobufRunAcceptProtobufResponseProtobuf() {
        final byte[] byteRequest = RequestProto.TScenarioRunRequest.newBuilder()
                .setInput(RequestProto.TInput.newBuilder()
                        .setText(RequestProto.TInput.TText.newBuilder()
                                .setUtterance("продавец слонов")
                                .build())
                        .build())
                .build()
                .toByteArray();

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, "application/protobuf");
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        String result = restTemplate.postForObject("http://localhost:" + port + "/megamind/run",
                new HttpEntity<>(byteRequest, headers), String.class);

    }

    @Test
    void testProtobufRunAcceptProtobufResponseXProtobuf() {
        final byte[] byteRequest = RequestProto.TScenarioRunRequest.newBuilder()
                .setInput(RequestProto.TInput.newBuilder()
                        .setText(RequestProto.TInput.TText.newBuilder()
                                .setUtterance("продавец слонов")
                                .build())
                        .build())
                .build()
                .toByteArray();

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, "application/x-protobuf");
        headers.add(HttpHeaders.ACCEPT, "application/x-protobuf");
        String result = restTemplate.postForObject("http://localhost:" + port + "/megamind/run",
                new HttpEntity<>(byteRequest, headers), String.class);
    }

    @Test
    void testJsonRunResponse() {
        HttpHeaders headers = getHttpHeaders();
        var body = "{\n" +
                "    \"input\": {\n" +
                "        \"text\": {\n" +
                "            \"utterance\": \"продавец слонов\"\n" +
                "        }\n" +
                "    }\n" +
                "}";
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(body, headers), String.class);
    }

    @Test
    void testSkillActivationWithSwitchDialog() {
        when(skillProvider.findSkillsByPhrases(any())).thenReturn(Map.of("города",
                List.of(CITY_GAME_SKILL_ID)));

        String request = requestBody("activate_skill.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();
        var layout = result.getResponseBody().getLayout();

        assertEquals(CITY_GAME_SKILL_ID, result.getResponseBody().getFrameActionsMap().get("action_1")
                .getDirectives()
                .getList(0)
                .getOpenDialogDirective()
                .getDirectives(2)
                .getCallbackDirective()
                .getPayload()
                .getFieldsOrThrow("dialog_id")
                .getStringValue()
        );
    }

    @Test
    void testFixedSkillActivation() {
        String request = requestBody("activate_fixed_skill_games.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertEquals(CITY_GAME_SKILL_ID, result.getResponseBody().getFrameActionsMap().get("action_1")
                .getDirectives()
                .getList(0)
                .getOpenDialogDirective()
                .getDirectives(2)
                .getCallbackDirective()
                .getPayload()
                .getFieldsOrThrow("dialog_id")
                .getStringValue()
        );
    }

    @Test
    void testFixedSkillActivationFrameHasHigherPriority() {
        String request = requestBody("activate_fixed_skill_with_multiple_semantic_frame.json");

        when(skillProvider.getSkill(eq(SECOND_MEMORY_SKILL_ID)))
                .then(ans -> Optional.of(TestSkills.secondMemorySkill(port, true, false, Collections.emptySet(),
                        Collections.emptyMap())));

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertEquals(SECOND_MEMORY_SKILL_ID, result.getResponseBody().getFrameActionsMap().get("action_1")
                .getDirectives()
                .getList(0)
                .getOpenDialogDirective()
                .getDirectives(2)
                .getCallbackDirective()
                .getPayload()
                .getFieldsOrThrow("dialog_id")
                .getStringValue()
        );
    }

    @Test
    void testFixedSkillActivationFrameWithDenyFlagNotActivating() {
        String request = requestBody("activate_fixed_skill_with_multiple_semantic_frame.json");

        when(skillProvider.getSkill(eq(SECOND_MEMORY_SKILL_ID)))
                .then(ans -> Optional.of(TestSkills.secondMemorySkill(
                        port,
                        true,
                        false,
                        Set.of(SkillFeatureFlag.EXTERNAL_SKILL_FIXED_ACTIVATE_DISABLED),
                        Collections.emptyMap())));

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertEquals(CITY_GAME_SKILL_ID, result.getResponseBody()
                .getFrameActionsMap()
                .get("action_1")
                .getDirectives()
                .getList(0)
                .getOpenDialogDirective()
                .getDirectives(2)
                .getCallbackDirective()
                .getPayload()
                .getFieldsOrThrow("dialog_id")
                .getStringValue()
        );
    }

    @Test
    void testFixedSkillActivationFrameWithDenyFlagIrrelevant() {
        String request = requestBody("activate_fixed_skill_second_memory.json");

        when(skillProvider.getSkill(eq(SECOND_MEMORY_SKILL_ID)))
                .then(ans -> Optional.of(TestSkills.secondMemorySkill(
                        port,
                        true,
                        false,
                        Set.of(SkillFeatureFlag.EXTERNAL_SKILL_FIXED_ACTIVATE_DISABLED),
                        Collections.emptyMap())));

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertTrue(result.getFeatures().getIsIrrelevant());
    }

    @Test
    void testFixedSkillActivationWithNotRecommendedSkillIrrelevant() {
        String request = requestBody("activate_fixed_skill_second_memory.json");

        when(skillProvider.getSkill(eq(SECOND_MEMORY_SKILL_ID)))
                .then(ans -> Optional.of(TestSkills.secondMemorySkill(
                        port,
                        false,
                        false,
                        Set.of(),
                        Collections.emptyMap())));

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertTrue(result.getFeatures().getIsIrrelevant());
    }

    @Test
    void testDoNot5xxOnInvalidWebhook() {
        String request = requestBody("apply_fixed_skill_second_memory.json");

        when(skillProvider.getSkill(eq(SECOND_MEMORY_SKILL_ID)))
                .then(ans -> Optional.of(TestSkills.secondMemorySkill(
                        port,
                        false,
                        false,
                        Set.of(),
                        Collections.emptyMap(),
                        "http://alice_sub_trains.ngrok.io/v1/alice"
                )));

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/apply", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioApplyResponse.class).getBody();

        assertEquals(result.getResponseBody().getAnalyticsInfo().getActions(0).getId(),
                "external_skill.request_failure");
    }

    @Test
    void testFixedSkillActivationFrameNotRecommendedSkillNotActivating() {
        String request = requestBody("activate_fixed_skill_with_multiple_semantic_frame.json");

        when(skillProvider.getSkill(eq(SECOND_MEMORY_SKILL_ID)))
                .then(ans -> Optional.of(TestSkills.secondMemorySkill(port, false, false, Collections.emptySet(),
                        Collections.emptyMap())));

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertEquals(CITY_GAME_SKILL_ID, result.getResponseBody().getFrameActionsMap().get("action_1")
                .getDirectives()
                .getList(0)
                .getOpenDialogDirective()
                .getDirectives(2)
                .getCallbackDirective()
                .getPayload()
                .getFieldsOrThrow("dialog_id")
                .getStringValue()
        );
    }

    @Test
    void testSkillActivationWithRequestContinuesWithSwitchDialog() {
        when(skillProvider.findSkillsByPhrases(any())).thenReturn(Map.of("города",
                List.of(CITY_GAME_SKILL_ID)));
        String request = requestBody("activate_skill_with_request_on_pp.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertEquals(CITY_GAME_SKILL_ID, result.getResponseBody().getFrameActionsMap().get("action_1")
                .getDirectives()
                .getList(0)
                .getOpenDialogDirective()
                .getDirectives(3)
                .getCallbackDirective()
                .getPayload()
                .getFieldsOrThrow("dialog_id")
                .getStringValue()
        );

        assertEquals("найти столицу колумбии", result.getResponseBody().getFrameActionsMap().get("action_1")
                .getDirectives()
                .getList(0)
                .getOpenDialogDirective()
                .getDirectives(2)
                .getTypeTextSilentDirective()
                .getText()
        );
    }

    @Test
    void testSkillActivationWithRequestOnPPWithDialogContainsRequestPhrase() {
        when(skillProvider.findSkillsByPhrases(any())).thenReturn(Map.of("города",
                List.of(CITY_GAME_SKILL_ID)));
        String request = requestBody("activate_skill_with_request_on_pp.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertEquals("найти столицу колумбии", result.getResponseBody()
                .getFrameActionsMap().get("action_1")
                .getDirectives()
                .getList(0)
                .getOpenDialogDirective()
                .getDirectives(3)
                .getCallbackDirective()
                .getPayload()
                .getFieldsOrThrow("request")
                .getStringValue()
        );

        assertEquals("найти столицу колумбии", result.getResponseBody().getFrameActionsMap().get("action_1")
                .getDirectives()
                .getList(0)
                .getOpenDialogDirective()
                .getDirectives(2)
                .getTypeTextSilentDirective()
                .getText()
        );
    }

    @Test
    void testSkillActivationWithRequestOnStationContainsRequestApplyArgs() throws InvalidProtocolBufferException {
        when(skillProvider.findSkillsByPhrases(any())).thenReturn(Map.of("города",
                List.of(CITY_GAME_SKILL_ID)));
        String request = requestBody("activate_skill_with_request_on_station.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        var args = result.getApplyArguments().unpack(ApplyArgsProto.ApplyArgumentsWrapper.class);
        assertEquals("найти столицу колумбии", args.getRequestSkill().getRequest());
    }

    @Test
    void testSkillRequestAfterTimeoutOnStationLeadsToIrrelevantResponse() {
        when(skillProvider.findSkillsByPhrases(any())).thenReturn(Map.of("города",
                List.of(CITY_GAME_SKILL_ID)));
        String request = requestBody("request_skill_after_timeout_on_station.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertTrue(result.getFeatures().getIsIrrelevant());
    }

    @Test
    void testSkillIrrelevantActivation() {
        // given
        when(skillProvider.findSkillsByPhrases(anySet())).thenReturn(Map.of("город",
                List.of(CITY_GAME_SKILL_ID)));

        // when
        String request = requestBody("activate_skill_irrelevant.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(Headers.X_REQUEST_ID, UUID.randomUUID().toString());
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        // then
        assertTrue(result.getFeatures().getIsIrrelevant());
    }

    @Test
    void testRequestSkillWithState() throws IOException {
        // when
        String request = requestBody("request_with_state.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(Headers.X_REQUEST_ID, UUID.randomUUID().toString());
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        // then
        assertFalse(result.getFeatures().getIsIrrelevant(), "false irrelevant");

        var args = result.getApplyArguments().unpack(ApplyArgsProto.ApplyArgumentsWrapper.class);
        assertEquals(CITY_GAME_SKILL_ID, args.getRequestSkill().getSkillId());
    }

    @Test
    void testDeactivateDialogByDialogId() {
        var request = requestBody("deactivate_dialog_by_dialog_id.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(Headers.X_REQUEST_ID, UUID.randomUUID().toString());
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertFalse(result.getFeatures().getIsIrrelevant(), "false irrelevant");
        assertFalse(result.hasApplyArguments(), "no apply arguments expected");

        var layout = result.getResponseBody().getLayout();
        var directives = layout.getDirectivesList();
        assertTrue(directives.get(0).hasEndDialogSessionDirective(), "end_dialog_session directive expected");
        assertTrue(directives.get(1).hasCloseDialogDirective(), "close_dialog directive expected");

        assertEquals(0, layout.getSuggestsList().size());
    }

    @Test
    void testDeactivateDialogByState() {
        var request = requestBody("deactivate_dialog_by_state.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(Headers.X_REQUEST_ID, UUID.randomUUID().toString());
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertFalse(result.getFeatures().getIsIrrelevant(), "false irrelevant");
        assertFalse(result.hasApplyArguments(), "no apply arguments expected");

        var layout = result.getResponseBody().getLayout();
        var directives = layout.getDirectivesList();
        assertTrue(directives.get(0).hasEndDialogSessionDirective(), "end_dialog_session directive expected");
        assertTrue(directives.get(1).hasCloseDialogDirective(), "close_dialog directive expected");

        assertEquals(0, layout.getSuggestsList().size());
    }

    @Test
    void testDeactivateDialogIrrelevant() {
        var request = requestBody("deactivate_dialog_irrelevant_no_dialog_id.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(Headers.X_REQUEST_ID, UUID.randomUUID().toString());
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        assertTrue(result.getFeatures().getIsIrrelevant(), "false irrelevant");
    }

    @Test
    @Disabled("apply outside of a skill is not implemented correctly")
    void testRequestSkillRunAndThenApply() throws IOException {
        // when
        var runRequest = requestBody("activate_skill.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(Headers.X_REQUEST_ID, UUID.randomUUID().toString());
        var runResult = restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.POST,
                new HttpEntity<>(runRequest, headers), ResponseProto.TScenarioRunResponse.class).getBody();

        // then
        assertEquals(CITY_GAME_SKILL_ID, runResult.getResponseBody()
                .getFrameActionsMap()
                .get("action_1")
                .getDirectives()
                .getList(2)
                .getCallbackDirective()
                .getPayload()
                .getFieldsOrThrow("dialog_id")
                .getStringValue()
        );

        // when
        var applyBaseRequest = requestBody("say_moscow_to_cities_base_request.json");
        var builder = RequestProto.TScenarioBaseRequest.newBuilder();
        JsonFormat.parser().merge(applyBaseRequest, builder);

        RequestProto.TScenarioApplyRequest applyRequest = RequestProto.TScenarioApplyRequest.newBuilder()
                .setBaseRequest(builder)
                //.setArguments(Any.pack(args)) TODO: FIXME
                .build();

        var applyResult = restTemplate.exchange("http://localhost:" + port + "/megamind/apply", HttpMethod.POST,
                new HttpEntity<>(applyRequest, headers), ResponseProto.TScenarioApplyResponse.class).getBody();

        // then
        assertEquals(TestSkill.RESPONSE_NORMAL, applyResult.getResponseBody().getLayout().getCards(0).getText());
    }

    @Test
    void testWrongProtoThrows400() {
        final byte[] byteRequest = InvalidRequestProto.TInvalidRequestProto.newBuilder().setBaseRequest("tst").build()
                .toByteArray();

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, "application/protobuf");
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        assertThrows(HttpClientErrorException.BadRequest.class, () ->
                restTemplate.postForEntity("http://localhost:" + port + "/megamind/run",
                        new HttpEntity<>(byteRequest, headers), String.class)
        );

        verify(accessLogger, new Times(1)).log(
                any(HttpServletRequest.class),
                any(HttpServletResponse.class),
                any(HttpMessageNotReadableException.class)
        );

    }

    @Test
    void testWrongMethodThrows405() {
        String request = requestBody("activate_fixed_skill_with_multiple_semantic_frame.json");

        HttpHeaders headers = getHttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        assertThrows(HttpClientErrorException.MethodNotAllowed.class,
                // GET but expected POST
                () -> restTemplate.exchange("http://localhost:" + port + "/megamind/run", HttpMethod.GET,
                        new HttpEntity<>(request, headers), ResponseProto.TScenarioRunResponse.class)
        );

        verify(accessLogger, new Times(1)).log(
                any(HttpServletRequest.class),
                any(HttpServletResponse.class),
                any(HttpRequestMethodNotSupportedException.class)
        );

    }

    @Disabled
    @Test
    void testApplyCallWithEmptyState() {
        HttpHeaders headers = getHttpHeaders();
        var body = "{\n" +
                "  \"base_request\": {\n" +
                "    \"request_id\": \"27599C38-87AB-4683-8321-3BFDE7FD3344\",\n" +
                "    \"random_seed\": \"16162782565235561938\",\n" +
                "    \"client_info\": {\n" +
                "      \"app_id\": \"ru.yandex.mobile.inhouse\",\n" +
                "      \"app_version\": \"490\",\n" +
                "      \"os_version\": \"12.4\",\n" +
                "      \"platform\": \"iphone\",\n" +
                "      \"uuid\": \"931add87717a492882744c064881cb23\",\n" +
                "      \"device_id\": \"911933E0-02BA-4BB9-8176-AA30C65B11D8\",\n" +
                "      \"lang\": \"ru-RU\",\n" +
                "      \"client_time\": \"20190823T215659\",\n" +
                "      \"timezone\": \"Europe/Moscow\",\n" +
                "      \"timestamp\": \"1566586619\",\n" +
                "      \"device_model\": \"iPhone\",\n" +
                "      \"device_manufacturer\": \"Apple\"\n" +
                "    },\n" +
                "    \"location\": {\n" +
                "      \"lat\": 55.73655052684742,\n" +
                "      \"lon\": 37.64022921072628,\n" +
                "      \"accuracy\": 65.0,\n" +
                "      \"recency\": 95.0\n" +
                "    },\n" +
                "    \"interfaces\": {\n" +
                "    },\n" +
                "    \"device_state\": {\n" +
                "    },\n" +
                "    \"state\": {}\n" +
                "  },\n" +
                "  \"arguments\": {\n" +
                "    \"@type\": \"type.googleapis.com/ru.yandex.alice.paskill.dialogovo.proto.RelevantApplyArgs\",\n" +
                "    \"skill_id\": \"" + CITY_GAME_SKILL_ID + "\",\n" +
                "    \"raw_utterance\": \"Запусти навык сказки\",\n" +
                "    \"utterance\": \"Запусти навык сказки\"\n" +
                "  }\n" +
                "}";
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        var result = restTemplate.exchange("http://localhost:" + port + "/megamind/apply", HttpMethod.POST,
                new HttpEntity<>(body, headers), String.class);
    }

    private String requestBody(String s) {
        return ResourceUtils.getStringResource(s);
    }

}
