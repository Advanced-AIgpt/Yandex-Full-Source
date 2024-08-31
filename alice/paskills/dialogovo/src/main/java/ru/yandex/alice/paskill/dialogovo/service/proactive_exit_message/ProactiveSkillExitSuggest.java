package ru.yandex.alice.paskill.dialogovo.service.proactive_exit_message;

import java.util.Set;

import lombok.AllArgsConstructor;
import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.service.ProactiveSkillResponseWrapper;
import ru.yandex.alice.paskill.dialogovo.service.wizard.WizardResponse;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Component
public class ProactiveSkillExitSuggest {

    private static final Logger logger = LogManager.getLogger();

    private static final String DO_NOT_UNDERSTAND_FORM = "paskills.internal.do_not_understand";
    private static final String FORM_WEAK_DEACTIVATE = "paskills.internal.deactivate_weak";

    private final ProactiveSkillResponseWrapper proactiveSkillResponseWrapper;
    private final MetricRegistry metricRegistry;
    private final long suggestExitAfterNDoNotUnderstandReplies;

    public ProactiveSkillExitSuggest(
            @Value("${proactiveExitSuggestConfig.suggestExitAfterNDoNotUnderstandReplies}")
                    long suggestExitAfterNDoNotUnderstandReplies,
            ProactiveSkillResponseWrapper proactiveSkillResponseWrapper,
            @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry
    ) {
        this.suggestExitAfterNDoNotUnderstandReplies = suggestExitAfterNDoNotUnderstandReplies;
        this.proactiveSkillResponseWrapper = proactiveSkillResponseWrapper;
        this.metricRegistry = metricRegistry;
    }

    public ApplyResult apply(
            SkillProcessRequest request,
            WebhookResponse webhookResponse,
            WizardResponse userQueryWizard,
            WizardResponse skillReplyWizard,
            Session.ProactiveSkillExitState state,
            long currentMessageId,
            Set<String> experiments
    ) {
        long doNotUnderstandCounter = state.getDoNotUnderstandReplyCounter();
        final boolean containsWeakDeactivateForm = userQueryWizard.containsForm(FORM_WEAK_DEACTIVATE);

        if (skillReplyWizard.containsForm(DO_NOT_UNDERSTAND_FORM)) {
            logger.info("Skill probably replied with fallback");
            doNotUnderstandCounter++;
        } else {
            doNotUnderstandCounter = 0;
        }

        if (endSession(webhookResponse)) {

            return new ApplyResult(false, containsWeakDeactivateForm, state.getDoNotUnderstandReplyCounter());

        } else if (alreadySuggestedInSession(state, currentMessageId, experiments)) {

            // we've already suggested exit at this session
            metricRegistry.rate("proactive_exit_suggest", Labels.of("suggested_exit", "false")).inc();
            return new ApplyResult(false, containsWeakDeactivateForm, state.getDoNotUnderstandReplyCounter());

        } else if (shouldSuggestExit(doNotUnderstandCounter, containsWeakDeactivateForm)) {

            logger.info("Suggesting exit (doNotUnderstandCounter={}, detectedWeakDeactivateForm={})",
                    doNotUnderstandCounter,
                    containsWeakDeactivateForm);
            metricRegistry.rate("proactive_exit_suggest", Labels.of("suggested_exit", "true")).inc();

            webhookResponse.getResponse().ifPresent(r ->
                    proactiveSkillResponseWrapper.appendExitSuggest(
                            r, request.getRandom(), request.getSkill().getName()
                    )
            );
            return new ApplyResult(true, containsWeakDeactivateForm, doNotUnderstandCounter);
        } else {

            metricRegistry.rate("proactive_exit_suggest", Labels.of("suggested_exit", "false")).inc();
            return new ApplyResult(false, false, doNotUnderstandCounter);
        }
    }

    private boolean shouldSuggestExit(long doNotUnderstandCounter, boolean containsWeakDeactivateForm) {
        return containsWeakDeactivateForm || doNotUnderstandCounter >= suggestExitAfterNDoNotUnderstandReplies;
    }

    private boolean endSession(WebhookResponse webhookResponse) {
        return webhookResponse.getResponse().map(Response::isEndSession).orElse(false);
    }

    private boolean alreadySuggestedInSession(Session.ProactiveSkillExitState state, long currentMessageId,
                                              Set<String> experiments) {
        return (!experiments.contains(Experiments.SUGGEST_EXIT_MORE_OFTEN)
                && state.getSuggestedExitAtMessageId() > 0)
                || (experiments.contains(Experiments.SUGGEST_EXIT_MORE_OFTEN)
                && currentMessageId - state.getSuggestedExitAtMessageId() <= 1);
    }

    @Data
    @AllArgsConstructor
    public static class ApplyResult {
        public static final ApplyResult EMPTY = new ApplyResult(false, false, 0);

        private boolean suggestedExit;
        private boolean containsWeakDeactivateForm;
        private long doNotUnderstandCounter;

        public boolean isSuggestedExit() {
            return suggestedExit;
        }

        public long getDoNotUnderstandCounter() {
            return doNotUnderstandCounter;
        }

        public boolean getContainsWeakDeactivateForm() {
            return containsWeakDeactivateForm;
        }
    }
}
