package ru.yandex.alice.paskill.dialogovo.service;

import java.util.List;
import java.util.Map;
import java.util.Optional;

import com.google.common.collect.ImmutableMap;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillActivationPhraseSearcher;
import ru.yandex.alice.paskill.dialogovo.service.normalizer.NormalizationServiceImpl;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.anySet;
import static org.mockito.Mockito.when;

@SpringJUnitConfig(classes = {
        TestConfigProvider.class,
        NormalizationServiceImpl.class,
        SkillDetectorImpl.class
})
public class SkillDetectorTest {

    @Autowired
    private SkillDetector skillDetector;
    @MockBean
    private SkillActivationPhraseSearcher activationPhraseSearcher;

    @Test
    public void testExternalSkillActivateFound() {
        when(activationPhraseSearcher.findSkillsByPhrases(anySet()))
                .thenReturn(Map.of("юниткредит", List.of("1")));

        String value = "юниткредит банк филиал номер два";

        Optional<SkillDetector.DetectedSkill> detectedSkillO =
                skillDetector.tryDetectSkill(value, false);

        assertTrue(detectedSkillO.isPresent());
        assertEquals(detectedSkillO.get().getSkillId(), "1");
    }

    @Test
    public void testExternalSkillActivateFoundNormalized() {
        when(activationPhraseSearcher.findSkillsByPhrases(anySet()))
                .thenReturn(Map.of("100 к 1", List.of("1")));

        String value = "сто к одному";

        Optional<SkillDetector.DetectedSkill> detectedSkillO =
                skillDetector.tryDetectSkill(value, false);

        assertTrue(detectedSkillO.isPresent());
        assertEquals(detectedSkillO.get().getSkillId(), "1");
    }

    @Test
    public void testExternalSkillActivateFoundInTheMiddle() {
        when(activationPhraseSearcher.findSkillsByPhrases(anySet()))
                .thenReturn(Map.of("юниткредит", List.of("1"),
                        // longest must win
                        "банк филиал", List.of("2")));


        String value = "юниткредит банк филиал номер два";
        Optional<SkillDetector.DetectedSkill> detectedSkillO = skillDetector.tryDetectSkill(value, false);

        assertTrue(detectedSkillO.isPresent());
        assertEquals(detectedSkillO.get().getSkillId(), "2");
        assertEquals(detectedSkillO.get().getSuffix(), "номер 2");
    }

    @Test
    public void testExternalSkillActivateFoundAndMostRelevant() {
        when(activationPhraseSearcher.findSkillsByPhrases(anySet()))
                .thenReturn(ImmutableMap.of(
                        //must win with same len but closer to start
                        "юниткредит банк", List.of("1"),
                        "банк филиал", List.of("2")));


        String value = "юниткредит банк филиал номер два";
        Optional<SkillDetector.DetectedSkill> detectedSkillO = skillDetector.tryDetectSkill(value, false);

        assertTrue(detectedSkillO.isPresent());
        assertEquals(detectedSkillO.get().getSkillId(), "1");
        assertEquals(detectedSkillO.get().getSuffix(), "филиал номер 2");
    }

    @Test
    public void testExternalSkillActivateWithRequestDoNotActivatesIntheMiddle() {
        when(activationPhraseSearcher.findSkillsByPhrases(anySet())).thenReturn(
                Map.of("вторую память", List.of("1"))
        );

        String value = "попроси купить колбасы вторую память";
        Optional<SkillDetector.DetectedSkill> detectedSkillO = skillDetector.tryDetectSkill(value, true);

        assertTrue(detectedSkillO.isEmpty());
    }

    @Test
    public void testExternalSkillActivateWithRequestActivatesFromStartWithLongestFrame() {
        when(activationPhraseSearcher.findSkillsByPhrases(anySet())).thenReturn(
                Map.of("2 память", List.of("1"))
        );

        String value = "вторую память купить колбасы";
        Optional<SkillDetector.DetectedSkill> detectedSkillO = skillDetector.tryDetectSkill(value, true);

        assertTrue(detectedSkillO.isPresent());
        assertEquals(detectedSkillO.get().getSkillId(), "1");
        assertEquals(detectedSkillO.get().getSuffix(), "купить колбасы");
    }
}
