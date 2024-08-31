package ru.yandex.alice.paskill.dialogovo.external.v1;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.Locale;
import java.util.Objects;

import javax.validation.ConstraintViolation;
import javax.validation.Valid;
import javax.validation.Validation;
import javax.validation.ValidatorFactory;
import javax.validation.groups.Default;

import com.google.common.base.CaseFormat;
import org.hibernate.validator.messageinterpolation.ResourceBundleMessageInterpolator;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.external.ProtocolValidator;
import ru.yandex.alice.paskill.dialogovo.external.WebhookError;
import ru.yandex.alice.paskill.dialogovo.external.WebhookErrorCode;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Directives;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.AudioPlayerActionType;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;

@Component
public class ProtocolValidatorV1 implements ProtocolValidator {
    private static class RuMessageInterpolator extends ResourceBundleMessageInterpolator {
        @Override
        public String interpolate(String message, Context context) {
            return super.interpolate(message, context, new Locale("ru", "RU"));
        }
    }

    private final ValidatorFactory validatorFactory = Validation
            .byDefaultProvider()
            .configure()
            .messageInterpolator(new RuMessageInterpolator())
            .buildValidatorFactory();

    @Override
    public ProtocolValidationResult validate(WebhookResponse resp, SourceType source) {
        var validator = validatorFactory.getValidator();
        // enable validation for SourceType validation group
        var violations = validator.validate(resp, Default.class, source.getValidationGroupClass());

        var errors = new ArrayList<WebhookError>();
        if (resp.getResponse().isEmpty() && !resp.isStartAccountLinkingResponse()) {
            errors.add(WebhookError.create("response", WebhookErrorCode.MISSING_INTENT));
            errors.add(WebhookError.create("start_account_linking", WebhookErrorCode.MISSING_INTENT));
        }

        for (ConstraintViolation<WebhookResponse> v : violations) {
            errors.add(
                    WebhookError.create(
                            CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, v.getPropertyPath().toString()),
                            WebhookErrorCode.INVALID_VALUE,
                            v.getMessage()
                    )
            );
        }

        if (resp.getResponse().isPresent()) {
            @Valid Response response = resp.getResponse().get();
            boolean hasAudioPlayDirective = response.getDirectives().flatMap(Directives::getAudioPlayer)
                    .filter(playerAction -> playerAction.getAction() == AudioPlayerActionType.PLAY).isPresent();
            boolean hasCanvasCommand = response.getDirectives().flatMap(Directives::getCanvasCommand).isPresent();
            boolean hasCanvasShow = response.getDirectives().flatMap(Directives::getCanvasShow).isPresent();
            boolean hasTeasers = response.getTeasersMeta().isPresent();
            boolean hasWidgets = response.getWidgetGalleryMeta().isPresent();
            // allow null text only if one of special directives specified
            if (
                    response.getText() == null && source != SourceType.SYSTEM
                            && !(hasAudioPlayDirective || hasCanvasCommand || hasCanvasShow || hasTeasers || hasWidgets)
            ) {
                errors.add(WebhookError.create("response.text",
                        WebhookErrorCode.INVALID_VALUE,
                        "не должно равняться null"));
            }
        }

        // make order predictable for tests
        Comparator<WebhookError> comparator = Comparator.comparing(it -> Objects.requireNonNullElse(it.path(), ""));
        errors.sort(
                comparator
                        .thenComparing(WebhookError::code)
                        .thenComparing(it -> Objects.requireNonNullElse(it.message(), ""))

        );

        return new ProtocolValidationResult(errors);
    }
}
