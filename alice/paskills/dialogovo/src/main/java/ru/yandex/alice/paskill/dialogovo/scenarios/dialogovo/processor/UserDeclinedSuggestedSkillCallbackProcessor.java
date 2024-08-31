package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillLayoutsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillSuggestUserDeclineDirective;

import static java.util.function.Predicate.not;

@Component
public class UserDeclinedSuggestedSkillCallbackProcessor implements RunRequestProcessor<DialogovoState> {

    private final SkillLayoutsFactory skillLayoutsFactory;

    public UserDeclinedSuggestedSkillCallbackProcessor(SkillLayoutsFactory skillLayoutsFactory) {
        this.skillLayoutsFactory = skillLayoutsFactory;
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.SKILL_SUGGEST_USER_DECLINE;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return onCallbackDirective(SkillSuggestUserDeclineDirective.class)
                .and(not(IS_IN_SKILL))
                .test(request);
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        return new RunOnlyResponse<>(new ScenarioResponseBody<>(
                skillLayoutsFactory.createSuggestUserDeclineAnswerLayout(request.getRandom()),
                context.getAnalytics()
                        .setIntent(Intents.EXTERNAL_SKILL_DISCOVERY_DECLINE)
                        .toAnalyticsInfo(),
                false));
    }
}
