package ru.yandex.alice.paskill.dialogovo.external.v1.request

import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants

class UserAgreementsAcceptedRequest : RequestBase(InputType.USER_AGREEMENTS_ACCEPTED)

class UserAgreementsAcceptedWebhookRequest(
    meta: Meta,
    session: WebhookSession,
    request: UserAgreementsAcceptedRequest,
    state: WebhookState?
) : WebhookRequest<UserAgreementsAcceptedRequest>(meta, session, request, state) {

    override fun getMetricaEvent(): MetricaEvent {
        return MetricaEventConstants.userAgreementsAcceptedMetricaEvent
    }
}

class UserAgreementsRejectedRequest : RequestBase(InputType.USER_AGREEMENTS_REJECTED)

class UserAgreementsRejectedWebhookRequest(
    meta: Meta,
    session: WebhookSession,
    request: UserAgreementsRejectedRequest,
    state: WebhookState?
) : WebhookRequest<UserAgreementsRejectedRequest>(meta, session, request, state) {

    override fun getMetricaEvent(): MetricaEvent {
        return MetricaEventConstants.userAgreementsRejectedMetricaEvent
    }
}
