package ru.yandex.alice.paskill.dialogovo.webhook.client

import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookRequestBase
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType

data class WebhookRequestParams(
    val skill: SkillInfo,
    val authToken: String?,
    val source: SourceType,
    val experiments: Set<String>,
    val body: WebhookRequestBase,
    /**
     * Alice's request id for internal usage. Should not be sent to skill webhook.
     */
    val internalRequestId: String?,
    /**
     * Alice's device id for internal usage. Should not be sent to skill webhook.
     */
    val internalDeviceId: String?,
    /**
     * Alice's uuid for internal usage. Should not be sent to skill webhook.
     */
    val internalUuid: String?,
) {

    fun getWebhookUrl(): String? {
        return skill.webhookUrl
    }

    fun getFunctionId(): String? {
        return skill.functionId
    }

    fun isUseZora(): Boolean {
        return skill.isUseZora
    }

    fun isYaFunctionRequest(): Boolean {
        return !getFunctionId().isNullOrEmpty()
    }

    class WebhookRequestParamsBuilder(
        var skill: SkillInfo,
        var authToken: String? = null,
        var source: SourceType,
        var experiments: Set<String> = setOf(),
        var body: WebhookRequestBase,
        var internalRequestId: String? = null,
        var internalDeviceId: String? = null,
        var internalUuid: String? = null,
    ) {

        fun skill(skill: SkillInfo): WebhookRequestParamsBuilder {
            this.skill = skill
            return this
        }

        fun authToken(authToken: String?): WebhookRequestParamsBuilder {
            this.authToken = authToken
            return this
        }

        fun source(source: SourceType): WebhookRequestParamsBuilder {
            this.source = source
            return this
        }

        fun experiments(experiments: Set<String>): WebhookRequestParamsBuilder {
            this.experiments = experiments
            return this
        }

        fun body(body: WebhookRequestBase): WebhookRequestParamsBuilder {
            this.body = body
            return this
        }

        fun internalRequestId(internalRequestId: String?): WebhookRequestParamsBuilder {
            this.internalRequestId = internalRequestId
            return this
        }

        fun internalDeviceId(internalDeviceId: String?): WebhookRequestParamsBuilder {
            this.internalDeviceId = internalDeviceId
            return this
        }

        fun internalUuid(internalUuid: String?): WebhookRequestParamsBuilder {
            this.internalUuid = internalUuid
            return this
        }

        fun build(): WebhookRequestParams = WebhookRequestParams(
            skill = skill,
            authToken = authToken,
            source = source,
            experiments = experiments,
            body = body,
            internalRequestId = internalRequestId,
            internalDeviceId = internalDeviceId,
            internalUuid = internalUuid,
        )
    }

    companion object {
        @JvmStatic
        fun builder(
            skill: SkillInfo,
            source: SourceType,
            body: WebhookRequestBase,
        ): WebhookRequestParamsBuilder {
            return WebhookRequestParamsBuilder(
                skill = skill,
                source = source,
                body = body,
            )
        }
    }
}
