package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.feedback;

import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.input.Input;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.UnknownFeedbackEvent;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.NewSessionDirective;

@Component
public class LogNonsenseFeedbackRunProcessor implements RunRequestProcessor<DialogovoState> {

    private static final Logger logger = LogManager.getLogger();

    private final SkillProvider skillProvider;
    private final SaveFeedbackNlg saveFeedbackNlg;
    private final Phrases phrases;

    public LogNonsenseFeedbackRunProcessor(SkillProvider skillProvider, SaveFeedbackNlg feedbackNlg, Phrases phrases) {
        this.skillProvider = skillProvider;
        this.saveFeedbackNlg = feedbackNlg;
        this.phrases = phrases;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        boolean isFeedbackRequested = request.getStateO()
                .map(state -> state.getFeedbackRequestedForSkillIdO().isPresent())
                .orElse(false);
        boolean isNewSession = request.getInput().isCallback(NewSessionDirective.class);
        return isFeedbackRequested && !isNewSession;
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.SAVE_NONSENSE_FEEDBACK;
    }

    @Override
    public BaseRunResponse process(
            Context context,
            MegaMindRequest<DialogovoState> request
    ) {
        final var input = request.getInput();
        final var utterance = input instanceof Input.Text ? ((Input.Text) input).getOriginalUtterance() : null;
        final String skillId = request.getStateO().get().getFeedbackRequestedForSkillIdO().get();
        logger.info("Feedback for skill {}: \"{}\"", skillId, utterance);
        final String text = phrases.getRandom("feedback_nonsense", request.getRandom());
        var layoutBuilder = Layout.builder()
                .textCard(text)
                .shouldListen(false);
        if (request.isVoiceSession()) {
            layoutBuilder.outputSpeech(text);
        }
        var session = request.getStateO().flatMap(DialogovoState::getSessionO);
        context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_SAVE_FEEDBACK);
        context.getAnalytics().addEvent(UnknownFeedbackEvent.INSTANCE);
        final Optional<SkillInfo> skill = skillProvider.getSkill(skillId);
        skill.ifPresent(s -> context.getAnalytics().addObject(new SkillAnalyticsInfoObject(s)));
        var responseBody = saveFeedbackNlg.generateFeedbackReceivedResponse(
                context,
                skillId,
                session,
                layoutBuilder,
                request.getServerTime(),
                request.isVoiceSession(),
                request.getStateO());
        return new RunOnlyResponse<>(responseBody);
    }

}
