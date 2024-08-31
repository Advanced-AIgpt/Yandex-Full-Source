package ru.yandex.alice.paskill.dialogovo.service.logging;

import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestParams;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestResult;

public interface SkillRequestLogger {
    void log(WebhookRequestParams request, WebhookRequestResult response);
}
