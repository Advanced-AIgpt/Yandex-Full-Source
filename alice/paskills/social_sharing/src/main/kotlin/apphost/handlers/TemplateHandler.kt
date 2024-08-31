package ru.yandex.alice.social.sharing.apphost.handlers

import NAppHostHttp.Http
import com.google.protobuf.ByteString
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.proto.WebApi
import ru.yandex.web.apphost.api.request.RequestContext
import java.nio.charset.Charset

internal fun httpResponseFromString(body: String, headers: List<Http.THeader>): Http.THttpResponse {
    return Http.THttpResponse.newBuilder()
        .setContent(ByteString.copyFrom(body, Charset.forName("UTF-8")))
        .addAllHeaders(headers)
        .build()
}

/**
 * Dummy template node used for debug purposes.
 * Will be replaced by report renderer.
 */
interface TemplateHandler: ApphostHandler {

    override val path: String
        get() = "/templates"

    fun render(templateData: WebApi.TWebPageTemplateData): Http.THttpResponse
    fun renderNotFound(): Http.THttpResponse

    override fun handle(context: RequestContext) {
        val templateData = context
            .getSingleRequestItemO(WEB_PAGE_DATA)
        val response = templateData
            .map { render(it.getProtobufData(WebApi.TWebPageTemplateData.getDefaultInstance())) }
            .orElse(renderNotFound())
        context.addProtobufItem(PROTO_HTTP_RESPONSE, response)
    }

}

class HtmlTemplateHandler: TemplateHandler {

    private val template = this::class.java.classLoader.getResource("html/index.html")!!.readText()

    override fun render(templateData: WebApi.TWebPageTemplateData): Http.THttpResponse {
        val message = if (templateData.hasDocument()) {
            template
        } else {
            "Not found"
        }
        return httpResponseFromString(message, DEFAULT_HEADERS_HTML)
    }

    override fun renderNotFound(): Http.THttpResponse {
        return httpResponseFromString("Web page data not found", DEFAULT_HEADERS_HTML)
    }
}

@Configuration
open class TemplateConfiguration {
    @Bean("templateHandler")
    open fun templateHandler(): TemplateHandler {
        return HtmlTemplateHandler()
    }
}
