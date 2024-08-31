package ru.yandex.alice.paskill.dialogovo.webhook.client;

import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;

@Component
public interface WebhookClient {
    /**
     * call webhook handle
     *
     * @param context
     * @param req     webhook request parameters
     * @return processed webhook response
     */
    WebhookRequestResult callWebhook(Context context, WebhookRequestParams req);

}
