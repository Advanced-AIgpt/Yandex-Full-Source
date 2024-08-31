package ru.yandex.alice.kronstadt.scenarios.contacts.mssngr

import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.http.HttpHeaders
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.scenarios.contacts.UploadContactsError
import ru.yandex.alice.kronstadt.scenarios.contacts.mssngr.domain.MssngrApiUploadContactsRequest
import ru.yandex.alice.kronstadt.scenarios.contacts.mssngr.domain.UploadContactsResponse
import ru.yandex.alice.kronstadt.scenarios.contacts.mssngr.domain.converter.UploadContactsRequestConverter
import ru.yandex.alice.paskills.common.apphost.http.HttpRequest
import ru.yandex.alice.protos.data.Contacts
import ru.yandex.kronstadt.alice.scenarios.contacts.proto.TContactsDeviceData

const val X_UUID_HEADER = "X-UUID"

@Component
class MssngrApiService(
    private val objectMapper: ObjectMapper,
    private val requestContext: RequestContext,
    private val uploadContactsRequestConverter: UploadContactsRequestConverter,
    @Value("\${mssgrRegistry.host}") private val mssgrRegistryHost: String,
    @Value("\${tests.it2:false}") isIt2Test: Boolean,
) {

    private val httpScheme: HttpRequest.Scheme

    init {
        this.httpScheme = if (isIt2Test) HttpRequest.Scheme.HTTP else HttpRequest.Scheme.HTTPS
    }

    private val logger = LogManager.getLogger(MssngrApiService::class.java)

    fun parseTHttpMssngrResponse(
        mssngrHttpResponseItem: UploadContactsResponse,
        uuid: String
    ): UploadContactsResponse = try {
        objectMapper.readValue(
            (mssngrHttpResponseItem as Http.THttpResponse).content.toStringUtf8(),
            UploadContactsResponse::class.java
        )
    } catch (e: Exception) {
        throw UploadContactsError(uuid, "Failed parsing response: $mssngrHttpResponseItem")
    }

    fun buildTHttpMssngrRequest(
        request: Contacts.TUpdateContactsRequest,
        contactsDeviceData: TContactsDeviceData,
        fullUpload: Boolean,
        uuid: String
    ): HttpRequest<MssngrApiUploadContactsRequest> {
        val mssgrRequest: MssngrApiUploadContactsRequest = uploadContactsRequestConverter.convert(
            request = request,
            uuid = uuid,
            oldSyncKey = if (fullUpload) 0 else contactsDeviceData.lastSyncKey,
            newSyncKey = contactsDeviceData.lastSyncKey + 1
        )
        logger.debug("Created contact upload messenger request: {}", mssgrRequest)

        return HttpRequest.builder<MssngrApiUploadContactsRequest>("")
            .method(HttpRequest.Method.POST)
            // http схема используется для it2 тестов, так stubber_server не умеет обслуживать https запросы
            // https://wiki.yandex-team.ru/alice/hollywood/integrationtests/faq-i-sbornik-receptov-po-it-testam/#kakprikrutitstubberkhttpsistochniku
            .scheme(httpScheme)
            .headers(
                mapOf(
                    HttpHeaders.HOST to mssgrRegistryHost,
                    HttpHeaders.AUTHORIZATION to "OAuth ${requestContext.oauthToken}",
                    X_UUID_HEADER to uuid,
                )
            )
            .content(mssgrRequest)
            .build()
    }
}
