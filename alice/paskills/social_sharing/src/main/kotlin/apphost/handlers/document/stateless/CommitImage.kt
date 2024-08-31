package ru.yandex.alice.social.sharing.apphost.handlers.document.stateless

import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.AVATARS_IMAGE
import ru.yandex.alice.social.sharing.document.Image
import ru.yandex.alice.social.sharing.document.ImageProvider
import ru.yandex.alice.social.sharing.document.StatelessDocumentContext
import ru.yandex.alice.social.sharing.proto.context.StatelessDocument
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class CommitImage(
        private val imageProvider: ImageProvider,
        private val objectMapper: ObjectMapper,
        @Value("\${avatars.namespace}") private val namespace: String,
        @Value("\${avatars.skill_card.alias}") private val alias: String,
) : ApphostHandler {

    override val path = "/stateless_document/commit_image"

    override fun handle(context: RequestContext) {
        val params = StatelessDocumentContext.fromContext(context).urlParams
        val avatarsHttpResponse = context
            .getSingleRequestItem("avatars_http_response")
            .getProtobufData(Http.THttpResponse.getDefaultInstance())
        val avatarsResponse = objectMapper.readValue(
            avatarsHttpResponse.content.toStringUtf8(),
            AvatarsResponse::class.java)
        val image = Image(params.imageUrl, avatarsResponse.groupId, avatarsResponse.imageName)
        imageProvider.upsert(image, avatarsHttpResponse.content.toStringUtf8())
        context.addProtobufItem(
                AVATARS_IMAGE,
            StatelessDocument.TAvatarsImage.newBuilder()
                .setUrl(image.avatarsUrl(namespace, alias).toString())
                .setAvatarsId(image.avatarsId)
                .build()
        )
    }
}
