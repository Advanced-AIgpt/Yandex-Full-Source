package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoEvent
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TRequestSkillWebhookEvent
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TRequestSkillWebhookEvent.TWebhookError
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo
import java.time.Duration
import java.util.stream.Collectors

data class RequestSkillWebhookEvent private constructor(
    val url: String,
    val responseTime: Duration,
    val errors: List<Error>,
    val proxy: ProxyType,
) : AnalyticsInfoEvent() {

    override fun fillProtoField(
        protoBuilder: AnalyticsInfo.TAnalyticsInfo.TEvent.Builder
    ): AnalyticsInfo.TAnalyticsInfo.TEvent.Builder {
        val errs = errors.stream()
            .map { err: Error ->
                TWebhookError.newBuilder()
                    .setErrorType(err.type)
                    .setErrorDetail(err.details)
                    .build()
            }.collect(Collectors.toList())
        return protoBuilder.setRequestSkillWebhookEvent(
            TRequestSkillWebhookEvent.newBuilder()
                .setUrl(url)
                .setResponseTimeMs(responseTime.toMillis())
                .setProxy(proxy.protoValue)
                .addAllErrors(errs)
        )
    }

    data class Error(
        val type: String,
        val details: String,
    )

    companion object {
        @JvmStatic
        fun success(url: String, responseTime: Duration, proxyType: ProxyType): RequestSkillWebhookEvent {
            return RequestSkillWebhookEvent(url, responseTime, emptyList(), proxyType)
        }

        @JvmStatic
        fun error(
            url: String,
            responseTime: Duration,
            errorType: String,
            errorDetail: String,
            proxyType: ProxyType,
        ): RequestSkillWebhookEvent {
            return RequestSkillWebhookEvent(
                url,
                responseTime,
                listOf(Error(errorType, errorDetail)),
                proxyType,
            )
        }

        @JvmStatic
        fun error(
            url: String,
            responseTime: Duration,
            errors: List<Error>,
            proxyType: ProxyType,
        ): RequestSkillWebhookEvent {
            return RequestSkillWebhookEvent(url, responseTime, errors, proxyType)
        }
    }
}
