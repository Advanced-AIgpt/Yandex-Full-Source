package ru.yandex.alice.paskill.dialogovo.service.wizard;

import java.util.Collections;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Intent;

import static ru.yandex.alice.paskill.dialogovo.service.wizard.IntentsPredicates.GENERAL_YANDEX_INTENT_PREDICATE;
import static ru.yandex.alice.paskill.dialogovo.service.wizard.IntentsPredicates.SKILL_INTENT_PREDICATE;

@Data
public class WizardResponse {

    public static final WizardResponse EMPTY = new WizardResponse(Collections.emptyMap());
    private final Map<String, Intent> forms;

    public boolean containsForm(String formName) {
        return forms.containsKey(formName);
    }

    public Map<String, Intent> getSkillIntents(SkillInfo skillInfo, SkillProcessRequest req) {
        return getSkillIntents(skillInfo, Optional.of((req)));
    }

    public Map<String, Intent> getSkillIntents(SkillInfo skillInfo, Optional<SkillProcessRequest> reqO) {
        String skillId = skillInfo.getId();
        final String skillPrefix = skillId + ".";
        final String replacePrefix = "^" + skillPrefix;
        boolean allowAdditionalValues = skillInfo.hasFeatureFlag(SkillFeatureFlag.ALLOW_ADDITIONAL_VALUES_IN_SLOTS);
        return forms.values().stream()
                .filter(intent ->
                        SKILL_INTENT_PREDICATE.test(intent, skillId)
                                || GENERAL_YANDEX_INTENT_PREDICATE.test(intent, reqO))
                .map(intent -> intent.withName(intent.getName().replaceFirst(replacePrefix, "")))
                .collect(
                        Collectors.toMap(
                                Intent::getName,
                                i -> allowAdditionalValues ? i : i.withoutAdditionalValues()));
    }


}
