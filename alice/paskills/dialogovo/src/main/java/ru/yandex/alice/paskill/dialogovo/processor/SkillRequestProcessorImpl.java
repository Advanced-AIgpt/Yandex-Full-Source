package ru.yandex.alice.paskill.dialogovo.processor;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.Future;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.base.Strings;
import com.google.protobuf.util.Timestamps;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject;
import ru.yandex.alice.kronstadt.core.domain.AudioPlayerActivityState;
import ru.yandex.alice.kronstadt.core.domain.FiltrationLevel;
import ru.yandex.alice.megamind.protos.common.FrameProto;
import ru.yandex.alice.memento.proto.MementoApiProto;
import ru.yandex.alice.memento.proto.UserConfigsProto.TExternalSkillUserAgreements.TExternalSkillUserAgreement;
import ru.yandex.alice.paskill.dialogovo.controller.SkillPurchaseCallbackRequest;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.domain.UserAgreement;
import ru.yandex.alice.paskill.dialogovo.domain.UserFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.external.ButtonPressedWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Intent;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Nlu;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.AccountLinkingCompleteWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.ButtonPressedRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.CanvasCallback;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.FilteringMode;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.Markup;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.Meta;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.MordoviaCallbackWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.SimpleUtteranceRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.TeasersWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.UserAgreementsAcceptedRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.UserAgreementsAcceptedWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.UserAgreementsRejectedRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.UserAgreementsRejectedWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.UtteranceWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookRequestBase;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookSession;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookState;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WidgetGalleryWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerEventWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.geolocation.GeolocationSharingAllowedRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.geolocation.GeolocationSharingAllowedWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.geolocation.GeolocationSharingRejectedRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.geolocation.GeolocationSharingRejectedWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.show.ShowPullWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillProductActivatedRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillProductActivatedWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillProductActivationEvent;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillProductActivationFailedRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillProductActivationFailedWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillPurchaseCompleteRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillPurchaseCompleteWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillPurchaseConfirmationRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillPurchaseConfirmationWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Directives;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ResponseAnalytics;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.megamind.AliceHandledWithDevConsoleMessageException;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskill.dialogovo.processor.discovery.activation.converter.ProvidableToSkillFrameService;
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.EnrichmentDataProvider;
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.RequestEnrichmentData;
import ru.yandex.alice.paskill.dialogovo.providers.UserProvider;
import ru.yandex.alice.paskill.dialogovo.providers.skill.UserAgreementProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SessionAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.payment.PurchaseCompleteEvent;
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;
import ru.yandex.alice.paskill.dialogovo.service.logging.SkillRequestLogger;
import ru.yandex.alice.paskill.dialogovo.service.state.SkillState;
import ru.yandex.alice.paskill.dialogovo.service.state.SkillStateDao;
import ru.yandex.alice.paskill.dialogovo.service.state.SkillStateId;
import ru.yandex.alice.paskill.dialogovo.utils.ClientInfoUtils;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.webhook.client.SolomonHelper;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookClient;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookException;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestParams;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestResult;
import ru.yandex.alice.paskills.common.billing.model.api.SkillProductItem;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookSession.UserSkillProducts;
import static ru.yandex.alice.paskill.dialogovo.utils.CryptoUtils.signData;
import static ru.yandex.alice.paskill.dialogovo.utils.WebhookErrorUtils.exceededErrorsLimit;

@Component
public class SkillRequestProcessorImpl implements SkillRequestProcessor {

    private static final DiagnosticInfoLogger DIAGNOSTIC_INFO_LOGGER = new DiagnosticInfoLogger();

    private static final Logger logger = LogManager.getLogger();

    private static final List<AudioPlayerActivityState> CONTINUE_PLAY_STATES = List.of(
            AudioPlayerActivityState.PAUSED,
            AudioPlayerActivityState.STOPPED
    );
    private static final String YANDEX_PLAYER_CONTINUE = "YANDEX.PLAYER.CONTINUE";
    //TODO: интент для литреса, убрать если навык перейдет на "YANDEX.PLAYER.CONTINUE"
    private static final String LITRES_PLAYER_CONTINUE = "go_on";

    private final WebhookClient webhookClient;
    private final SurfaceChecker surfaceChecker;
    private final UserProvider userProvider;
    private final StartAccountLinkingResponseHandler startAccountLinkingResponseHandler;
    private final ShowPullResponseHandler showPullResponseHandler;
    private final WidgetGalleryResponseHandler widgetGalleryResponseHandler;
    private final TeasersResponseHandler teasersResponseHandler;
    private final SkillProductActivationResponseHandler skillProductActivationResponseHandler;
    private final OrdinaryResponseHandler ordinaryResponseHandler;
    private final RequestContext requestContext;
    private final AppMetricaEventSender appMetricaEventSender;
    private final SkillStateDao skillStateDao;
    private final EnrichmentDataProvider enrichmentDataProvider;
    private final SkillRequestLogger skillRequestLogger;
    private final MetricRegistry metricRegistry;
    private final DialogovoInstrumentedExecutorService asyncSkillRequestServiceExecutor;
    private final UserAgreementProvider userAgreementProvider;
    private final DialogovoRequestContext dialogovoRequestContext;
    private final ObjectMapper objectMapper;
    private final ProvidableToSkillFrameService providableToSkillFrameService;

    @Value("${enableLogSkillRequests}")
    private boolean enableLogSkillRequests;

    @SuppressWarnings("ParameterNumber")
    public SkillRequestProcessorImpl(
            WebhookClient webhookClient,
            SurfaceChecker surfaceChecker,
            UserProvider userProvider,
            StartAccountLinkingResponseHandler startAccountLinkingResponseHandler,
            ShowPullResponseHandler showPullResponseHandler,
            WidgetGalleryResponseHandler widgetGalleryResponseHandler,
            TeasersResponseHandler teasersResponseHandler,
            SkillProductActivationResponseHandler skillProductActivationResponseHandler,
            OrdinaryResponseHandler ordinaryResponseHandler,
            RequestContext requestContext,
            AppMetricaEventSender appMetricaEventSender,
            SkillStateDao skillStateDao,
            EnrichmentDataProvider enrichmentDataProvider,
            SkillRequestLogger skillRequestLogger,
            @Qualifier("externalMetricRegistry") MetricRegistry metricRegistry,
            @Qualifier("asyncSkillRequestServiceExecutor") DialogovoInstrumentedExecutorService
                    asyncSkillRequestServiceExecutor,
            UserAgreementProvider userAgreementProvider,
            DialogovoRequestContext dialogovoRequestContext,
            ObjectMapper objectMapper,
            ProvidableToSkillFrameService providableToSkillFrameService) {
        this.webhookClient = webhookClient;
        this.surfaceChecker = surfaceChecker;
        this.userProvider = userProvider;
        this.startAccountLinkingResponseHandler = startAccountLinkingResponseHandler;
        this.showPullResponseHandler = showPullResponseHandler;
        this.widgetGalleryResponseHandler = widgetGalleryResponseHandler;
        this.teasersResponseHandler = teasersResponseHandler;
        this.skillProductActivationResponseHandler = skillProductActivationResponseHandler;
        this.ordinaryResponseHandler = ordinaryResponseHandler;
        this.requestContext = requestContext;
        this.appMetricaEventSender = appMetricaEventSender;
        this.skillStateDao = skillStateDao;
        this.enrichmentDataProvider = enrichmentDataProvider;
        this.skillRequestLogger = skillRequestLogger;
        this.metricRegistry = metricRegistry;
        this.asyncSkillRequestServiceExecutor = asyncSkillRequestServiceExecutor;
        this.userAgreementProvider = userAgreementProvider;
        this.dialogovoRequestContext = dialogovoRequestContext;
        this.objectMapper = objectMapper;
        this.providableToSkillFrameService = providableToSkillFrameService;
    }

    @Override
    public Future<SkillProcessResult> processAsync(Context context, SkillProcessRequest request) {
        return asyncSkillRequestServiceExecutor.supplyAsyncInstrumented(
                () -> process(context, request, true),
                Duration.ofMillis(3500));
    }

    @Override
    public SkillProcessResult process(Context context, SkillProcessRequest request) {
        return process(context, request, false);
    }

    private SkillProcessResult process(Context context, SkillProcessRequest request, boolean async) {
        var skill = request.getSkill();
        if (!skill.isOnAir()) {
            throw new AliceHandledWithDevConsoleMessageException("Skill not published",
                    ErrorAnalyticsInfoAction.REQUEST_FAILURE,
                    "Навык выключен", "Навык выключен");
        }

        if (!surfaceChecker.isSkillSupported(request.getClientInfo(), skill.getSurfaces())) {
            throw new AliceHandledWithDevConsoleMessageException("Surface not supported",
                    ErrorAnalyticsInfoAction.UNSUPPORTED_SURFACE_FAILURE, "Данная поверхность не поддерживается",
                    "Я это, конечно, умею. Но в другом приложении.", "Я это конечно умею. Но в друг+ом приложении");
        }


        var newSession = request.getSession()
                .map(session -> session.getEventId().isPresent() ? session : session.getNext())
                .orElseGet(() -> Session.create(request.getActivationSourceType(), request.getRequestTime()));

        context.getAnalytics().addObject(new SessionAnalyticsInfoObject(newSession.getSessionId(),
                newSession.getActivationSourceType()));

        var enrichmentData = enrichmentDataProvider.enrichRequest(request, newSession, context.getSource());
        WebhookRequestParams params = makeParams(request, enrichmentData, newSession, context.getSource());

        WebhookRequestResult requestResult = webhookClient.callWebhook(context, params);

        DIAGNOSTIC_INFO_LOGGER.log(
                Optional.ofNullable(requestContext.getRequestId()),
                skill.getId(),
                requestResult.getErrors(),
                context.getSource(),
                newSession.getSessionId(),
                request.getClientInfo().getUuid(),
                newSession.getMessageId()
        );

        // debug for https://st.yandex-team.ru/PASKILLS-7468
        logRequestSessionInfo(request, newSession);

        if (isSaveSkillLogsPersistent(context, request, requestResult)) {
            skillRequestLogger.log(params, requestResult);
        }

        if (requestResult.hasErrors()) {
            if (!request.getClientInfo().isYaSmartDevice()
                    || exceededErrorsLimit(requestResult, Optional.of(newSession))) {
                appMetricaEventSender.sendEndSessionEvents(
                        context.getSource(),
                        skill.getId(),
                        skill.getLook(),
                        skill.getEncryptedAppMetricaApiKey(),
                        request.getClientInfo(),
                        request.getLocationInfo(),
                        request.getRequestTime(),
                        Optional.of(newSession),
                        request.isTestRequest(),
                        skill.hasFeatureFlag(SkillFeatureFlag.APPMETRICA_EVENTS_TIMESTAMP_COUNTER)
                );
            }
            throw new WebhookException(params, requestResult, requestResult.getCause().orElse(null));
        }

        long newEventCount = appMetricaEventSender.sendClientEvents(
                context.getSource(),
                skill.getId(),
                skill.getLook(),
                skill.getEncryptedAppMetricaApiKey(),
                request.getClientInfo(),
                request.getLocationInfo(),
                request.getRequestTime(),
                Optional.of(newSession),
                params.getBody().getMetricaEvent(),
                Optional.of(requestResult),
                request.isTestRequest(),
                skill.hasFeatureFlag(SkillFeatureFlag.APPMETRICA_EVENTS_TIMESTAMP_COUNTER));

        var resultBuilder = SkillProcessResult.builder(requestResult, skill, params.getBody())
                .skillResponseText(requestResult.getResponse()
                        .flatMap(WebhookResponse::getResponse)
                        .map(Response::getText)
                        .orElse(null))
                .session(newSession.withAppmetricaEventCounter(newEventCount));

        var responseO = requestResult.getResponse();
        if (responseO.isEmpty()) {
            return resultBuilder.build();
        }

        WebhookResponse response = responseO.get();
        resultBuilder.sensitiveData(response.getAnalytics()
                .map(ResponseAnalytics::getSensitiveData)
                .orElse(false));

        response.getAnalytics()
                .map(ResponseAnalytics::getScene)
                .ifPresent(sceneAnalytics ->
                        context.getAnalytics().addObject(new AnalyticsInfoObject(
                                        sceneAnalytics.toString(),
                                        "scene",
                                        "ID сцены: " + sceneAnalytics.getId()
                                )
                        )
                );

        WebhookResponseHandler responseHandler = chooseResponseHandler(response, request);

        if (skill.getUseStateStorage()) {
            // don't store session for async calls (callbacks), store only app and user states
            boolean storeSession = !async;
            updateStates(skill, params.getBody(), response, request.getRequestTime(),
                    request.getClientInfo().getUuid(), storeSession);
        }

        return responseHandler.handleResponse(resultBuilder, request, context, enrichmentData, response).build();
    }

    private boolean isSaveSkillLogsPersistent(Context context, SkillProcessRequest request,
                                              WebhookRequestResult requestResult) {
        return context.getSource() == SourceType.USER &&
                !requestResult.getResponse()
                        .flatMap(WebhookResponse::getAnalytics)
                        .map(ResponseAnalytics::getSensitiveData)
                        .orElse(false) &&
                (enableLogSkillRequests ||
                        request.getSkill().hasFeatureFlag(SkillFeatureFlag.SAVE_LOGS) ||
                        request.hasExperiment(Experiments.SAVE_SKILL_LOGS));
    }

    private WebhookResponseHandler chooseResponseHandler(
            WebhookResponse response, SkillProcessRequest request
    ) {
        // some directives are handled in OrdinaryResponseHandler along with skill response
        if (response.isStartAccountLinkingResponse() && response.getResponse().isEmpty()) {
            return startAccountLinkingResponseHandler;
        } else if (isProductActivationResponse(response)
                && request.getSkill().hasUserFeatureFlag(UserFeatureFlag.USER_SKILL_PRODUCT)) {
            return skillProductActivationResponseHandler;
        } else if (request.getShowPullRequest().isPresent()) {
            return showPullResponseHandler;
        } else if (request.getWidgetGalleryRequest().isPresent()) {
            return widgetGalleryResponseHandler;
        } else if (request.getTeasersRequest().isPresent()) {
            return teasersResponseHandler;
        } else {
            return ordinaryResponseHandler;
        }
    }

    private boolean isProductActivationResponse(WebhookResponse response) {
        return response.getResponse()
                .flatMap(Response::getDirectives)
                .flatMap(Directives::getActivateSkillProduct)
                .isPresent();
    }

    private void updateStates(SkillInfo skill,
                              WebhookRequestBase request,
                              WebhookResponse response,
                              Instant requestTime,
                              String applicationId,
                              boolean storeSession) {
        var newSession = request.getSession();
        var userId = requestContext.getCurrentUserId();

        Map<String, Object> stateBaseOnPreviousSessionState =
                request.getState() != null && doNotResetState(skill, response) ?
                        request.getState().getSession().orElse(Collections.emptyMap()) :
                        Collections.emptyMap();
        Map<String, Object> sessionState = response.getSessionState().orElse(stateBaseOnPreviousSessionState);

        Map<String, Object> userStateIncrement = userId != null ?
                response.getUserStateUpdate().orElse(Collections.emptyMap()) : Collections.emptyMap();

        skillStateDao.storeSessionAndUserAndApplicationState(
                new SkillStateId(skill.getId(), userId, newSession.getSessionId(), applicationId),
                requestTime,
                storeSession ? sessionState : null,
                userStateIncrement,
                response.getApplicationState().orElse(null)
        );
    }

    private boolean doNotResetState(SkillInfo skill, WebhookResponse response) {
        return skill.hasFeatureFlag(SkillFeatureFlag.DO_NOT_RESET_STATE_ON_KEEP_STATE) && response.isKeepState();
    }

    WebhookRequestParams makeParams(
            SkillProcessRequest req,
            RequestEnrichmentData enrichmentData,
            Session session,
            SourceType source
    ) {
        var skill = req.getSkill();
        var userId = requestContext.getCurrentUserId();

        // introduce user-generated flag on skill
        // PASKILLS-4419: Добавить на флаг опцию хранения стейта
        boolean processState = skill.getUseStateStorage() && source.isByUser();

        Optional<String> token = enrichmentData.getSocialToken();

        var nlu = enrichmentData.getNlu();
        Map<String, Intent> intents = new LinkedHashMap<>(
                enrichmentData.getWizardResponse().getSkillIntents(skill, req)
        );
        for (FrameProto.TTypedSemanticFrame providableFrame : req.getProvidableToSkillFrames()) {
            Intent intent = providableToSkillFrameService.toIntent(providableFrame, skill);
            if (intent != null) {
                intents.put(intent.getName(), intent);
            }
        }

        if (source == SourceType.USER && skill.getGrammarsBase64().isPresent() && intents.isEmpty()) {
            metricRegistry.rate("empty_intents", SolomonHelper.getExternalSolomonLabels(skill, source));
        }
        nlu = nlu.withIntents(intents);

        Optional<Map<String, Object>> sessionState;
        Optional<Map<String, Object>> userState;
        Optional<Map<String, Object>> applicationState;
        if (processState) {
            // even for empty state we should pass it to skill if developer opt-in-ed storage feature
            sessionState = enrichmentData.getSkillState()
                    .map(SkillState::getSessionState)
                    .or(() -> Optional.of(Collections.emptyMap()));
            userState = userId != null
                    ? enrichmentData.getSkillState()
                    .map(SkillState::getUserState)
                    .or(() -> Optional.of(Collections.emptyMap()))
                    : Optional.empty();
            applicationState = enrichmentData.getSkillState()
                    .map(SkillState::getApplicationState)
                    .or(() -> Optional.of(Collections.emptyMap()));
        } else {
            sessionState = Optional.empty();
            userState = Optional.empty();
            applicationState = Optional.empty();
        }
        var state = WebhookState.create(req.getViewState(), sessionState,
                userState, applicationState, req.getAudioPlayerState());
        var userSkillProduct = enrichmentData.getUserSkillProductsResult();
        Optional<WebhookSession.Location> location = req.isNeedToShareGeolocation() ?
                WebhookSession.Location.from(req.getLocationInfo()) :
                Optional.empty();

        var body = getRequestBody(req, session, nlu, state, token, userSkillProduct, location);

        return WebhookRequestParams.builder(skill, source, body)
                .experiments(req.getExperiments())
                .authToken(token.orElse(null))
                .internalRequestId(requestContext.getRequestId())
                .internalDeviceId(req.getClientInfo().getDeviceId())
                .internalUuid(req.getClientInfo().getUuid())
                .build();
    }

    WebhookRequestBase getRequestBody(SkillProcessRequest req,
                                      Session session,
                                      Nlu nlu,
                                      WebhookState state,
                                      Optional<String> token,
                                      List<UserSkillProducts> userSkillProducts,
                                      Optional<WebhookSession.Location> location) {

        var skill = req.getSkill();
        var applicationId = req.getApplicationId();
        var hashedUserId = userProvider.getApplicationId(skill, applicationId);
        if (Strings.isNullOrEmpty(hashedUserId)) {
            logger.warn("application_id is null or empty in skill request for uuid: {} and client app_id: {}",
                    req.getClientInfo().getUuid(), req.getClientInfo().getAppId());
        }
        var persistentUserId = userProvider.getPersistentUserId(skill, requestContext.getCurrentUserId());

        Optional<Boolean> agreedToUserAgreements = agreedToUserAgreements(req);

        var webhookSession = WebhookSession.builder()
                .sessionId(session.getSessionId())
                .skillId(req.getSkillId())
                .messageId(session.getMessageId())
                .eventId(session.getEventId())
                .user(persistentUserId.map(userId -> new WebhookSession.User(
                        userId,
                        token,
                        userSkillProducts,
                        agreedToUserAgreements)))
                // context is set for floyd requests only
                .floydUser(Optional.ofNullable(dialogovoRequestContext.getFloydRequestContext())
                        .map(u -> new WebhookSession.FloydUser(u.getPuid(), u.getLogin(), u.getOperatorChatId())))
                .application(new WebhookSession.Application(hashedUserId))
                .location(location)
                .build();

        var meta = getMeta(req);

        if (req.isAccountLinkingCompleteEvent()) {
            return new AccountLinkingCompleteWebhookRequest(meta, webhookSession, state);
        } else if (req.getSkillPurchaseConfirmEvent().isPresent()) {
            SkillPurchaseCallbackRequest callbackRequest = req.getSkillPurchaseConfirmEvent().get();
            String signedData = String.format(
                    "purchase_request_id=%s&purchase_token=%s&order_id=%s&purchase_timestamp=%d",
                    callbackRequest.getPurchaseRequestId(),
                    callbackRequest.getPurchaseToken(),
                    callbackRequest.getPurchaseOfferUuid(),
                    callbackRequest.getPurchaseTimestamp()
            );
            if (skill.getPrivateKey() == null) {
                throw new SkillPrivateKeyAbsentException();
            }
            String privateKey = skill.getPrivateKey();
            var request = new SkillPurchaseConfirmationRequest(
                    callbackRequest.getPurchaseRequestId(),
                    callbackRequest.getPurchaseToken(),
                    callbackRequest.getPurchaseOfferUuid(),
                    callbackRequest.getPurchaseTimestamp(),
                    callbackRequest.getPurchasePayload(),
                    signedData,
                    signData(privateKey, signedData)
            );
            return new SkillPurchaseConfirmationWebhookRequest(meta, webhookSession, request, state);
        } else if (req.getSkillPurchaseCompleteEvent().isPresent()) {
            PurchaseCompleteEvent purchaseCompleteEvent = req.getSkillPurchaseCompleteEvent().get();
            var request = new SkillPurchaseCompleteRequest(
                    purchaseCompleteEvent.getPurchaseRequestId(),
                    purchaseCompleteEvent.getPurchaseOfferUuid(),
                    purchaseCompleteEvent.getPurchasePayload()
            );
            return new SkillPurchaseCompleteWebhookRequest(meta, webhookSession, request, state);
        } else if (req.getAudioPlayerEvent().isPresent()) {
            return new AudioPlayerEventWebhookRequest(meta, webhookSession, state, req.getAudioPlayerEvent().get(),
                    objectMapper);
        } else if (req.isMordoviaCallbackEvent()) {
            CanvasCallback callback = new CanvasCallback(req.getMordoviaCallbackDirective().getCommand(),
                    req.getMordoviaCallbackDirective().getMeta());
            return new MordoviaCallbackWebhookRequest(meta, webhookSession, state, callback);
        } else if (req.getSkillProductActivationEvent().isPresent()) {
            var activationEvent = req.getSkillProductActivationEvent().get();
            return getSkillProductActivationRequest(state, webhookSession, meta, activationEvent);
        } else if (req.getGeolocationSharingAllowEvent().isPresent()) {
            var request = new GeolocationSharingAllowedRequest();
            return new GeolocationSharingAllowedWebhookRequest(meta, webhookSession, request, state);
        } else if (req.getGeolocationSharingRejectedEvent().isPresent()) {
            var request = new GeolocationSharingRejectedRequest();
            return new GeolocationSharingRejectedWebhookRequest(meta, webhookSession, request, state);
        } else if (req.getUserAgreementsAcceptedEvent().isPresent()) {
            var request = new UserAgreementsAcceptedRequest();
            return new UserAgreementsAcceptedWebhookRequest(meta, webhookSession, request, state);
        } else if (req.getUserAgreementsRejectedEvent().isPresent()) {
            var request = new UserAgreementsRejectedRequest();
            return new UserAgreementsRejectedWebhookRequest(meta, webhookSession, request, state);
        } else if (req.getWidgetGalleryRequest().isPresent()) {
            return new WidgetGalleryWebhookRequest(meta, webhookSession, req.getWidgetGalleryRequest().get(), state);
        } else if (req.getTeasersRequest().isPresent()) {
            return new TeasersWebhookRequest(meta, webhookSession, req.getTeasersRequest().get(), state);
        } else {
            // TODO: implement dangerous context markup like in VINS/BASS (PASKILLS-4040)
            var markup = Markup.of(false);
            if (req.isButtonPress()) {
                var request = new ButtonPressedRequest(nlu, req.getButtonPayload());
                return new ButtonPressedWebhookRequest(meta, webhookSession, request, state);
            } else if (req.getShowPullRequest().isPresent()) {
                return new ShowPullWebhookRequest(meta, webhookSession, req.getShowPullRequest().get(), state);
            } else {
                if (nlu.getIntents().isEmpty() && req.getAudioPlayerState().isPresent() &&
                        CONTINUE_PLAY_STATES.contains(req.getAudioPlayerState().get().getAudioPlayerActivityState()) &&
                        req.hasExperiment(Experiments.SKILL_AUDIO_RENDER_DATA)) {
                    nlu = nlu.withIntents(Map.of(
                            YANDEX_PLAYER_CONTINUE, new Intent(YANDEX_PLAYER_CONTINUE),
                            LITRES_PLAYER_CONTINUE, new Intent(LITRES_PLAYER_CONTINUE)
                    ));
                }
                var request = new SimpleUtteranceRequest(
                        req.getNormalizedUtterance().orElse(null),
                        req.getOriginalUtterance().orElse(null),
                        nlu,
                        markup);
                return new UtteranceWebhookRequest(meta, webhookSession, request, state);
            }
        }
    }

    private WebhookRequestBase getSkillProductActivationRequest(
            WebhookState state,
            WebhookSession webhookSession,
            Meta meta,
            SkillProductActivationEvent activationEvent
    ) {
        if (activationEvent.getType() == SkillProductActivationEvent.EventType.SUCCESS) {
            SkillProductItem productItem = activationEvent.getSkillProductItem().get();
            var request = new SkillProductActivatedRequest(productItem.getUuid(), productItem.getName());
            return new SkillProductActivatedWebhookRequest(meta, webhookSession, request, state);
        } else {
            var request = new SkillProductActivationFailedRequest(activationEvent.getType().convertToError());
            return new SkillProductActivationFailedWebhookRequest(meta, webhookSession, request, state);
        }
    }

    private Meta getMeta(SkillProcessRequest req) {
        var clientInfo = req.getClientInfo();
        var skill = req.getSkill();

        boolean fromScheduler = ActivationSourceType.SCHEDULER.equals(req.getActivationSourceType());

        var interfaces = fromScheduler ? Meta.Interfaces.EMPTY_INSTANCE : Meta.Interfaces.of(
                clientInfo.isHasScreenBASSimpl(),
                clientInfo.isSupportsBilling(),
                clientInfo.isSupportsAccountLinking(),
                skill.hasUserFeatureFlag(UserFeatureFlag.AUDIO_PLAYER) && clientInfo.hasAudioPlayer(),
                skill.hasUserFeatureFlag(UserFeatureFlag.USER_SKILL_PRODUCT) && clientInfo.isSupportMusicActivation(),
                skill.hasUserFeatureFlag(UserFeatureFlag.GEOLOCATION_SHARING)
                        && clientInfo.isSupportGeolocationSharing(),
                skill.hasUserFeatureFlag(UserFeatureFlag.USER_AGREEMENTS) &&
                        clientInfo.isSupportUserAgreements());
        var builder = Meta.builder()
                .clientId(ClientInfoUtils.printId(clientInfo, req.getRequestTime()))
                .locale(clientInfo.getLang())
                .timezone(clientInfo.getTimezone())
                .interfaces(interfaces)
                .filteringMode(req.getFiltrationLevel().map(this::getContentRatingQuota).orElse(null));

        if (skill.isExposeInternalFlags()) {
            if (!req.getClientInfo().supportsDivCards()) {
                builder.flags(Optional.of(List.of("no_cards_support")));
            }

            builder.experiments(Optional.of(new ArrayList<>(req.getExperiments())));
        }

        if ((clientInfo.isQuasar()
                && skill.hasFeatureFlag(SkillFeatureFlag.SEND_DEVICE_ID))
                || req.hasExperiment(Experiments.SEND_DEVICE_ID)) {

            builder.deviceId(clientInfo.getDeviceIdO());
        }

        return builder.build();
    }

    private FilteringMode getContentRatingQuota(@Nonnull FiltrationLevel fl) {
        return switch (fl) {
            case NO_FILTER -> FilteringMode.NO_FILTER;
            case MODERATE -> FilteringMode.MODERATE;
            case FAMILY_SEARCH, SAFE -> FilteringMode.SAFE;
            default -> throw new IllegalStateException("Unexpected value: " + fl);
        };
    }

    private Optional<Boolean> agreedToUserAgreements(SkillProcessRequest request) {
        if (!request.getSkill().hasUserFeatureFlag(UserFeatureFlag.USER_AGREEMENTS)) {
            return Optional.empty();
        }
        UUID skillId = UUID.fromString(request.getSkill().getId());
        List<UserAgreement> userAgreements = request.getSkill().getDraft()
                ? userAgreementProvider.getDraftUserAgreements(skillId)
                : userAgreementProvider.getPublishedUserAgreements(skillId);
        Set<UUID> skillUserAgreements = userAgreements
                .stream()
                .map(UserAgreement::getId)
                .collect(Collectors.toSet());
        if (skillUserAgreements.isEmpty()) {
            return Optional.of(false);
        }
        Set<UUID> userAgreedToIds = savedAgreements(request.getMementoData().getUserConfigs())
                .getOrDefault(skillId, Collections.emptyList())
                .stream()
                .map(ExternalSkillUserAgreement::id)
                .collect(Collectors.toSet());
        return Optional.of(userAgreedToIds.containsAll(skillUserAgreements));
    }

    private Map<UUID, List<ExternalSkillUserAgreement>> savedAgreements(MementoApiProto.TUserConfigs userConfigs) {
        return userConfigs.getExternalSkillUserAgreements().getUserAgreementsMap().entrySet().stream()
                .collect(Collectors.toMap(
                                entry -> UUID.fromString(entry.getKey()),
                                entry -> entry.getValue().getUserAgreementsList()
                                        .stream()
                                        .map(ExternalSkillUserAgreement::fromProto)
                                        .toList()
                        )
                );
    }

    record ExternalSkillUserAgreement(
            UUID id,
            Instant agreedAt,
            String ip,
            String userAgent,
            String url
    ) {
        static ExternalSkillUserAgreement fromProto(TExternalSkillUserAgreement proto) {
            return new ExternalSkillUserAgreement(
                    UUID.fromString(proto.getUserAgreementId()),
                    Instant.ofEpochMilli(Timestamps.toMillis(proto.getAgreedAt())),
                    proto.getIp(),
                    proto.getUserAgent(),
                    proto.getUserAgreementLinks()
            );
        }
    }

    private void logRequestSessionInfo(SkillProcessRequest request, Session session) {
        try {
            logger.info(objectMapper.writeValueAsString(new RequestSessionInfo(
                    request.getSession().map(Session::getMessageId),
                    session.getMessageId(), session.getSessionId(),
                    session.getEventId(), session.getActivationSourceType(),
                    request.getRequestTime(), request.getClientInfo().getAppId(),
                    request.getClientInfo().getUuid(),
                    request.getClientInfo().getDeviceId())));
        } catch (JsonProcessingException e) {
            //
        }
    }

    private record RequestSessionInfo(
            Optional<Long> oldSessionMessageId,
            Long messageId,
            String sessionId,
            Optional<UUID> eventId,
            ActivationSourceType activationSourceType,
            Instant requestTime,
            String appId,
            String uuid,
            @Nullable String deviceId
    ) {
    }
}
