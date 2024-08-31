package ru.yandex.alice.paskill.dialogovo.processor;

import java.io.IOException;
import java.time.Instant;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.json.JSONException;
import org.junit.jupiter.api.Test;
import org.skyscreamer.jsonassert.JSONAssert;
import org.skyscreamer.jsonassert.JSONCompareMode;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;

import ru.yandex.alice.kronstadt.test.ClientInfoTestUtils;
import ru.yandex.alice.kronstadt.test.DynamicValueTokenComparator;
import ru.yandex.alice.megamind.protos.scenarios.RequestProto;
import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Intent;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Nlu;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.NluEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.StringEntity;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.RequestEnrichmentData;
import ru.yandex.alice.paskill.dialogovo.service.ner.NerService;
import ru.yandex.alice.paskill.dialogovo.service.wizard.WizardResponse;
import ru.yandex.alice.paskill.dialogovo.service.wizard.WizardService;
import ru.yandex.alice.paskill.dialogovo.test.TestSkills;
import ru.yandex.alice.paskill.dialogovo.utils.ResourceUtils;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestParams;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

@SpringBootTest(classes = {TestConfigProvider.class}, webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class SkillRequestProcessorImplTest implements ClientInfoTestUtils {

    private static final String UTTERANCE = "давай поиграем в города";

    @Autowired
    private SkillRequestProcessorImpl skillRequestProcessor;

    @MockBean
    private WizardService wizardService;

    @MockBean
    private NerService nerService;

    @Autowired
    private ObjectMapper objectMapper;

    private Session session() {
        return Session.create(ActivationSourceType.DIRECT, Instant.now());
    }

    private Nlu nlu() {
        return Nlu.builder()
                .tokens(List.of("давай", "поиграем", "в", "города"))
                .entities(Collections.emptyList())
                .build();
    }

    private SkillInfo skillInfo(Set<String> skillFeatureFlags, Map<String, Object> userFeatureFlags) {
        return TestSkills.cityGameSkill(80, skillFeatureFlags, userFeatureFlags);
    }

    private SkillProcessRequest buildRequest(Set<String> skillFeatureFlags, Map<String, Object> userFeatureFlags) {
        return SkillProcessRequest.builder()
                .normalizedUtterance(UTTERANCE)
                .originalUtterance(UTTERANCE)
                .clientInfo(searchApp(DEFAULT_UUID))
                .requestTime(Instant.now())
                .locationInfo(Optional.empty())
                .skill(skillInfo(skillFeatureFlags, userFeatureFlags))
                .activationSourceType(ActivationSourceType.DIRECT)
                .session(Optional.of(Session.create(
                        "session_id",
                        1,
                        Instant.now(),
                        false,
                        Session.ProactiveSkillExitState.createEmpty(),
                        ActivationSourceType.DIRECT)))
                .viewState(Optional.empty())
                .requestTime(Instant.now())
                .mementoData(RequestProto.TMementoData.getDefaultInstance())
                .build();
    }

    void runTestMakeParams(WizardResponse mockWizardResponse, String expectedResponseFilename) throws IOException,
            JSONException {
        var skillInfo = skillInfo(Collections.emptySet(), Collections.emptyMap());
        var request = buildRequest(Collections.emptySet(), Collections.emptyMap());
        when(wizardService.requestWizardAsync(Optional.of(UTTERANCE), Optional.empty()))
                .thenReturn(CompletableFuture.completedFuture(mockWizardResponse));
        when(nerService.getNluAsync(UTTERANCE, skillInfo.getId()))
                .thenReturn(CompletableFuture.completedFuture(nlu()));
        RequestEnrichmentData enrichmentData = new RequestEnrichmentData(
                nlu(),
                Optional.empty(),
                Optional.empty(),
                mockWizardResponse,
                Collections.emptyList()
        );
        WebhookRequestParams params = skillRequestProcessor.makeParams(
                request,
                enrichmentData,
                session(),
                SourceType.USER);
        var expected = ResourceUtils.getStringResource("webhook_requests/" + expectedResponseFilename);
        JSONAssert.assertEquals(expected, objectMapper.writeValueAsString(params.getBody()),
                new DynamicValueTokenComparator(JSONCompareMode.STRICT));
    }

    @Test
    void sendEmptyIntents() throws IOException, JSONException {
        var mockWizardResponse = mock(WizardResponse.class);
        when(mockWizardResponse.getSkillIntents(any(SkillInfo.class), any(SkillProcessRequest.class)))
                .thenReturn(Collections.emptyMap());
        runTestMakeParams(mockWizardResponse, "intents_empty_map.json");
    }

    @Test
    void sendIntentWithStringSlots() throws IOException, JSONException {
        Map<String, Intent> intents = new HashMap<>();
        Map<String, NluEntity> slots = new HashMap<>();
        slots.put("test_slot", new StringEntity(1, 2, "навык"));
        intents.put("test_grammar", new Intent("test_grammar", slots));
        var mockWizardResponse = mock(WizardResponse.class);
        when(mockWizardResponse.getSkillIntents(
                any(SkillInfo.class),
                any(SkillProcessRequest.class))
        ).thenReturn(intents);
        runTestMakeParams(mockWizardResponse, "intents_string_slot.json");
    }
}
