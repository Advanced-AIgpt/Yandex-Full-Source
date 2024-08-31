package ru.yandex.alice.paskill.dialogovo.processor;

import java.util.List;
import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.AliceHandledException;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.directive.StartMusicRecognizerDirective;
import ru.yandex.alice.kronstadt.core.layout.TextCard;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ActivateSkillProduct;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Directives;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.OpenYandexAuthCommand;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.RequestEnrichmentData;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static ru.yandex.alice.paskill.dialogovo.domain.Experiments.USE_SKILL_PURCHASE_ANY_DEVICE;

@Component
class SkillProductActivationResponseHandler implements WebhookResponseHandler {
    private static final Logger logger = LogManager.getLogger();

    private static final String PLEASE_START_MUSIC_KEY = "skill_product.activation.handle_directive.please_start_music";
    private static final String USER_SHOULD_BE_LOG_IN_KEY
            = "skill_product.activation.handle_directive.user_should_be_log_in";
    private static final String PAUSE_AFTER_TTS = "sil<[1500]>";

    private final RequestContext requestContext;
    private final MetricRegistry metricRegistry;
    private final Phrases phrases;
    private final String yandexAuthUri;

    SkillProductActivationResponseHandler(
            RequestContext requestContext,
            MetricRegistry metricRegistry,
            Phrases phrases,
            @Value("${yandexAuthUri}") String yandexAuthUri
    ) {
        this.requestContext = requestContext;
        this.metricRegistry = metricRegistry;
        this.phrases = phrases;
        this.yandexAuthUri = yandexAuthUri;
    }

    @Override
    public SkillProcessResult.Builder handleResponse(
            SkillProcessResult.Builder builder,
            SkillProcessRequest request,
            Context context,
            RequestEnrichmentData requestEnrichment,
            WebhookResponse response
    ) {
        var skillProductActivationType = getActivateSkillProductDirective(request, response).getActivationType();
        String skillId = request.getSkillId();

        switch (skillProductActivationType) {
            case MUSIC:
                logger.info("Skill request music activation. SkillId: " + skillId);
                checkClientSupportMusicActivation(request);
                if (requestContext.getCurrentUserId() == null) {
                    return requestUserLogin(builder, request, skillId);
                }
                return handleMusicActivateType(builder, request, response);
            default:
                throw AliceHandledException.from(
                        "Unexpected ActivationType: " + skillProductActivationType,
                        ErrorAnalyticsInfoAction.ACTIVATE_SKILL_PRODUCT,
                        phrases.getRandom("skill_product.activation.handle_directive.failed", request.getRandom()),
                        true);
        }
    }

    private SkillProcessResult.Builder requestUserLogin(
            SkillProcessResult.Builder builder,
            SkillProcessRequest request,
            String skillId
    ) {
        logger.warn("Skill " + skillId + " requested product activation, but user didn't login.");
        metricRegistry.rate("skill.music.product.activation.handle.directive.rate",
                Labels.of("skill_id", skillId, "result", "user_not_login")).inc();

        String userShouldBeLogInText = phrases.getRandom(USER_SHOULD_BE_LOG_IN_KEY, request.getRandom());
        builder.getLayout().textCard(userShouldBeLogInText);
        return builder
                .setTts(userShouldBeLogInText, null)
                .openYandexAuthCommand(Optional.of(new OpenYandexAuthCommand(yandexAuthUri)));
    }

    private SkillProcessResult.Builder handleMusicActivateType(
            SkillProcessResult.Builder builder,
            SkillProcessRequest request,
            WebhookResponse response
    ) {
        builder.startMusicRecognizerDirective(Optional.of(StartMusicRecognizerDirective.INSTANCE));

        String defaultActivationPhrase = phrases.getRandom(PLEASE_START_MUSIC_KEY, request.getRandom());
        String text = response.getResponse().map(Response::getText).orElse(defaultActivationPhrase);
        String tts = response.getResponse().map(Response::getTts).orElse(defaultActivationPhrase) + PAUSE_AFTER_TTS;
        builder.getLayout()
                .cards(List.of(new TextCard(text)))
                // Should be false because we want music recognizer.
                .shouldListen(false);
        builder.setTts(tts, null);

        String skillId = request.getSkillId();
        logger.info("Successfully handled music activation. SkillId: " + skillId);
        metricRegistry.rate("skill.music.product.activation.handle.directive.rate",
                Labels.of("skill_id", skillId, "result", "success")).inc();

        return builder;
    }

    private ActivateSkillProduct getActivateSkillProductDirective(
            SkillProcessRequest request,
            WebhookResponse response
    ) {
        return response.getResponse()
                .flatMap(Response::getDirectives)
                .flatMap(Directives::getActivateSkillProduct)
                .orElseThrow(() -> AliceHandledException.from(
                        "Unable to handle ActivateSkillProduct directive",
                        ErrorAnalyticsInfoAction.ACTIVATE_SKILL_PRODUCT,
                        phrases.getRandom("skill_product.activation.handle_directive.failed", request.getRandom()),
                        true));
    }

    private void checkClientSupportMusicActivation(SkillProcessRequest req) {
        if (!req.getClientInfo().isSupportMusicActivation() && !req.hasExperiment(USE_SKILL_PURCHASE_ANY_DEVICE)) {
            throw AliceHandledException.from(
                    "Surface is not supported",
                    ErrorAnalyticsInfoAction.REQUEST_FAILURE,
                    "Навык требует музыкальную активацию, но к сожалению, на этом устройстве такая " +
                            " возможность не поддерживается",
                    true);
        }
    }
}
