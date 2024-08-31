package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Random;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Strings;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Service;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.ActionSpace;
import ru.yandex.alice.kronstadt.core.AliceHandledException;
import ru.yandex.alice.kronstadt.core.DivRenderData;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.StackEngine;
import ru.yandex.alice.kronstadt.core.StackEngineAction;
import ru.yandex.alice.kronstadt.core.StackEngineKt;
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;
import ru.yandex.alice.kronstadt.core.directive.CloseDialogDirective;
import ru.yandex.alice.kronstadt.core.directive.EndDialogSessionDirective;
import ru.yandex.alice.kronstadt.core.directive.HideViewDirective;
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective;
import ru.yandex.alice.kronstadt.core.directive.ModroviaCallbackDirective;
import ru.yandex.alice.kronstadt.core.directive.MordoviaCommandDirective;
import ru.yandex.alice.kronstadt.core.directive.MordoviaShowDirective;
import ru.yandex.alice.kronstadt.core.directive.OpenUriDirective;
import ru.yandex.alice.kronstadt.core.directive.ShowViewDirective;
import ru.yandex.alice.kronstadt.core.domain.AudioPlayerActivityState;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.Voice;
import ru.yandex.alice.kronstadt.core.input.Input;
import ru.yandex.alice.kronstadt.core.input.UtteranceInput;
import ru.yandex.alice.kronstadt.core.layout.Button;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.layout.TextCard;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.config.UserSkillProductConfig;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.domain.UserFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerEventRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerState;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillProductActivationEvent;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.AudioPlayerAction;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.AudioPlayerActionType;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.Play;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.Stop;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.processor.DialogovoDirectiveFactory;
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.processor.discovery.activation.converter.ProvidableToSkillFrameService;
import ru.yandex.alice.paskill.dialogovo.providers.UserProvider;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.GeolocationSharingState;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.ProductActivationState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillLayoutsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.VoiceButtonFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.FeedbackRequestNlg;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.AudioPlayerControlAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.AudioStreamAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.DeactivateSkillAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.FeedbackRequestedEvent;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.GeolocationSharingActions;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.OpenYandexAuthAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SkillProductActivationActions;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.UserAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.AccountLinkingCompleteDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.ButtonPressDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.GetNextAudioPlayerItemCallbackDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.NewSessionDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.UserAgreementsAcceptedDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.UserAgreementsRejectedDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.renderer.SkillAudioRendererService;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.renderer.SkillDialogRendererService;
import ru.yandex.alice.paskill.dialogovo.service.api.ApiException;
import ru.yandex.alice.paskill.dialogovo.service.api.ApiService;
import ru.yandex.alice.paskill.dialogovo.service.billing.BillingService;
import ru.yandex.alice.paskill.dialogovo.service.billing.BillingServiceException;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookException;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationResult;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static ru.yandex.alice.kronstadt.core.MegaMindRequest.DeviceState.PLAYER_STATE_META_SKILL_ID_FIELD;
import static ru.yandex.alice.kronstadt.core.directive.TtsPlayPlaceholderDirective.TTS_PLAY_PLACEHOLDER_DIRECTIVE_DIALOG;
import static ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillProductActivationEvent.EventType;
import static ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters.HAS_MORDOVIA_VIEW;
import static ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticFrames.ALICE_EXTERNAL_SKILL_ACTIVATE;
import static ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticFrames.EXTERNAL_SKILL_FIXED_ACTIVATE;
import static ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.DialogovoUtils.isInSkillTab;
import static ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.SpaceActionUtils.DIALOG_ACTION_SPACE_ID;
import static ru.yandex.alice.paskill.dialogovo.utils.WebhookErrorUtils.exceededErrorsLimit;
import static ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationResult.ActivationResult.ALREADY_ACTIVATED;
import static ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationResult.ActivationResult.SUCCESS;

@Service
public class MegaMindRequestSkillApplier {
    private static final Logger logger = LogManager.getLogger();

    private static final AnalyticsInfoAction MORDOVIA_COMMAND_ACTION =
            new AnalyticsInfoAction("mordovia_command",
                    "mordovia_command",
                    "Команда для реакции интерфейса");
    private static final AnalyticsInfoAction MORDOVIA_SHOW_ACTION =
            new AnalyticsInfoAction("mordovia_show",
                    "mordovia_show",
                    "Команда для запуска интерфейса");

    private final SkillProvider skillProvider;
    private final UserProvider userProvider;
    private final RequestContext requestContext;
    private final SkillRequestProcessor processor;
    private final VoiceButtonFactory voiceButtonFactory;
    private final FeedbackRequestNlg feedbackRequestNlg;
    private final DialogovoDirectiveFactory directiveFactory;
    private final SkillLayoutsFactory skillLayoutsFactory;
    private final SuggestButtonFactory suggestButtonFactory;
    private final ApiService apiService;
    private final BillingService billingService;
    private final Phrases phrases;
    private final UserSkillProductConfig skillProductConfig;
    private final MetricRegistry metricRegistry;
    private final Rate applyFailureCounter;
    private final SkillDialogRendererService skillDialogRendererService;
    private final SkillAudioRendererService skillAudioRendererService;
    private final ProvidableToSkillFrameService providableToSkillFrameService;

    @SuppressWarnings("ParameterNumber")
    public MegaMindRequestSkillApplier(
            SkillProvider skillProvider,
            UserProvider userProvider,
            RequestContext requestContext,
            SkillRequestProcessor processor,
            VoiceButtonFactory voiceButtonFactory,
            FeedbackRequestNlg feedbackRequestNlg,
            DialogovoDirectiveFactory directiveFactory,
            SkillLayoutsFactory skillLayoutsFactory,
            SuggestButtonFactory suggestButtonFactory,
            ApiService apiService,
            MetricRegistry metricRegistry,
            BillingService billingService,
            Phrases phrases,
            UserSkillProductConfig productconfig,
            SkillDialogRendererService skillDialogRendererService,
            SkillAudioRendererService skillAudioRendererService,
            ProvidableToSkillFrameService providableToSkillFrameService
    ) {
        this.skillProvider = skillProvider;
        this.userProvider = userProvider;
        this.requestContext = requestContext;
        this.processor = processor;
        this.voiceButtonFactory = voiceButtonFactory;
        this.feedbackRequestNlg = feedbackRequestNlg;
        this.directiveFactory = directiveFactory;
        this.skillLayoutsFactory = skillLayoutsFactory;
        this.suggestButtonFactory = suggestButtonFactory;
        this.apiService = apiService;
        this.billingService = billingService;
        this.phrases = phrases;
        this.skillProductConfig = productconfig;
        this.metricRegistry = metricRegistry;
        this.applyFailureCounter = metricRegistry.rate("business_failure",
                Labels.of("aspect", "megamind",
                        "process", "apply",
                        "reason", "webhook_station_exception"));
        this.skillDialogRendererService = skillDialogRendererService;
        this.skillAudioRendererService = skillAudioRendererService;
        this.providableToSkillFrameService = providableToSkillFrameService;
    }


    @SuppressWarnings("MethodLength")
    public ScenarioResponseBody<DialogovoState> processApply(
            MegaMindRequest<DialogovoState> request,
            Context ctx,
            RequestSkillApplyArguments applyArgs
    ) {
        return processApply(request, ctx, applyArgs, Optional.empty());
    }

    @SuppressWarnings("MethodLength")
    public ScenarioResponseBody<DialogovoState> processApply(
            MegaMindRequest<DialogovoState> request,
            Context ctx,
            RequestSkillApplyArguments applyArgs,
            Optional<RequestSkillEvents> events
    ) {

        var clientInfo = request.getClientInfo();
        var skillId = applyArgs.getSkillId();
        var skill = skillProvider.getSkill(skillId)
                .orElseThrow(() -> new AliceHandledException("Skill not found " + skillId,
                        ErrorAnalyticsInfoAction.REQUEST_FAILURE, "Навык не найден", "Навык не найден"));

        ctx.getAnalytics().addObject(new SkillAnalyticsInfoObject(skill));

        Optional<ActivationSourceType> activationSourceTypeO = Optional.ofNullable(applyArgs.getActivationSourceType());

        final Optional<Map<String, Object>> viewState =
                (skill.hasFeatureFlag(SkillFeatureFlag.MORDOVIA) || skill.hasUserFeatureFlag(UserFeatureFlag.MORDOVIA))
                        && request.getClientInfo().isQuasar() ?
                        Optional.of(request.getMordoviaState() != null ? request.getMordoviaState() :
                                Collections.emptyMap()) :
                        Optional.empty();
        var skillRequest = SkillProcessRequest.builder()
                .skill(skill)
                .locationInfo(request.getLocationInfoO())
                .experiments(request.getExperiments())
                .random(request.getRandom())
                .clientInfo(clientInfo)
                .viewState(viewState)
                .testRequest(request.isTest())
                .requestTime(request.getServerTime())
                .activationSourceType(activationSourceTypeO.orElse(ActivationSourceType.UNDETECTED))
                .mementoData(request.getMementoData())
                .providableToSkillFrames(
                        providableToSkillFrameService.getProvidableToSkillFramesFromMMRequest(request)
                );

        setAudioPlayerState(skillRequest, request, skill);

        if (SkillFilters.SKILL_CAN_PROCESS_FILTRATION_LEVEL_VERBOSE.test(request, skill)) {
            logger.info(
                    "Add filtration level {} to request of skill with id {}",
                    request.getFiltrationLevel(),
                    skill.getId()
            );
            skillRequest.filtrationLevel(request.getFiltrationLevel());
        }

        var input = request.getInput();
        @Nullable
        var callbackDirective = input instanceof Input.Callback ? ((Input.Callback) input).getDirective() : null;
        Optional<Session> session = resolveSessionOnSkillRequest(request, skill, applyArgs);

        if (callbackDirective instanceof NewSessionDirective ||
                (isImmediateActivateApply(clientInfo) && session.isEmpty())) {

            // TODO: PASKILLS-4362 разобраться, почему блин вообще это должно передаватьс через аргументы!!!
            //  раньше было просто "". возможно это как-то связано с суффиксными запросами...
            if (!Strings.isNullOrEmpty(applyArgs.getPayload())) {
                skillRequest.isButtonPress(true);
                skillRequest.buttonPayload(applyArgs.getPayload());
            } else {
                skillRequest.normalizedUtterance(applyArgs.getRequestO().orElse(""));
                skillRequest.originalUtterance(applyArgs.getOriginalUtteranceO().orElse(""));
                skillRequest.session(Optional.empty());
            }
        } else {
            skillRequest.session(session);

            if (input instanceof UtteranceInput) {
                var textInput = (UtteranceInput) input;
                skillRequest.normalizedUtterance(textInput.getNormalizedUtterance());
                skillRequest.originalUtterance(textInput.getOriginalUtterance());
            } else if (input instanceof Input.Callback) {

                if (callbackDirective instanceof AccountLinkingCompleteDirective) {
                    skillRequest.accountLinkingCompleteEvent(true);
                } else if (callbackDirective instanceof ModroviaCallbackDirective) {
                    var mordoviaCallback = (ModroviaCallbackDirective) callbackDirective;
                    skillRequest.mordoviaCallbackEvent(true);
                    skillRequest.mordoviaCallbackDirective(mordoviaCallback);
                } else if (callbackDirective instanceof GetNextAudioPlayerItemCallbackDirective) {
                    skillRequest.audioPlayerEvent(
                            new AudioPlayerEventRequest(InputType.AUDIO_PLAYER_PLAYBACK_NEARLY_FINISHED));
                } else if (callbackDirective instanceof ButtonPressDirective) {
                    var inputPayload = (ButtonPressDirective) callbackDirective;
                    // В реализации басса если мы нажимаем на кнопку, и в ней нет payload
                    // то мы ходим в вебхук с type=SimpleUtterance а не type=ButtonPressed
                    skillRequest.normalizedUtterance(inputPayload.getText());
                    if (inputPayload.getPayload() == null) {
                        skillRequest.originalUtterance(inputPayload.getText());
                    } else {
                        skillRequest.isButtonPress(true);
                        skillRequest.buttonPayload(inputPayload.getPayload());
                    }
                } else if (callbackDirective instanceof UserAgreementsAcceptedDirective) {
                    skillRequest.userAgreementsAcceptedEvent(
                            new UserAgreementsAcceptedEvent(
                                    ((UserAgreementsAcceptedDirective) callbackDirective).getUserAgreementIds()
                            ));
                } else if (callbackDirective instanceof UserAgreementsRejectedDirective) {
                    skillRequest.userAgreementsRejectedEvent(UserAgreementsRejectedEvent.INSTANCE);
                } else {
                    logger.error("Received unknown callback directive: {}", callbackDirective);
                }
            } else if (input instanceof Input.Music) {
                var activationEvent =
                        getMusicSkillProductActivationEvent(skill, (Input.Music) input, request.getRandom());
                skillRequest.skillProductActivationEvent(activationEvent);
                ctx.getAnalytics().addAction(activationEvent.getType().convertToActionEvent());
            }
        }

        var geolocationSharingAllowedEventO = events.flatMap(RequestSkillEvents::getGeolocationSharingAllowedEvent);
        geolocationSharingAllowedEventO.ifPresent(skillRequest::geolocationSharingAllowedEvent);

        Boolean needToShareGeolocation = request.getStateO()
                .flatMap(DialogovoState::getGeolocationSharingStateO)
                .map(GeolocationSharingState::getAllowedSharingUntilTime)
                .map(untilTime -> request.getServerTime().isBefore(untilTime))
                .orElse(geolocationSharingAllowedEventO.isPresent());
        skillRequest.needToShareGeolocation(needToShareGeolocation);

        var geolocationSharingRejectedEventO = events.flatMap(RequestSkillEvents::getGeolocationSharingRejectedEvent);
        geolocationSharingRejectedEventO.ifPresent(skillRequest::geolocationSharingRejectedEvent);

        events
                .flatMap(RequestSkillEvents::getPurchaseCompleteEvent)
                .ifPresent(skillRequest::skillPurchaseCompleteEvent);

        skillRequest.voiceSession(request.isVoiceSession());

        ctx.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_REQUEST);

        Optional<Session> newSession;
        List<DivRenderData> renderData = new ArrayList<>();

        try {
            SkillProcessRequest skillProcessRequest = skillRequest.build();

            var hashedUserId = userProvider.getApplicationId(skill, skillProcessRequest.getApplicationId());
            var persistentUserId = userProvider.getPersistentUserId(skill, requestContext.getCurrentUserId());
            ctx.getAnalytics().addObject(new UserAnalyticsInfoObject(hashedUserId, persistentUserId));

            var processResult = processor.process(ctx, skillProcessRequest);
            var outputSpeech = request.isVoiceSession() ? processResult.getVoice() : null;

            newSession = processResult.getSession();

            var directives = new ArrayList<MegaMindDirective>();
            Optional<DialogovoState> state;
            final boolean expectRequests;
            Map<String, ActionRef> actions = new HashMap<>();
            Map<String, ActionSpace> actionSpaces = new HashMap<>();
            List<String> additionalTexts = new ArrayList<>();

            boolean requestFeedbackOnEndSession = shallCollectFeedback(request, skill, newSession);

            boolean hasAudioPlayDirective = processResult.getAudioPlayerAction().isPresent() &&
                    processResult.getAudioPlayerAction().get().getAction() == AudioPlayerActionType.PLAY;

            boolean playerNowPlaying = request.isSkillPlayerOwner(skillId) &&
                    request.inAudioPlayerStates(AudioPlayerActivityState.PLAYING);

            if (processResult.getEndSession()) {
                // audio_player can play out of session - so do not reset state if has audio_play directive
                // but close MM session with expectRequests
                if (hasAudioPlayDirective || playerNowPlaying) {
                    state = newSession.map(sess -> DialogovoState.createSkillState(skillId,
                            sess,
                            System.currentTimeMillis(),
                            Optional.empty(),
                            true,
                            request.getStateO().flatMap(DialogovoState::getGeolocationSharingStateO)));
                    expectRequests = false;
                } else {
                    if (requestFeedbackOnEndSession) {
                        actions.putAll(voiceButtonFactory.createFeedbackButtons(skillProcessRequest.getSkillId()));
                        var feedbackRequest = feedbackRequestNlg.render(request.getRandom(), skill);
                        additionalTexts.add(feedbackRequest.getText());
                        processResult = processResult.withSuggestButtons(feedbackRequestNlg.getSuggests(skill));
                        state = newSession.map(x -> DialogovoState.createSkillState(skillId,
                                x.getEnded(),
                                request.getServerTime().toEpochMilli(),
                                Optional.of(skill.getId()),
                                applyArgs.getResumeSessionAfterPlayerStopRequests(),
                                request.getStateO().flatMap(DialogovoState::getGeolocationSharingStateO)));
                        if (outputSpeech != null) {
                            outputSpeech = outputSpeech.length() > 0 &&
                                    Character.isLetter(outputSpeech.charAt(outputSpeech.length() - 1))
                                    ? outputSpeech + ". " + feedbackRequest.getTts(Voice.SHITOVA_US)
                                    : outputSpeech + " " + feedbackRequest.getTts(Voice.SHITOVA_US);
                        }
                        ctx.getAnalytics().addEvent(FeedbackRequestedEvent.INSTANCE);
                        expectRequests = true;
                    } else {
                        directives.add(new EndDialogSessionDirective(skillId));
                        if (isInSkillTab(request, skillId) && (session.isEmpty() || !session.get().isNew())) {
                            directives.add(new CloseDialogDirective(skillId));
                        }
                        state = Optional.empty();
                        expectRequests = false;
                    }
                }
            } else {
                state = newSession.map(sess -> DialogovoState.createSkillState(skillId,
                        sess,
                        System.currentTimeMillis(),
                        Optional.empty(),
                        applyArgs.getResumeSessionAfterPlayerStopRequests(),
                        request.getStateO().flatMap(DialogovoState::getGeolocationSharingStateO)));
                expectRequests = true;
            }

            if (processResult.getStartMusicRecognizerDirective().isPresent()) {
                directives.add(processResult.getStartMusicRecognizerDirective().get());
                var activationState = new ProductActivationState(0, DialogovoState.ActivationType.MUSIC);
                state = state.map(s -> s.withProductActivationState(activationState));
                ctx.getAnalytics().addAction(SkillProductActivationActions.ACTIVATION_BY_MUSIC_START_ACTION);
            }

            if (processResult.getOpenYandexAuthCommand().isPresent()) {
                String yandexAuthUri = processResult.getOpenYandexAuthCommand().get().yandexAuthUri();
                directives.add(new OpenUriDirective(yandexAuthUri));
                ctx.getAnalytics().addAction(OpenYandexAuthAction.INSTANCE);
            }

            if (geolocationSharingAllowedEventO.isPresent()) {
                var untilTime = geolocationSharingAllowedEventO.get().getUntilTime();
                var geolocationState = new GeolocationSharingState(false, untilTime);
                state = state.map(s -> s.withGeolocationSharingState(geolocationState));
                ctx.getAnalytics().addAction(GeolocationSharingActions.GEOLOCATION_SHARING_ALLOWED);
            } else if (geolocationSharingRejectedEventO.isPresent()) {
                state = state.map(s -> s.withGeolocationSharingState(null));
                ctx.getAnalytics().addAction(GeolocationSharingActions.GEOLOCATION_SHARING_REJECTED);
            } else if (processResult.getRequestedGeolocation().orElse(false)) {
                var geolocationState = new GeolocationSharingState(true, null);
                state = state.map(s -> s.withGeolocationSharingState(geolocationState));
            }

            // не могу понять, почему добавляем update_dialog_info для просто new_dialog_session
            // ведь в ПП на запрос активации возвращается open_dialog, сменяется вкладка и уже во вкладке отправляется
            // new_dialog_session. почему нужно что-то апдейтить???
            if (callbackDirective instanceof NewSessionDirective ||
                    (request.getClientInfo().isElariWatch() &&
                            skillProcessRequest.getSession().map(Session::isNew).orElse(true))) {
                directives.add(directiveFactory.updateDialogInfoDirective(skill));
            }

            if (request.getClientInfo().isYaSmartDevice()) {
                pausePlayersIfActive(directives, request, skill);
            }

            if (processResult.getCanvasShow().isPresent()) {

                var cmd = processResult.getCanvasShow().get();
                directives.add(new MordoviaShowDirective(cmd.getUrl(), cmd.isFullScreen(),
                        "dialogovo:" + skill.getId()));
                ctx.getAnalytics().addAction(MORDOVIA_SHOW_ACTION);

            } else if (processResult.getCanvasCommand().isPresent()) {

                var cmd = processResult.getCanvasCommand().get();
                directives.add(new MordoviaCommandDirective(cmd.getCommand(), cmd.getMeta(),
                        "dialogovo:" + skill.getId()));
                ctx.getAnalytics().addAction(MORDOVIA_COMMAND_ACTION);
            }

            if (clientInfo.isCentaur()) { // todo: replace "isCentaur" with new supported feature
                var skillRenderData = skillDialogRendererService.getSkillDialogRenderData(request, processResult);
                if (skillRenderData != null) {
                    ShowViewDirective.InactivityTimeout timeout = ShowViewDirective.InactivityTimeout.INFINITY;
                    if (processResult.getEndSession() && !hasAudioPlayDirective) {
                        timeout = ShowViewDirective.InactivityTimeout.SHORT;
                    } else {
                        actionSpaces.put(DIALOG_ACTION_SPACE_ID, SpaceActionUtils.getModalSkillSessionSpaceAction());
                    }
                    directives.add(new ShowViewDirective(
                            skillRenderData.getCardId(),
                            DIALOG_ACTION_SPACE_ID,
                            true,
                            timeout));
                    renderData.add(skillRenderData);
                }
                directives.add(TTS_PLAY_PLACEHOLDER_DIRECTIVE_DIALOG);
            }

            StackEngine stackEngine = null;
            if (processResult.getAudioPlayerAction().isPresent()) {
                AudioPlayerAction audioPlayerAction = processResult.getAudioPlayerAction().get();

                if (audioPlayerAction.getAction() == AudioPlayerActionType.PLAY) {
                    if (request.hasExperiment(Experiments.SKILL_AUDIO_RENDER_DATA)) {
                        var audioRenderData = skillAudioRendererService
                                .getAudioPlayRenderData((Play) audioPlayerAction);
                        directives.add(new ShowViewDirective(
                                audioRenderData.getCardId(),
                                DIALOG_ACTION_SPACE_ID,
                                true,
                                ShowViewDirective.Layer.Companion.getCONTENT()));
                        //TODO: убрать, когда научимся "сдувать" верхние слои, если пользователь явно захотел открыть
                        // слой ниже
                        directives.add(new HideViewDirective(HideViewDirective.Layer.Companion.getDIALOG()));
                        renderData.add(audioRenderData);
                    }

                    directives.add(directiveFactory.audioPlayDirective((Play) audioPlayerAction, skill.getId(),
                            skill.getName()));
                    // add get_next after play directive - for device next track warm up
                    GetNextAudioPlayerItemCallbackDirective getNextCallbackDirective =
                            new GetNextAudioPlayerItemCallbackDirective(skillId);
                    if (!request.hasExperiment(Experiments.PLAYER_WITH_STACK_ENGINE_DISABLED)) {
                        if (request.getRequestSource() != MegaMindRequest.RequestSource.GET_NEXT) {
                            // start new stack session if current not exists
                            stackEngine = new StackEngine(
                                    StackEngineAction.NewSession.INSTANCE,
                                    new StackEngineAction.ResetAdd(
                                            StackEngineKt.toStackEngineEffect(getNextCallbackDirective)
                                    )
                            );
                        } else {
                            stackEngine = StackEngineKt.toStackEngine(getNextCallbackDirective);
                        }
                    } else {
                        directives.add(getNextCallbackDirective);
                    }
                } else if (audioPlayerAction.getAction() == AudioPlayerActionType.STOP) {
                    directives.addAll(directiveFactory.audioStopSequence());
                }

                addPlayerActionAnalyticsInfo(ctx, audioPlayerAction, Optional.ofNullable(request.getDeviceState()));
            }

            var shouldListen = !processResult.getEndSession() &&
                    request.isVoiceSession() &&
                    processResult.getLayout().getShouldListen() &&
                    (processResult.getAudioPlayerAction().isEmpty() ||
                            processResult.getAudioPlayerAction().get() instanceof Stop);

            Layout layout = makeLayout(
                    request,
                    processResult,
                    additionalTexts,
                    outputSpeech,
                    new ArrayList<>(directives),
                    shouldListen);
            actions.putAll(processResult.getNluHints());
            return new ScenarioResponseBody<>(
                    layout,
                    state,
                    ctx.getAnalytics().toAnalyticsInfo(),
                    expectRequests,
                    actions,
                    processResult.getServerDirectives(),
                    stackEngine,
                    actionSpaces,
                    renderData
            );
        } catch (WebhookException e) {
            applyFailureCounter.inc();

            // закрываем только на станции
            if (clientInfo.isYaSmartDevice()) {
                if (!exceededErrorsLimit(e.getResponse(), session)) {
                    return buildScenarioResponseWithException(request, ctx, applyArgs, session, e);
                }
                var deactivateLayout = skillLayoutsFactory.createDeactivateSkillLayout(
                        skill.getId(),
                        e.getResponseBody().getAliceText(),
                        clientInfo,
                        HAS_MORDOVIA_VIEW.test(request, skill),
                        request.hasActiveAudioPlayer(),
                        false);
                return new ScenarioResponseBody<>(deactivateLayout,
                        ctx.getAnalytics().addAction(DeactivateSkillAction.INSTANCE).toAnalyticsInfo(), false);
            }
            throw e;
        }
    }

    private ScenarioResponseBody<DialogovoState> buildScenarioResponseWithException(
            MegaMindRequest<DialogovoState> request,
            Context ctx,
            RequestSkillApplyArguments applyArgs,
            Optional<Session> session,
            WebhookException e) {

        Session newSession = session
                .map(sess -> sess.getEventId().isPresent() ? sess : sess.getNext())
                .orElseGet(() -> Session
                        .create(applyArgs.getActivationSourceType() != null
                                        ? applyArgs.getActivationSourceType()
                                        : ActivationSourceType.UNDETECTED,
                                request.getServerTime()))
                .withIncFailCounter();
        DialogovoState state = DialogovoState.createSkillState(
                applyArgs.getSkillId(),
                newSession,
                System.currentTimeMillis(),
                Optional.empty(),
                applyArgs.getResumeSessionAfterPlayerStopRequests(),
                request.getStateO().flatMap(DialogovoState::getGeolocationSharingStateO));

        Layout layout = Layout.builder()
                .outputSpeech(e.getResponseBody().getAliceSpeech())
                .textCard(e.getResponseBody().getAliceText())
                .shouldListen(false)
                .suggest(suggestButtonFactory.getStopSuggest(request.getAssistantName()))
                .build();

        ctx.getAnalytics().addAction(e.getResponseBody().getAction());
        return new ScenarioResponseBody<>(layout,
                state,
                ctx.getAnalytics().toAnalyticsInfo(),
                true
        );
    }

    private Optional<Session> resolveSessionOnSkillRequest(MegaMindRequest<DialogovoState> request, SkillInfo skill,
                                                           RequestSkillApplyArguments applyArgs) {
        Optional<Session> sessionO = request.getStateO().flatMap(DialogovoState::getSessionO);

        if (request.getClientInfo().isCentaur() && (request.hasAnySemanticFrame(ALICE_EXTERNAL_SKILL_ACTIVATE)
                || request.hasAnySemanticFrame(EXTERNAL_SKILL_FIXED_ACTIVATE))) {
            return Optional.empty();
        }

        if (sessionO.isPresent()) {
            DialogovoState dialogovoState = request.getStateO().get();

            // Session state can proceed out of skill - so reset session on skill change
            if (dialogovoState.getCurrentSkillIdO().isPresent()
                    && !dialogovoState.getCurrentSkillIdO().get().equals(skill.getId())) {
                return Optional.empty();
            }

            boolean isSessionInBackground =
                    request.getStateO().map(DialogovoState::isSessionInBackground).orElse(false);
            // activate on same skill on background session
            if (isSessionInBackground && applyArgs.isActivation()) {
                return Optional.empty();
            }
        }

        return sessionO;
    }

    private void pausePlayersIfActive(ArrayList<MegaMindDirective> directives, MegaMindRequest<DialogovoState> request,
                                      SkillInfo skill) {
        if (request.isMusicPlayerCurrentlyPlaying() || request.isRadioPlayerCurrentlyPlaying()) {
            directives.addAll(directiveFactory.musicPlayerStopSequence(false));
        } else if (request.isAudioPlayerCurrentlyPlaying()) {
            if (!request.isSkillPlayerOwner(skill.getId())) {
                directives.addAll(directiveFactory.audioStopSequence());
            }
        }
    }

    private void addPlayerActionAnalyticsInfo(Context ctx, AudioPlayerAction audioPlayerAction,
                                              Optional<MegaMindRequest.DeviceState> deviceStateO) {
        if (audioPlayerAction.getAction() == AudioPlayerActionType.PLAY) {
            ctx.getAnalytics().addAction(new AudioPlayerControlAction(
                    AudioPlayerControlAction.AudioPlayerControlActionType.PLAY));
            Play play = (Play) audioPlayerAction;
            ctx.getAnalytics().addObject(new AudioStreamAnalyticsInfoObject(
                    play.getAudioItem().getStream().getToken(),
                    play.getAudioItem().getStream().getOffsetMs()));
        } else if (audioPlayerAction.getAction() == AudioPlayerActionType.STOP) {
            ctx.getAnalytics().addAction(new AudioPlayerControlAction(
                    AudioPlayerControlAction.AudioPlayerControlActionType.STOP));
            //try get current stream from state
            Optional<MegaMindRequest.DeviceState.AudioPlayer> audioPlayerStateO =
                    deviceStateO.map(deviceState -> deviceState.getAudioPlayerState());
            if (audioPlayerStateO.isPresent()) {
                MegaMindRequest.DeviceState.AudioPlayer audioPlayerState = audioPlayerStateO.get();
                ctx.getAnalytics().addObject(new AudioStreamAnalyticsInfoObject(
                        audioPlayerState.getToken(),
                        audioPlayerState.getOffsetMs()));
            }
        }
    }

    // Add player state for every request to skill if it was last to modify audio player state
    private void setAudioPlayerState(SkillProcessRequest.SkillProcessRequestBuilder skillProcessRequestBuilder,
                                     MegaMindRequest<DialogovoState> request, SkillInfo skill) {
        Optional<MegaMindRequest.DeviceState.AudioPlayer> audioPlayerStateO =
                request.getDeviceStateO().map(MegaMindRequest.DeviceState::getAudioPlayerState);
        if (audioPlayerStateO.isPresent()) {
            MegaMindRequest.DeviceState.AudioPlayer audioPlayerState = audioPlayerStateO.get();

            if (audioPlayerState.getMeta().getOrDefault(PLAYER_STATE_META_SKILL_ID_FIELD, "")
                    .equals(skill.getId())) {
                skillProcessRequestBuilder.audioPlayerState(new AudioPlayerState(
                        audioPlayerState.getToken(),
                        audioPlayerState.getOffsetMs(),
                        audioPlayerState.getActivityState()));
            }
        }
    }

    @Nullable
    private SkillProductActivationEvent getMusicSkillProductActivationEvent(
            SkillInfo skillInfo,
            Input.Music musicInput,
            Random random
    ) {
        String skillId = skillInfo.getId();
        var musicIdToTokenCode = skillProductConfig.getMusicIdToTokenCode();
        var musicUrlToTokenCode = skillProductConfig.getMusicUrlToTokenCode();

        if (musicInput.getMusicResult() == Input.Music.MusicResult.NOT_MUSIC) {
            metricRegistry.rate("skill.music.product.activation.event.rate",
                    Labels.of("skill_id", skillId, "result", "error_not_playing")).inc();
            logger.warn("Not music playing during music activation. SkillId: " + skillId);
            return new SkillProductActivationEvent(EventType.MUSIC_NOT_PLAYING, Optional.empty());
        }

        String tokenCode;
        var musicData = musicInput.getMusicData();
        if (musicIdToTokenCode.containsKey(musicData.getMusicId())) {
            tokenCode = musicIdToTokenCode.get(musicData.getMusicId());
        } else if (musicUrlToTokenCode.containsKey(musicData.getUrl())) {
            tokenCode = musicUrlToTokenCode.get(musicData.getUrl());
        } else {
            metricRegistry.rate("skill.music.product.activation.event.rate",
                    Labels.of("skill_id", skillId, "result", "error_not_recognized")).inc();
            logger.warn("Not recognized during music activation. SkillId: " + skillId);
            return new SkillProductActivationEvent(EventType.MUSIC_NOT_RECOGNIZED, Optional.empty());
        }

        try {
            UserSkillProductActivationResult result = billingService.activateUserSkillProduct(skillInfo, tokenCode);
            if (result.getActivationResult() == SUCCESS || result.getActivationResult() == ALREADY_ACTIVATED) {
                if (result.getActivationResult() == SUCCESS) {
                    metricRegistry.rate("skill.music.product.activation.event.rate",
                            Labels.of("skill_id", skillId, "result", "success")).inc();
                    logger.info("Success music activation. SkillId: " + skillId + ", token: " + tokenCode);
                } else {
                    metricRegistry.rate("skill.music.product.activation.event.rate",
                            Labels.of("skill_id", skillId, "result", "already_exist")).inc();
                    logger.info("Music activation of existed product. SkillId: " + skillId + ", token: " + tokenCode);
                }
                return new SkillProductActivationEvent(EventType.SUCCESS, Optional.of(result.getSkillProduct()));
            } else {
                return null;
            }
        } catch (BillingServiceException e) {
            throw AliceHandledException.from(
                    "Billing didn't activate product. tokenCode = " + tokenCode + ", skillId = " + skillId,
                    ErrorAnalyticsInfoAction.MUSIC_SKILL_PRODUCT_ACTIVATION_FAILURE,
                    phrases.getRandom("skill_product.activation.handle_directive.internal_error", random),
                    true);
        }
    }

    private Layout makeLayout(
            MegaMindRequest<DialogovoState> request,
            SkillProcessResult processResult,
            List<String> additionalTexts,
            String outputSpeech,
            List<MegaMindDirective> directives,
            boolean shouldListen
    ) {
        final Layout layout;
        List<Button> suggests;
        ClientInfo clientInfo = request.getClientInfo();
        Layout originalLayout = processResult.getLayout();

        if (clientInfo.isYaSmartDevice() || clientInfo.isNavigatorOrMaps() || clientInfo.isYaAuto()) {
            suggests = new ArrayList<>();
            if (!processResult.getEndSession()) {
                suggests.add(suggestButtonFactory.getStopSuggest(request.getAssistantName()));
                suggests.addAll(processResult.getLayout().getSuggests());
            }
        } else {
            suggests = processResult.getLayout().getSuggests();
        }
        if (!originalLayout.getDivCards().isEmpty() && !clientInfo.isYaSmartDevice()) {
            layout = Layout.builder()
                    .outputSpeech(outputSpeech)
                    .shouldListen(shouldListen)
                    .cards(originalLayout.getCards())
                    .suggests(suggests)
                    .directives(directives)
                    .contentProperties(originalLayout.getContentProperties())
                    .build();
        } else if (!originalLayout.getTextCards().isEmpty()) {
            layout = Layout.builder()
                    .addCards(originalLayout.getTextCards())
                    // TODO: remove, add TextCards to original layout instead
                    .addCards(
                            additionalTexts.stream().map(TextCard::new).collect(Collectors.toList())
                    )
                    .outputSpeech(outputSpeech)
                    .suggests(suggests)
                    .directives(directives)
                    .shouldListen(shouldListen)
                    .contentProperties(originalLayout.getContentProperties())
                    .build();
        } else {
            layout = Layout.builder()
                    .shouldListen(false)
                    .directives(directives)
                    .contentProperties(originalLayout.getContentProperties())
                    .build();

        }
        return layout;
    }

    private boolean shallCollectFeedback(
            MegaMindRequest<DialogovoState> request, SkillInfo skill, Optional<Session> session
    ) {
        if (!request.hasExperiment(Experiments.REQUEST_FEEDBACK_ON_END_SESSION)) {
            return false;
        }
        ClientInfo clientInfo = request.getClientInfo();
        if (skill.isHideInStore()) {
            logger.debug("Not requesting feedback: private skill");
            return false;
        }
        if (!clientInfo.isSearchApp() && !clientInfo.isYaBrowser()) {
            logger.debug("Not requesting feedback: unsupported surface");
            // disable feedback requests for surfaces that do not have clickable buttons
            return false;
        }
        if (session.isEmpty() || session.get().isNew()) {
            logger.debug("Not requesting feedback: session is empty");
            // do not ask for feedback on first request in session
            return false;
        }
        var userTicket = requestContext.getCurrentUserTicket();
        if (userTicket == null) {
            logger.debug("Not requesting feedback: user is not defined");
            return false;
        }
        try {
            var mark = apiService.getFeedbackMarkO(userTicket, skill.getId(), request.getClientInfo().getUuid());
            logger.debug("Existing mark {} for skill {}", mark.orElse(null), skill.getId());
            return mark.isEmpty() || request.hasExperiment(Experiments.REQUEST_FEEDBACK_ALWAYS);
        } catch (ApiException e) {
            logger.error("Api threw error, could not get mark for skill " + skill.getId(), e);
            return false;
        }
    }

    private boolean isImmediateActivateApply(ClientInfo clientInfo) {
        return clientInfo.isYaSmartDevice()
                || clientInfo.isNavigatorOrMaps()
                || clientInfo.isYaAuto()
                || clientInfo.isElariWatch();
    }
}
