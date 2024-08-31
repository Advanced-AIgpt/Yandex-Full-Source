package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.feedback;

import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.CommitNeededResponse;
import ru.yandex.alice.kronstadt.core.CommitResult;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.CommittingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.FeedbackSavedEvent;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.SaveFeedbackApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.SaveFeedbackDirective;
import ru.yandex.alice.paskill.dialogovo.service.api.ApiException;
import ru.yandex.alice.paskill.dialogovo.service.api.ApiService;

@Component
public class SaveFeedbackRunProcessor implements CommittingRunProcessor<DialogovoState, SaveFeedbackApplyArguments> {

    private static final Logger logger = LogManager.getLogger();

    private final RequestContext requestContext;
    private final ApiService apiService;
    private final SaveFeedbackNlg feedbackNlg;
    private final SkillProvider skillProvider;

    public SaveFeedbackRunProcessor(
            RequestContext requestContext,
            ApiService apiService,
            SaveFeedbackNlg feedbackNlg,
            SkillProvider skillProvider
    ) {
        this.requestContext = requestContext;
        this.apiService = apiService;
        this.feedbackNlg = feedbackNlg;
        this.skillProvider = skillProvider;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return request.getInput().isCallback(SaveFeedbackDirective.class);
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.SAVE_FEEDBACK;
    }

    @Override
    public BaseRunResponse process(
            Context context,
            MegaMindRequest<DialogovoState> request
    ) {
        SaveFeedbackDirective directive = request.getInput().getDirective(SaveFeedbackDirective.class);
        String skillId = directive.getSkillId();
        final Optional<SkillInfo> skill = skillProvider.getSkill(skillId);
        var layoutBuilder = Layout.builder()
                .textCard("Спасибо за оценку!")
                .outputSpeech("Спасибо за оценку!");
        var session = request.getStateO().flatMap(DialogovoState::getSessionO);
        context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_SAVE_FEEDBACK);

        if (skill.isPresent()) {
            final FeedbackMark mark = directive.getFeedbackMark();
            context.getAnalytics().addObject(new SkillAnalyticsInfoObject(skill.get()));
            context.getAnalytics().addEvent(FeedbackSavedEvent.create(mark));
            SaveFeedbackApplyArguments applyArguments = new SaveFeedbackApplyArguments(
                    mark,
                    skillId);
            ScenarioResponseBody<DialogovoState> scenarioResponseBody = feedbackNlg.generateFeedbackReceivedResponse(
                    context,
                    skillId,
                    session,
                    layoutBuilder,
                    request.getServerTime(),
                    request.isVoiceSession(),
                    request.getStateO());
            return new CommitNeededResponse<>(scenarioResponseBody, applyArguments);
        } else {
            ScenarioResponseBody<DialogovoState> scenarioResponseBody = feedbackNlg.generateFeedbackReceivedResponse(
                    context,
                    skillId,
                    session,
                    layoutBuilder,
                    request.getServerTime(),
                    request.isVoiceSession(),
                    request.getStateO());
            logger.error("Failed to save feedback for skill {}: skill not found", skillId);
            return new RunOnlyResponse<>(scenarioResponseBody);
        }
    }

    @Override
    public Class<SaveFeedbackApplyArguments> getApplyArgsType() {
        return SaveFeedbackApplyArguments.class;
    }

    @Override
    public CommitResult commit(MegaMindRequest<DialogovoState> request, Context context,
                               SaveFeedbackApplyArguments applyArguments) {
        boolean saved = saveFeedback(request, applyArguments.getSkillId(), context);
        return saved ? CommitResult.Success : CommitResult.Error;
    }

    private boolean saveFeedback(MegaMindRequest<DialogovoState> request, String skillId, Context context) {
        var directive = request.getInput().getDirective(SaveFeedbackDirective.class);
        context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_SAVE_FEEDBACK);
        var mark = directive.getFeedbackMark();
        logger.info("Feedback {} for skill {}", mark.getTitle(), directive.getSkillId());

        var userTicket = requestContext.getCurrentUserTicket();
        try {
            if (userTicket != null) {
                apiService.putFeedbackMark(userTicket, skillId, mark, request.getClientInfo().getUuid());
                context.getAnalytics().addEvent(FeedbackSavedEvent.create(mark));
                logger.info("Saved mark {} for skill {}", mark.getTitle(), directive.getSkillId());
                return true;
            } else {
                logger.warn("User ticket not found, won't save feedback");
                return false;
            }
        } catch (ApiException e) {
            logger.error("Api threw error, did not save mark {} for skill {}",
                    mark.getTitle(),
                    directive.getSkillId(),
                    e);
            return false;
        }
    }
}
