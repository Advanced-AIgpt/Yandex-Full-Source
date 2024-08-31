package ru.yandex.alice.paskill.dialogovo.scenarios;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.ActionRef.NluHint;
import ru.yandex.alice.kronstadt.core.directive.CallbackDirective;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameSlot;
import ru.yandex.alice.megamind.protos.common.FrameProto;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.semanticframes.FixedActivate;

import static ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotTypes.ACTIVATION_SOURCE_TYPE;

@Component
public class SkillsSuggestVoiceButtonFactory {

    private static final NluHint NLU_HINT_EXTERNAL_SKILL_SUGGEST_CONFIRM =
            new NluHint(SkillsSemanticFrames.ALICE_EXTERNAL_SKILL_SUGGEST_CONFIRM);

    private static final NluHint NLU_HINT_ALICE_EXTERNAL_SKILL_SUGGEST_DECLINE =
            new NluHint(SkillsSemanticFrames.ALICE_EXTERNAL_SKILL_SUGGEST_DECLINE);

    private static final NluHint NLU_HINT_ALICE_COMMON_DECLINE =
            new NluHint(SkillsSemanticFrames.ALICE_COMMON_DECLINE);

    public ActionRef createConfirmSkillActivationVoiceButton(
            SkillInfo skill,
            ActivationSourceType activationSourceType
    ) {
        return createConfirmSkillActivationVoiceButton(skill, activationSourceType, null);
    }

    public ActionRef createConfirmSkillActivationVoiceButton(
            SkillInfo skill,
            ActivationSourceType activationSourceType,
            FrameProto.TTypedSemanticFrame activationSemanticFrame
    ) {
        // If we get activationSemanticFrame we send TSF with this activation frame, else SF without it
        if (activationSemanticFrame != null) {
            return ActionRef.withTypedSemanticFrame(
                    "",
                    new FixedActivate("", null, skill.getId(), activationSourceType, activationSemanticFrame),
                    NLU_HINT_EXTERNAL_SKILL_SUGGEST_CONFIRM);
        }

        var fixedSkillIdActivateSlot = SemanticFrameSlot.create(
                SkillsSemanticSlotTypes.FIXED_SKILL_ID,
                skill.getId(),
                SkillsSemanticSlotEntityTypes.ACTIVATION_PHRASE_EXTERNAL_SKILL_ID);

        var skillActivationSourceSlot = SemanticFrameSlot.create(
                ACTIVATION_SOURCE_TYPE,
                activationSourceType.value(),
                SkillsSemanticSlotEntityTypes.ACTIVATION_SOURCE_TYPE);

        return ActionRef.withSemanticFrame(
                SemanticFrame.create(SkillsSemanticFrames.EXTERNAL_SKILL_FIXED_ACTIVATE,
                        fixedSkillIdActivateSlot, skillActivationSourceSlot),
                NLU_HINT_EXTERNAL_SKILL_SUGGEST_CONFIRM);
    }

    public ActionRef createDeclineVoiceButton(CallbackDirective directive) {
        return ActionRef.withCallback(
                directive,
                NLU_HINT_ALICE_EXTERNAL_SKILL_SUGGEST_DECLINE);
    }

    public ActionRef createDeclineDoNothingButton() {
        return ActionRef.withSemanticFrame(
                SemanticFrame.create(SkillsSemanticFrames.GENERAL_PROACTIVITY_DISAGREE_DO_NOTHING),
                NLU_HINT_ALICE_COMMON_DECLINE);
    }


}
