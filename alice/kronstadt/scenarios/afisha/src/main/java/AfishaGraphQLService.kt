package ru.yandex.alice.kronstadt.scenarios.afisha

import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.tvm.USER_TICKET_HEADER
import ru.yandex.alice.kronstadt.scenarios.afisha.graphql.EventType
import ru.yandex.alice.kronstadt.scenarios.afisha.graphql.GraphQLQueryBuilder
import ru.yandex.alice.kronstadt.scenarios.afisha.graphql.Parameters
import ru.yandex.alice.kronstadt.scenarios.afisha.model.request.QueryRequest
import ru.yandex.alice.paskills.common.apphost.http.HttpRequest

@Component
class AfishaGraphQLService(
    private val requestContext: RequestContext,
    private val graphQLQueryBuilder: GraphQLQueryBuilder,
    @Value("\${image.size}") private val imageSize: String,
    @Value("\${date.short}") private val shortDate: Boolean,
    @Value("\${tag}") private val tag: String
) {
    private val httpScheme: HttpRequest.Scheme = HttpRequest.Scheme.HTTPS

    fun buildRequest(request: MegaMindRequest<Any>): HttpRequest<QueryRequest> {
        val userTicket = requestContext.currentUserTicket ?: error("User ticket is null")

        val (lat: Double?, lon: Double?) = if (request.getLocationInfoO().isPresent) {
            val locationInfo = request.getLocationInfoO().get()
            Pair(locationInfo.lat, locationInfo.lon)
        } else {
            logger.info("Location info for client is not present, getting request for default location")
            Pair(null, null)
        }

        val queryRequest = QueryRequest(
            query = graphQLQueryBuilder.buildQuery(
                QUERY_NAME,
                listOf(
                    EventType.ACTUAL_EVENTS,
                    EventType.CONCERT_RECOMMENDATION,
                    EventType.THEATRE_RECOMMENDATION
                ),
                listOf(
                    Parameters.IMAGE_SIZE,
                    Parameters.SHORT_DATE,
                    Parameters.TAG
                )
            ),
            variables = mapOf(
                Parameters.IMAGE_SIZE.paramName to imageSize,
                Parameters.SHORT_DATE.paramName to shortDate,
                Parameters.TAG.paramName to tag
            ),
            lat = lat,
            lon = lon
        )
        return HttpRequest.builder<QueryRequest>("/graphql?query_name=$QUERY_NAME")
            .content(queryRequest)
            .scheme(httpScheme)
            .method(HttpRequest.Method.POST)
            .headers(
                mapOf(
                    USER_TICKET_HEADER to userTicket
                )
            ).build()
    }

    companion object {
        private val logger = LogManager.getLogger(AfishaGraphQLService::class.java)
        private const val QUERY_NAME = "afisha"
    }
}
