package ru.yandex.alice.paskill.dialogovo.processor;

import java.net.URI;
import java.util.List;
import java.util.Optional;

import javax.annotation.Nullable;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;
import org.springframework.util.StringUtils;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.directive.server.SendPushMessageDirective;
import ru.yandex.alice.kronstadt.core.directive.server.SendPushMessageDirective.AppType;
import ru.yandex.alice.kronstadt.core.layout.TextCard;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.domain.StartAccountLinkingButton;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.megamind.AliceHandledWithDevConsoleMessageException;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.RequestEnrichmentData;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.AccountLinkingCompleteDirective;
import ru.yandex.alice.paskill.dialogovo.utils.VoiceUtils;

@Component
class StartAccountLinkingResponseHandler implements WebhookResponseHandler {
    private final DialogovoDirectiveFactory directiveFactory;
    private final RequestContext requestContext;
    private final DirectiveToDialogUriConverter converter;
    private final Phrases phrases;
    private final FingerprintCalculator fingerprintCalculator;
    @Value("${accountLinking.url}")
    private String accountLinkingUrl;

    private static final Logger logger = LogManager.getLogger();

    StartAccountLinkingResponseHandler(
            DialogovoDirectiveFactory directiveFactory,
            RequestContext requestContext,
            DirectiveToDialogUriConverter converter,
            Phrases phrases,
            FingerprintCalculator fingerprintCalculator
    ) {

        this.directiveFactory = directiveFactory;
        this.requestContext = requestContext;
        this.converter = converter;
        this.phrases = phrases;
        this.fingerprintCalculator = fingerprintCalculator;
    }

    @Override
    public SkillProcessResult.Builder handleResponse(SkillProcessResult.Builder builder,
                                                     SkillProcessRequest req,
                                                     Context context,
                                                     RequestEnrichmentData enrichmentData,
                                                     WebhookResponse response) {

        var skill = req.getSkill();
        var clientInfo = req.getClientInfo();

        if (skill.getSocialAppName().isEmpty()) {
            throw new AliceHandledWithDevConsoleMessageException("Skill doesn't support account linking",
                    ErrorAnalyticsInfoAction.REQUEST_FAILURE, "Oauth приложение не настроено", "Навык не отвечает.");
        }

        if (!clientInfo.isSupportsAccountLinking()) {
            String msg = "Навык запрашивает авторизацию, но к сожалению, " +
                    "на этом устройстве такая возможность не поддерживается";
            throw new AliceHandledWithDevConsoleMessageException("Surface is not supported",
                    ErrorAnalyticsInfoAction.REQUEST_FAILURE,
                    msg, msg);
        }

        builder.getLayout().shouldListen(false);
        if (req.getSkill().hasFeatureFlag(SkillFeatureFlag.START_ACCOUNT_LINKING_CUSTOM_ANSWER)) {
            builder.getLayout().shouldListen(response.getResponse().flatMap(Response::getShouldListen)
                    .orElse(false));
        }

        var link = generateAccountLinkingLink(req);
        // YaBro|SearchApp
        if (clientInfo.isYaBrowser() || clientInfo.isSearchApp()) {
            var text = phrases.getRandom("account.linking.start.without.push", req.getRandom(),
                    VoiceUtils.normalize(skill.getName()));
            var directives = directiveFactory.createLinkDirective(link);
            var textCard = TextCard.builder()
                    .setText(text)
                    .button(new StartAccountLinkingButton("Авторизоваться", directives, link))
                    .build();
            builder.getLayout().textCard(textCard);
            return builder
                    .appendTts(text, null, " ");
            // Console
        } else if (clientInfo.isDevConsole()) {
            setTextWithOverrides(req, skill, response, builder);
            return builder;
        }

        // Station
        try {
            if (requestContext.getCurrentUserId() != null && context.getSource().isByUser()) {
                // currentUserId is always present for station requests but may be absent for dev-console auth request
                logger.info("send start account linking push for user {}", requestContext.getCurrentUserId());
                builder.serverDirective(new SendPushMessageDirective(
                        "Навык: " + skill.getName(),
                        "Запрос авторизации в навыке",
                        link,
                        "alice_skill_account_linking",
                        "alice-skill-start-purchase-request",
                        SendPushMessageDirective.ALICE_DEFAULT_DEVICE_ID,
                        List.of(AppType.SEARCH_APP),
                        "Авторизация в навыке " + skill.getName(),
                        "Авторизоваться"
                ));
            }

            setTextWithOverrides(req, skill, response, builder);
            return builder;
        } catch (Exception e) {
            throw new AliceHandledWithDevConsoleMessageException("Unable to handle account linking complete",
                    ErrorAnalyticsInfoAction.ACCOUNT_LINKING_FAILURE,
                    "Не удалось выполнить авторизацию.",
                    "Не удалось выполнить авторизацию.",
                    "Не удалось выполнить авторизацию",
                    true,
                    e);
        }
    }

    private void setTextWithOverrides(SkillProcessRequest req, SkillInfo skill,
                                      WebhookResponse webhookResponse, SkillProcessResult.Builder builder) {
        String generalText = phrases.getRandom("account.linking.start.with.push", req.getRandom(), skill.getNameTts());

        builder.getLayout().textCard(new TextCard(generalText));
        builder.appendTts(generalText, null, " ");

        var responseO = webhookResponse.getResponse();
        // custom overridable text from skill - consider as deprecated
        if (req.getSkill().hasFeatureFlag(SkillFeatureFlag.START_ACCOUNT_LINKING_CUSTOM_ANSWER)) {
            if (responseO.isPresent()) {
                Response response = responseO.get();

                if (!StringUtils.isEmpty(response.getText())) {
                    builder.getLayout().setCards(List.of(new TextCard(response.getText())));
                }

                if (!StringUtils.isEmpty(response.getTts())) {
                    builder.setTts(response.getTts(), skill.getVoice());
                }
            }
        }
    }

    private URI generateAccountLinkingLink(SkillProcessRequest req) {
        var clientInfo = req.getClientInfo();
        if (clientInfo.isYaSmartDevice() && clientInfo.getDeviceIdO().isEmpty()) {
            throw new AliceHandledWithDevConsoleMessageException("Missing device id",
                    ErrorAnalyticsInfoAction.ACCOUNT_LINKING_FAILURE,
                    "Не удалось выполнить авторизацию.", "Не удалось выполнить авторизацию.");
        }
        @Nullable String initialDeviceId = req.getClientInfo().isYaSmartDevice()
                ? clientInfo.getDeviceIdO().get()
                : null;

        Optional<String> fingerprint;
        if (req.getSkill().hasFeatureFlag(SkillFeatureFlag.OAUTH_VTB_FINGERPRINT)) {
            fingerprint = Optional.of(
                    fingerprintCalculator.calculateVtbFingerprint(req.getSkill(), req.getApplicationId())
            );
            logger.info("Adding fingerprint parameter: {}", fingerprint);
        } else {
            fingerprint = Optional.empty();
        }

        return UriComponentsBuilder.fromUriString(accountLinkingUrl)
                .pathSegment(req.getSkill().getId())
                .queryParam("session_type", req.isVoiceSession() ? "voice" : "text")
                .queryParamIfPresent("user_data", fingerprint)
                .queryParam(
                        "directives",
                        converter.wrapCallbackDirectiveToBase64(
                                new AccountLinkingCompleteDirective(req.getSkill().getId(), initialDeviceId))
                ).build().toUri();
    }
}
