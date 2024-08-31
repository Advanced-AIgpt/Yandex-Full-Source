package ru.yandex.alice.paskill.dialogovo.scenarios;

import java.util.Map;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.ActionRef.NluHint;
import ru.yandex.alice.kronstadt.core.semanticframes.MusicPlay;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameSlot;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType;
import ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.SaveFeedbackDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.suggest.RespondWithSilenceDirective;

import static ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark.ACCEPTABLE;
import static ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark.BAD;
import static ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark.EXCELLENT;
import static ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark.GOOD;
import static ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark.VERY_BAD;

@Component
public class VoiceButtonFactory {

    private static final NluHint NLU_HINT_EXTERNAL_SKILL_SUGGEST_CONFIRM =
            new NluHint(SkillsSemanticFrames.ALICE_EXTERNAL_SKILL_SUGGEST_CONFIRM);

    private static final NluHint NLU_HINT_ALICE_EXTERNAL_SKILL_SUGGEST_DECLINE =
            new NluHint(SkillsSemanticFrames.ALICE_EXTERNAL_SKILL_SUGGEST_DECLINE);

    private static final NluHint NLU_HINT_FEEDBACK_EXCELLENT = new NluHint(SemanticFrames.FEEDBACK_EXCELLENT);

    private static final NluHint NLU_HINT_FEEDBACK_GOOD = new NluHint(SemanticFrames.FEEDBACK_GOOD);

    private static final NluHint NLU_HINT_FEEDBACK_ACCEPTABLE = new NluHint(SemanticFrames.FEEDBACK_ACCEPTABLE);

    private static final NluHint NLU_HINT_FEEDBACK_BAD = new NluHint(SemanticFrames.FEEDBACK_BAD);

    private static final NluHint NLU_HINT_FEEDBACK_VERY_BAD = new NluHint(SemanticFrames.FEEDBACK_VERY_BAD);


    public Map<String, ActionRef> createFeedbackButtons(String skillId) {
        return Map.of(
                "excellent", actionRef(skillId, EXCELLENT, NLU_HINT_FEEDBACK_EXCELLENT),
                "good", actionRef(skillId, GOOD, NLU_HINT_FEEDBACK_GOOD),
                "acceptable", actionRef(skillId, ACCEPTABLE, NLU_HINT_FEEDBACK_ACCEPTABLE),
                "bad", actionRef(skillId, BAD, NLU_HINT_FEEDBACK_BAD),
                "very_bad", actionRef(skillId, VERY_BAD, NLU_HINT_FEEDBACK_VERY_BAD)
        );
    }

    public Map<String, ActionRef> createMusicSuggest() {
        return Map.of(
                "confirm", ActionRef.withTypedSemanticFrame(
                        "включи музыку",
                        MusicPlay.INSTANCE,
                        NLU_HINT_EXTERNAL_SKILL_SUGGEST_CONFIRM),
                // TODO: use alice.do_nothing semantic frame after PASKILLS-6296 is released
                "decline", ActionRef.withCallback(
                        RespondWithSilenceDirective.INSTANCE,
                        NLU_HINT_ALICE_EXTERNAL_SKILL_SUGGEST_DECLINE)
        );
    }

    public Map<String, ActionRef> createRecipeSuggest(Recipe recipe) {
        SemanticFrameSlot slot = SemanticFrameSlot.create(
                SemanticSlotType.RECIPE.getValue(),
                recipe.getId(),
                SemanticSlotEntityType.RECIPE);
        return Map.of(
                "confirm",
                ActionRef.withSemanticFrame(
                        SemanticFrame.create(SemanticFrames.RECIPE_SELECT_RECIPE, slot),
                        NLU_HINT_EXTERNAL_SKILL_SUGGEST_CONFIRM),
                "decline", ActionRef.withCallback(
                        // TODO: до рефакторинга callback директив эта штука ловилась MusicSuggestDeclineProcessor,
                        //  что странно, не понятно почему тут и выше использутся одна и та же директива
                        RespondWithSilenceDirective.INSTANCE,
                        NLU_HINT_ALICE_EXTERNAL_SKILL_SUGGEST_DECLINE)
        );
    }

    private ActionRef actionRef(String skillId, FeedbackMark mark, NluHint nluHint) {
        return ActionRef.withCallback(new SaveFeedbackDirective(skillId, mark), nluHint);
    }
}
