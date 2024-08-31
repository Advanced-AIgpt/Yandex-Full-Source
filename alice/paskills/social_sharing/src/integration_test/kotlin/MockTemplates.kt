package ru.yandex.alice.social.sharing

import NAppHostHttp.Http
import com.fasterxml.jackson.databind.JsonNode
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.node.ObjectNode
import com.fasterxml.jackson.databind.node.TextNode
import com.google.protobuf.util.JsonFormat
import com.jayway.jsonpath.internal.filter.ValueNodes
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.social.sharing.apphost.handlers.DEFAULT_HEADERS_JSON
import ru.yandex.alice.social.sharing.apphost.handlers.TemplateHandler
import ru.yandex.alice.social.sharing.apphost.handlers.httpResponseFromString
import ru.yandex.alice.social.sharing.proto.WebApi
import ru.yandex.web.apphost.api.AppHostService
import ru.yandex.web.apphost.api.AppHostServiceBuilder

class MockTemplates(
    private val objectMapper: ObjectMapper,
): TemplateHandler {

    override val path = "/"

    override fun render(templateData: WebApi.TWebPageTemplateData): Http.THttpResponse {
        val body = JsonFormat.printer().print(templateData)
        return httpResponseFromString(body, DEFAULT_HEADERS_JSON)
    }

    override fun renderNotFound(): Http.THttpResponse {
        val node = ObjectNode(objectMapper.nodeFactory)
        node.put("error" , "No web page data in apphost context")
        return httpResponseFromString(
            objectMapper.writeValueAsString(node),
            DEFAULT_HEADERS_JSON
        )
    }
}

@Configuration
open class MockTemplateApphostConfiguration {

    @Bean("mockTemplateApphostService", destroyMethod = "stop")
    open fun mockTemplateApphostService(objectMapper: ObjectMapper): AppHostService {
        val handler = MockTemplates(objectMapper)
        val apphostService = AppHostServiceBuilder
            .forPort(0)
            .withPathHandler(handler.path, handler)
            .useHttp()
            .build()
        apphostService.start()
        return apphostService
    }

}
