package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.feedback;

import java.util.List;
import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.layout.Button;
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
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.FeedbackRequestNlg;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.FeedbackRequestedEvent;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.RequestFeedbackFormDirective;

@Component
public class RequestFeedbackFormRunProcessor implements RunRequestProcessor<DialogovoState> {

    private static final Logger logger = LogManager.getLogger();

    private final SkillProvider skillProvider;
    private final FeedbackRequestNlg feedbackRequestNlg;
    private final RequestContext requestContext;
    private final Phrases phrases;

    public RequestFeedbackFormRunProcessor(
            SkillProvider skillProvider,
            FeedbackRequestNlg feedbackRequestNlg,
            RequestContext requestContext,
            Phrases phrases
    ) {
        this.skillProvider = skillProvider;
        this.feedbackRequestNlg = feedbackRequestNlg;
        this.requestContext = requestContext;
        this.phrases = phrases;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        if (!request.getInput().isCallback(RequestFeedbackFormDirective.class)) {
            return false;
        }
        final String skillId = request.getInput().getDirective(RequestFeedbackFormDirective.class).getSkillId();
        final Optional<SkillInfo> skill = skillProvider.getSkill(skillId);
        return skill.isPresent() && !skill.get().isHideInStore();
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.REQUEST_FEEDBACK_FORM;
    }

    @Override
    public BaseRunResponse process(
            Context context,
            MegaMindRequest<DialogovoState> request
    ) {
        context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_FEEDBACK_REQUEST);
        final String skillId = request.getInput().getDirective(RequestFeedbackFormDirective.class).getSkillId();
        final Optional<SkillInfo> skill = skillProvider.getSkill(skillId);
        skill.ifPresent(s -> context.getAnalytics().addObject(new SkillAnalyticsInfoObject(s)));
        if (requestContext.getCurrentUserId() == null) {
            return fallbackResponse(request, context, "feedback.no_auth");
        }
        if (skill.isPresent()) {
            List<Button> buttons = feedbackRequestNlg.getSuggests(skill.get());
            var textWithTts = feedbackRequestNlg.render(request.getRandom(), skill.get());
            var layout = Layout.builder()
                    .outputSpeech(request.isVoiceSession() ? textWithTts.getTts() : "")
                    .textCard(textWithTts.getText())
                    .shouldListen(request.isVoiceSession())
                    .suggests(buttons)
                    .build();
            context.getAnalytics().addEvent(FeedbackRequestedEvent.INSTANCE);
            DialogovoState state = request.getStateO()
                    .map(st -> st.withFeedbackRequest(skillId))
                    .orElse(DialogovoState.createSkillFeedbackRequest(skillId));
            ScenarioResponseBody<DialogovoState> body = new ScenarioResponseBody<>(
                    layout,
                    Optional.of(state),
                    context.getAnalytics().toAnalyticsInfo(),
                    true);
            return new RunOnlyResponse<>(body);
        } else {
            return fallbackResponse(request, context, "rate.button.skill_not_found");
        }
    }

    private RunOnlyResponse<DialogovoState> fallbackResponse(
            MegaMindRequest<DialogovoState> request,
            Context context,
            String phraseId
    ) {
        final String text = phrases.getRandom(phraseId, request.getRandom());
        var layout = Layout.builder()
                .textCard(text)
                .shouldListen(request.isVoiceSession());
        if (request.isVoiceSession()) {
            layout.outputSpeech(text);
        }
        ScenarioResponseBody<DialogovoState> body = new ScenarioResponseBody<>(
                layout.build(),
                request.getStateO(),
                context.getAnalytics().toAnalyticsInfo(),
                false);
        return new RunOnlyResponse<>(body);
    }

}
