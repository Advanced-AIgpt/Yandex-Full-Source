package ru.yandex.alice.paskill.dialogovo.webhook.client

import ru.yandex.alice.kronstadt.core.utils.StringEnum
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType

object GoZoraErrors {
    const val INVALID_SSL: String = "1000"
}

object GoZoraRequestHeaders {
    const val IGNORE_SSL_ERRORS = "X-Ya-Ignore-Certs"
    const val REQUEST_ID = "X-Yandex-Req-Id"
    const val CLIENT_ID = "X-Ya-Client-Id"
}

object GoZoraResponseHeaders {
    const val ERROR_CODE = "X-Yandex-Gozora-Error-Code"
    const val ERROR_DESCRIPTION = "X-Yandex-Gozora-Error-Description"
}

enum class GozoraClientId(
    private val value: String,
) : StringEnum {
    // default GoZora source for real user traffic
    DEFAULT("dialogovo"),
    // gozora source for skill healthchecks
    PING("dialogovo_ping");

    override fun value(): String {
        return this.value
    }

    companion object {
        @JvmStatic
        fun fromRequestSource(source: SourceType): GozoraClientId {
            return if (source == SourceType.PING) {
                PING
            } else {
                DEFAULT
            }
        }
    }
}
