package ru.yandex.alice.kronstadt.server.apphost

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.log.LoggingContext
import ru.yandex.alice.kronstadt.core.log.LoggingContextHolder
import ru.yandex.alice.megamind.protos.scenarios.ScenarioRequestMeta
import ru.yandex.passport.tvmauth.CheckedUserTicket
import ru.yandex.passport.tvmauth.TvmClient
import java.util.UUID

@Component
class ContextPopulator(
    private val requestContext: RequestContext,
    private val tvmClient: TvmClient
) {

    fun populateRequestAndLoggingContext(
        requestMeta: ScenarioRequestMeta.TRequestMeta,
        userAgent: String? = null
    ) {
        requestContext.apply {
            this.requestId = requestMeta.requestId.ifEmpty { UUID.randomUUID().toString() + "-generated-by-kronstadt" }
            if (requestMeta.userTicket.isNotEmpty()) {

                val checkUserTicket: CheckedUserTicket = tvmClient.checkUserTicket(requestMeta.userTicket)
                if (checkUserTicket.booleanValue()) {
                    this.currentUserId = checkUserTicket.defaultUid.toString()
                    this.currentUserTicket = requestMeta.userTicket
                } else {
                    throw RuntimeException("Invalid TVM user ticket. Status: ${checkUserTicket.status}")
                }
            }
            this.userAgent = userAgent
            this.sourceTvmClientId = null // apphost ignores TVM
            this.oauthToken = requestMeta.oAuthToken.ifEmpty { null }
            LoggingContextHolder.Companion.current = LoggingContext.builder()
                .user(requestContext.currentUserId)
                .requestId(requestContext.requestId)
                .build()
        }
    }
}
