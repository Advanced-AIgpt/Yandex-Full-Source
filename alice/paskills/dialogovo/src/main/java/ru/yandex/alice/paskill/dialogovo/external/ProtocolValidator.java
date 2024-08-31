package ru.yandex.alice.paskill.dialogovo.external;

import java.util.List;

import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;

public interface ProtocolValidator {

    ProtocolValidationResult validate(WebhookResponse resp, SourceType source);

    record ProtocolValidationResult(List<WebhookError> errors) {
        public boolean isValid() {
            return errors.isEmpty();
        }
    }

}
