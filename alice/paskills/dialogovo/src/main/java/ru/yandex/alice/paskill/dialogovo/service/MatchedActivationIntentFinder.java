package ru.yandex.alice.paskill.dialogovo.service;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.service.wizard.WizardResponse;
import ru.yandex.alice.paskill.dialogovo.service.wizard.WizardService;

/**
 * Responsible for finding matching activation grammars by skill
 */
@Component
public class MatchedActivationIntentFinder {
    private static final Logger logger = LogManager.getLogger();

    private final SkillProvider skillProvider;
    private final WizardService wizardService;

    public MatchedActivationIntentFinder(SkillProvider skillProvider, WizardService wizardService) {
        this.skillProvider = skillProvider;
        this.wizardService = wizardService;
    }

    public Set<String> find(String skillId, String input) {
        return find(List.of(skillId), input).getOrDefault(skillId, Set.of());
    }

    public Map<String, Set<String>> find(List<String> skillIds, String input) {
        logger.info("Start finding activation intents for skills [{}]", skillIds);

        Map<String, Set<String>> activationIntentsBySkillId = skillProvider.getActivationIntentFormNames(skillIds);

        // every skill in map has grammars_base64 filled and some intents with isActivation=true
        Map<String, SkillInfo> skillsWithActivationIntents = activationIntentsBySkillId
                .keySet()
                .stream()
                .map(skillProvider::getSkill)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .filter(skillInfo -> skillInfo.getGrammarsBase64().isPresent())
                .collect(Collectors.toMap(SkillInfo::getId, Function.identity()));

        Map<String, Set<String>> matchedSkillIdsToIntents = new HashMap<>();

        if (!skillsWithActivationIntents.isEmpty()) {
            String allSkillsGrammar = skillsWithActivationIntents.values().stream()
                    .map(SkillInfo::getGrammarsBase64)
                    .map(Optional::get)
                    .collect(Collectors.joining(";"));

            WizardResponse wizardResponse = wizardService.requestWizard(
                    Optional.of(input),
                    Optional.of(allSkillsGrammar)
            );

            for (var entry : skillsWithActivationIntents.entrySet()) {
                String skillId = entry.getKey();

                Set<String> activationIntents = activationIntentsBySkillId.get(skillId);
                Set<String> matchedIntents = wizardResponse.getSkillIntents(
                        entry.getValue(), Optional.empty())
                        .keySet();

                matchedIntents.retainAll(activationIntents);

                if (matchedIntents.size() > 0) {
                    matchedSkillIdsToIntents.put(skillId, matchedIntents);
                }
            }
        }

        return matchedSkillIdsToIntents;
    }
}
