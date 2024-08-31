package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.function.Predicate;

import ru.yandex.alice.kronstadt.core.AliceHandledException;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.SingleSemanticFrameRunProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;

import static ru.yandex.alice.paskill.dialogovo.domain.Experiments.DISABLE_VOLUME_CONTROL;
import static ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor.getSkillId;

public abstract class SoundProcessor implements SingleSemanticFrameRunProcessor<DialogovoState> {

    private final Predicate<MegaMindRequest<DialogovoState>> canProcessPredicate;
    private final String semanticFrameType;
    private final RunRequestProcessorType runRequestProcessorType;
    protected final SuggestButtonFactory suggestButtonFactory;
    private final SkillProvider skillProvider;
    protected final Phrases phrases;


    protected static final int MAX_VOLUME = 10;
    protected static final int VERY_HIGH_VOLUME = 9;
    protected static final int HIGH_VOLUME = 8;
    protected static final int MID_VOLUME = 4;
    protected static final int QUIET_VOLUME = 2;
    protected static final int VERY_QUIET_VOLUME = 1;
    protected static final int MIN_SOUNDING_VOLUME = 1;
    protected static final int MIN_VOLUME = 0;

    protected SoundProcessor(
            String semanticFrameType,
            RunRequestProcessorType runRequestProcessorType,
            SuggestButtonFactory suggestButtonFactory,
            SkillProvider skillProvider,
            Phrases phrases) {
        this.semanticFrameType = semanticFrameType;
        this.runRequestProcessorType = runRequestProcessorType;
        this.suggestButtonFactory = suggestButtonFactory;
        this.skillProvider = skillProvider;
        this.phrases = phrases;

        Predicate<MegaMindRequest<DialogovoState>> hasSoundLevel =
                req -> req.getDeviceStateO().isPresent() &&
                        // https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/sound.cpp?rev=6522485#L99
                        req.getDeviceStateO().get().getSoundLevel() != -1;
        Predicate<MegaMindRequest<DialogovoState>> soundControlEnabled =
                req -> !req.hasExperiment(DISABLE_VOLUME_CONTROL);

        canProcessPredicate = IS_IN_SKILL
                .and(IS_SMART_SPEAKER_OR_TV)
                .and(hasFrame(this.semanticFrameType))
                .and(hasSoundLevel)
                .and(soundControlEnabled);
    }

    protected SkillInfo getCurrentSkill(MegaMindRequest<DialogovoState> request) {
        return getSkillId(request)
                .flatMap(skillProvider::getSkill)
                .orElseThrow(() -> new AliceHandledException(
                        "Навык не найден", ErrorAnalyticsInfoAction.REQUEST_FAILURE, "Произошла ошибка"));
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return canProcessPredicate.test(request);
    }

    @Override
    public String getSemanticFrame() {
        return semanticFrameType;
    }

    @Override
    public RunRequestProcessorType getType() {
        return runRequestProcessorType;
    }
}
