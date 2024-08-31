package ru.yandex.alice.paskill.dialogovo.service.wizard;

import java.io.IOException;
import java.util.Collections;
import java.util.Map;
import java.util.Optional;

import com.fasterxml.jackson.databind.ObjectMapper;
import okhttp3.mockwebserver.MockResponse;
import okhttp3.mockwebserver.MockWebServer;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.EnableAutoConfiguration;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.kronstadt.core.domain.AudioPlayerActivityState;
import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;
import ru.yandex.alice.paskill.dialogovo.config.WizardConfig;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Intent;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.NluEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.StringEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerState;
import ru.yandex.alice.paskill.dialogovo.test.TestSkill;
import ru.yandex.alice.paskill.dialogovo.test.TestSkills;
import ru.yandex.alice.paskill.dialogovo.utils.ResourceUtils;
import ru.yandex.alice.paskill.dialogovo.utils.executor.TestExecutorsFactory;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.Mockito.when;

@SpringBootTest(classes = {
        TestConfigProvider.class,
        WizardServiceImplTest.TestConfiguration.class,
        TestSkill.class
}, webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class WizardServiceImplTest {

    private static final String INTENT_CONFIRM = "YANDEX.CONFIRM";
    private static final String INTENT_PLAYER_NEXT = "YANDEX.PLAYER.NEXT";

    private MockWebServer mockWizardServer;

    private WizardServiceImpl wizardService;

    @Autowired
    private TestSkill testSkill;
    @Autowired
    private ObjectMapper objectMapper;

    @BeforeEach
    void setUp() throws IOException {
        mockWizardServer = new MockWebServer();
        mockWizardServer.start();
        WizardConfig wizardConfig = new WizardConfig(mockWizardServer.url("/").toString(), 1000, 100, 3600, 0);
        wizardService = new WizardServiceImpl(wizardConfig, new RestTemplate(),
                TestExecutorsFactory.newSingleThreadExecutor(), objectMapper, false);
    }

    @AfterEach
    void tearDown() throws IOException {
        mockWizardServer.shutdown();
    }

    private void runTest(String wizardResponseFilename, Map<String, Intent> expectedIntents) throws IOException {
        String wizardResponseBody = ResourceUtils.getStringResource("wizard_responses/" + wizardResponseFilename);
        MockResponse wizardHttpResponse = new MockResponse()
                .addHeader("Content-Type: application/json;charset=UTF-8")
                .setBody(wizardResponseBody);
        mockWizardServer.enqueue(wizardHttpResponse);
        var skillInfo = TestSkills.cityGameSkill(1, Collections.emptySet(), Collections.emptyMap());
        WizardResponse wizardResponse = wizardService.requestWizard(Optional.of("utterance"), Optional.of(
                "grammarb64"));
        SkillProcessRequest reqMock = Mockito.mock(SkillProcessRequest.class);
        when(reqMock.getAudioPlayerState()).thenReturn(Optional.of(new AudioPlayerState(null, 1,
                AudioPlayerActivityState.IDLE)));
        assertEquals(expectedIntents, wizardResponse.getSkillIntents(skillInfo, reqMock));
    }

    @Test
    void noMatchInWizardResponseReturnsEmptyMap() throws IOException {
        runTest("no_match.json", Collections.emptyMap());
    }

    @Test
    void testTextSlot() throws IOException {
        StringEntity slot = new StringEntity(1, 2, "тест");
        Map<String, NluEntity> slots = Map.of("test_slot", slot);
        Map<String, Intent> expectedIntents = Map.of(INTENT_CONFIRM, new Intent(INTENT_CONFIRM, slots));
        runTest("text_slot.json", expectedIntents);
    }

    @Test
    void testPlayerIntentSlot() throws IOException {
        StringEntity slot = new StringEntity(1, 2, "тест");
        Map<String, NluEntity> slots = Map.of("test_slot", slot);
        Map<String, Intent> expectedIntents = Map.of(INTENT_PLAYER_NEXT, new Intent(INTENT_PLAYER_NEXT, slots));
        runTest("with_player_intent.json", expectedIntents);
    }

    /**
     * alice_grammars.json contains Alice's grammar alice.external_skill_activate_with_request
     * which should not be passed to external skill
     *
     * @throws IOException
     */
    @Test
    void filterOutAliceGrammars() throws IOException {
        StringEntity slot = new StringEntity(1, 2, "тест");
        Map<String, NluEntity> slots = Map.of("test_slot", slot);
        Map<String, Intent> expectedIntents = Map.of(INTENT_CONFIRM, new Intent(INTENT_CONFIRM, slots));
        runTest("alice_grammar.json", expectedIntents);
    }

    /**
     * skill_grammar.json contains grammar matches for two external skills.
     * Only one of them ("grammar", not "grammar_2") should be passed to webhook
     * Skill ID should be removed from grammar's name.
     *
     * @throws IOException
     */
    @Test
    void leavesOnlySkillsGrammar() throws IOException {
        StringEntity slot = new StringEntity(1, 2, "тест");
        Map<String, NluEntity> slots = Map.of("test_slot", slot);
        Map<String, Intent> expectedIntents = Map.of("my_grammar", new Intent("my_grammar", slots));
        runTest("skill_grammar.json", expectedIntents);
    }

    @Test
    void filterOutsNotAllowedYandexGeneralIntents() throws IOException {
        runTest("not_allowed_general_intent.json", Map.of());
    }

    @Test
    void httpErrorFromWizardConvertsToEmptyIntents() {
        mockWizardServer.enqueue(new MockResponse().setResponseCode(500));
        var skillInfo = TestSkills.cityGameSkill(1, Collections.emptySet(), Collections.emptyMap());
        WizardResponse wizardResponse = wizardService.requestWizard(Optional.of(""), Optional.of(""));
        SkillProcessRequest reqMock = Mockito.mock(SkillProcessRequest.class);
        assertEquals(Collections.emptyMap(), wizardResponse.getSkillIntents(skillInfo, reqMock));
    }

    @Configuration
    @EnableAutoConfiguration
    static class TestConfiguration {

    }

}
