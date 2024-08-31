package ru.yandex.alice.paskill.dialogovo.processor;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;

import com.google.common.base.Strings;
import lombok.val;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.layout.Button;
import ru.yandex.alice.kronstadt.core.layout.TextCard;
import ru.yandex.alice.kronstadt.core.layout.TextWithTts;
import ru.yandex.alice.kronstadt.core.layout.div.DivBody;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.domain.UserFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.converter.CardConverter;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Directives;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.AudioPlayerAction;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.AudioPlayerActionType;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.RequestEnrichmentData;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SuggestedSkillExitInfoObject;
import ru.yandex.alice.paskill.dialogovo.service.ProactiveSkillResponseWrapper;
import ru.yandex.alice.paskill.dialogovo.service.abuse.AbuseApplier;
import ru.yandex.alice.paskill.dialogovo.service.proactive_exit_message.ProactiveSkillExitSuggest;
import ru.yandex.alice.paskill.dialogovo.service.wizard.WizardResponse;
import ru.yandex.alice.paskill.dialogovo.service.wizard.WizardService;
import ru.yandex.alice.paskill.dialogovo.webhook.client.SolomonHelper;
import ru.yandex.misc.lang.StringUtils;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Component
class OrdinaryResponseHandler implements WebhookResponseHandler {

    private static final Logger logger = LogManager.getLogger();

    private final CardConverter cardConverter;
    private final DialogovoDirectiveFactory directiveFactory;
    private final ProactiveSkillResponseWrapper proactiveSkillResponseWrapper;
    private final AbuseApplier abuseApplier;
    private final WizardService wizardService;
    private final ProactiveSkillExitSuggest proactiveSkillExitSuggest;
    private final MetricRegistry externalMetricRegistry;
    private final RequestGeolocationResponseHandler requestGeolocationResponseHandler;
    private final ShowUserAgreementsHandler showUserAgreementsHandler;
    private final StartPurchaseResponseHandler startPurchaseResponseHandler;
    private final StartAccountLinkingResponseHandler startAccountLinkingResponseHandler;

    @Value("${abuseConfig.enabled}")
    private boolean abuseEnabled;

    @SuppressWarnings("ParameterNumber")
    OrdinaryResponseHandler(
            CardConverter cardConverter,
            DialogovoDirectiveFactory directiveFactory,
            ProactiveSkillResponseWrapper proactiveSkillResponseWrapper,
            AbuseApplier abuseApplier,
            WizardService wizardService,
            ProactiveSkillExitSuggest proactiveSkillExitSuggest,
            @Qualifier("externalMetricRegistry") MetricRegistry externalMetricRegistry,
            RequestGeolocationResponseHandler requestGeolocationResponseHandler,
            ShowUserAgreementsHandler showUserAgreementsHandler,
            StartPurchaseResponseHandler startPurchaseResponseHandler,
            StartAccountLinkingResponseHandler startAccountLinkingResponseHandler) {
        this.cardConverter = cardConverter;
        this.directiveFactory = directiveFactory;
        this.proactiveSkillResponseWrapper = proactiveSkillResponseWrapper;
        this.abuseApplier = abuseApplier;
        this.wizardService = wizardService;
        this.proactiveSkillExitSuggest = proactiveSkillExitSuggest;
        this.externalMetricRegistry = externalMetricRegistry;
        this.requestGeolocationResponseHandler = requestGeolocationResponseHandler;
        this.showUserAgreementsHandler = showUserAgreementsHandler;
        this.startPurchaseResponseHandler = startPurchaseResponseHandler;
        this.startAccountLinkingResponseHandler = startAccountLinkingResponseHandler;
    }

    @Override
    public SkillProcessResult.Builder handleResponse(
            SkillProcessResult.Builder skillProcessResultBuilder,
            SkillProcessRequest req,
            Context context,
            RequestEnrichmentData enrichmentData,
            WebhookResponse webhookResponse
    ) {
        var skill = req.getSkill();
        var response = webhookResponse.getResponse().get();

        boolean shouldPostProcessWithWizard = req.getClientInfo().isYaSmartDevice();
        CompletableFuture<WizardResponse> wizardResponseFuture = shouldPostProcessWithWizard
                ? wizardService.requestWizardAsync(Optional.ofNullable(response.getText()), Optional.empty())
                : CompletableFuture.completedFuture(WizardResponse.EMPTY);

        if (abuseEnabled && !req.hasExperiment(Experiments.DONT_APPLY_ABUSE)) {
            abuseApplier.apply(webhookResponse);
        }

        var skillReplyWizardResponse = wizardResponseFuture.join();
        if (context.getSource() == SourceType.USER) {
            externalMetricRegistry.rate("skill_fallback",
                    SolomonHelper.getExternalSolomonLabels(skill, context.getSource())).inc();
        }
        if (context.getSource() == SourceType.USER && req.getClientInfo().isYaSmartDevice()) {
            handleProactiveExitSuggest(skillProcessResultBuilder, req, context, enrichmentData, webhookResponse,
                    skillReplyWizardResponse);
        }

        // TTS string with <speaker voice=""> tag
        var voice = response.getTts() != null && !response.getTts().isEmpty()
                ? response.getTts()
                : response.getText();
        // TTS string without <speaker voice=""> tag
        String rawTts = voice;


        // TODO: test when skill responses with should_listen=false
        skillProcessResultBuilder.getLayout().shouldListen(response.getShouldListen().orElse(true));

        var text = response.getText();
        // https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/external_skill/parser1x.cpp#L436
        if (text != null) {
            text = response.getText().replace("\\n", "\n");
        }

        val textCardBuilder = TextCard.builder();

        if (isPrependActivationMessage(req, context, response)) {
            proactiveSkillResponseWrapper.prependActivationMessage(
                    new TextWithTts(skill.getName(), skill.getNameTts()),
                    text,
                    textCardBuilder,
                    skillProcessResultBuilder
            );
            text = StringUtils.capitalize(text);
            rawTts = StringUtils.capitalize(rawTts);
        }

        if (text != null) {
            textCardBuilder.appendText(text, "\n");
        }
        skillProcessResultBuilder.appendTts(rawTts, skill.getVoice(), "\n")
                .endSession(response.isEndSession())
                // meaningful only for floyd skill
                .floydExitNode(response.isEndSession() ? response.getFloydExitNode() : null);

        SkillProcessResultButtons processResultButtons = handleButtons(response);
        skillProcessResultBuilder.getLayout().setSuggests(processResultButtons.suggest);
        textCardBuilder.setButtons(processResultButtons.buttons);

        if (response.getCard().isPresent()) {
            Optional<DivBody> divBody = cardConverter.convert(skill.getId(), response.getCard().get(),
                    processResultButtons.additionalButtons);
            if (divBody.isPresent()) {
                skillProcessResultBuilder.getLayout().divCard(divBody.get());
            } else {
                logger.error("Failed to convert skill card to div: {}", response.getCard().get());
            }
        }

        if (!textCardBuilder.isEmpty() &&
                (!skillProcessResultBuilder.getLayout().hasDivCards() ||
                        req.getClientInfo().isYaSmartDevice())) {
            skillProcessResultBuilder.getLayout().textCard(textCardBuilder.build());
        }

        if (req.getSkill().hasUserFeatureFlag(UserFeatureFlag.AUDIO_PLAYER)) {
            skillProcessResultBuilder.audioPlayerAction(response.getDirectives()
                    .flatMap(Directives::getAudioPlayer));
        }

        if ((req.getSkill().hasFeatureFlag(SkillFeatureFlag.MORDOVIA) ||
                req.getSkill().hasUserFeatureFlag(UserFeatureFlag.MORDOVIA))
                && req.getClientInfo().isSupportModrovia()) {
            skillProcessResultBuilder.canvasShow(response.getDirectives().flatMap(Directives::getCanvasShow)
                    .or(response::getCanvasShow));
            skillProcessResultBuilder.canvasCommand(response.getDirectives().flatMap(Directives::getCanvasCommand)
                    .or(response::getCanvasCommand));
        }

        // handle directives that can be shown with skill response
        if (response.getDirectives().flatMap(Directives::getRequestGeolocation).isPresent()) {
            requestGeolocationResponseHandler.handleResponse(
                    skillProcessResultBuilder,
                    req,
                    context,
                    enrichmentData,
                    webhookResponse
            );
        } else if (response.getDirectives().flatMap(Directives::getShowUserAgreements).isPresent()) {
            showUserAgreementsHandler.handleResponse(
                    skillProcessResultBuilder,
                    req,
                    context,
                    enrichmentData,
                    webhookResponse
            );
        } else if (isStartPurchaseResponse(req, response)) {
            startPurchaseResponseHandler.handleResponse(
                    skillProcessResultBuilder,
                    req,
                    context,
                    enrichmentData,
                    webhookResponse
            );
        } else if (webhookResponse.isStartAccountLinkingResponse()) {
            startAccountLinkingResponseHandler.handleResponse(
                    skillProcessResultBuilder,
                    req,
                    context,
                    enrichmentData,
                    webhookResponse
            );
        }

        return skillProcessResultBuilder;
    }

    private void handleProactiveExitSuggest(
            SkillProcessResult.Builder skillProcessResultBuilder,
            SkillProcessRequest req,
            Context context,
            RequestEnrichmentData enrichmentData,
            WebhookResponse webhookResponse,
            WizardResponse skillReplyWizardResponse
    ) {
        Session.ProactiveSkillExitState proactiveExitSuggestState = req.getSession()
                .map(Session::getProactiveSkillExitState)
                .orElseGet(Session.ProactiveSkillExitState::createEmpty);
        long messageId = req.getSession().isPresent() ? req.getSession().get().getMessageId() + 1 : 1;
        var proactiveSkillExitSuggestResult = proactiveSkillExitSuggest.apply(
                req,
                webhookResponse,
                enrichmentData.getWizardResponse(),
                skillReplyWizardResponse,
                proactiveExitSuggestState,
                messageId,
                req.getExperiments()
        );
        skillProcessResultBuilder.proactiveExitSuggestResult(Optional.of(proactiveSkillExitSuggestResult));

        if (proactiveSkillExitSuggestResult.isSuggestedExit()) {
            context.getAnalytics().addObject(new SuggestedSkillExitInfoObject(
                            req.getSkillId(),
                            proactiveSkillExitSuggestResult.getContainsWeakDeactivateForm(),
                            proactiveSkillExitSuggestResult.getDoNotUnderstandCounter()
                    )
            );
        }
    }

    private boolean isStartPurchaseResponse(SkillProcessRequest req, Response response) {
        return req.getSkill().hasUserFeatureFlag(UserFeatureFlag.SKILL_PURCHASE)
                && response.getDirectives()
                .flatMap(Directives::getStartPurchase)
                .isPresent();
    }

    private boolean isPrependActivationMessage(SkillProcessRequest req, Context context, Response response) {
        boolean activationWithoutCommand = req.getOriginalUtterance().isPresent() &&
                req.getOriginalUtterance().get().isEmpty();
        boolean payloadActivation = !Strings.isNullOrEmpty(req.getButtonPayload());
        return context.getSource() == SourceType.USER &&
                req.getClientInfo().isYaSmartDevice() &&
                req.getSession().isEmpty() &&
                !isLastMessage(response) &&
                (activationWithoutCommand || payloadActivation);
    }

    private boolean isLastMessage(Response response) {
        return response.isEndSession() && !hasAudioPlayStartDirective(response);
    }

    private boolean hasAudioPlayStartDirective(Response response) {
        return response.getDirectives().flatMap(Directives::getAudioPlayer)
                .map(AudioPlayerAction::getAction)
                .filter(audioPlayerActionType -> audioPlayerActionType == AudioPlayerActionType.PLAY)
                .isPresent();
    }

    private SkillProcessResultButtons handleButtons(Response response) {
        var suggest = new ArrayList<Button>();
        var buttons = new ArrayList<Button>();
        var additionalButtons = new ArrayList<Button>();

        boolean cardIsAbsent = response.getCard().isEmpty();

        if (response.getButtons().isPresent()) {
            for (var btn : response.getButtons().get()) {
                var url = btn.getUrl();
                var title = Optional.of(btn.getTitle());
                var payload = Optional.ofNullable(btn.getPayload());

                var directives = directiveFactory.createButtonDirectives(title, url, payload);

                var button = Button.withUrlAndPayload(btn.getTitle(), url, payload, btn.getHide(), directives);

                if (btn.getHide()) {
                    suggest.add(button);
                } else {
                    if (cardIsAbsent) {
                        // Кнопки в карточках могут быть только с параметром `hide=true`
                        // Мы можем показать саджесты даже с карточкой, но не кнопки
                        // В реализации басса `hide=false` мы не получаем ошибку и просто игнорим
                        buttons.add(button);
                    } else {
                        additionalButtons.add(button);
                    }
                }
            }
        }
        return new SkillProcessResultButtons(suggest, buttons, additionalButtons);
    }

    private static class SkillProcessResultButtons {
        private final List<Button> suggest;
        private final List<Button> buttons;
        private final List<Button> additionalButtons;

        SkillProcessResultButtons(List<Button> suggest, List<Button> buttons, List<Button> additionalButtons) {
            this.suggest = suggest;
            this.buttons = buttons;
            this.additionalButtons = additionalButtons;
        }
    }
}
