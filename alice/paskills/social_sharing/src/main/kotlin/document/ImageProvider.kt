package ru.yandex.alice.social.sharing.document

import com.yandex.ydb.table.query.Params
import com.yandex.ydb.table.result.ResultSetReader
import com.yandex.ydb.table.values.PrimitiveValue
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.paskills.common.ydb.YdbClient
import java.net.URI
import java.util.concurrent.TimeUnit

data class Image(
    val externalUrl: String,
    val groupId: Long,
    val imageName: String,
) {
    val avatarsId: String
        get() = "$groupId/$imageName"

    fun avatarsUrl(namespace: String, alias: String): URI {
        return UriComponentsBuilder.newInstance()
            .scheme("https")
            .host("avatars.mds.yandex.net")
            .pathSegment("get-$namespace", groupId.toString(), imageName, alias)
            .build()
            .toUri()
    }

}

interface ImageProvider {
    fun get(url: String): Image?
    fun exists(url: String) = get(url) != null
    fun upsert(image: Image, avatarsResponse: String)
}

private val GET_IMAGE = """
    --!syntax_v1
    DECLARE ${"$"}url as Utf8;
    ${"$"}url_hash = Digest::CityHash(${"$"}url);
    SELECT
        url
        , image_name
        , group_id
    FROM
        images
    WHERE
        url_hash = ${"$"}url_hash
        AND url = ${"$"}url
    ;
""".trimIndent()

private val UPSERT_IMAGE = """
    --!syntax_v1
    DECLARE ${"$"}url as Utf8;
    DECLARE ${"$"}group_id as UInt64;
    DECLARE ${"$"}image_name as Utf8;
    DECLARE ${"$"}avatars_response as Utf8;
    ${"$"}url_hash = Digest::CityHash(${"$"}url);
    UPSERT INTO images (
        url_hash
        , url
        , group_id
        , image_name
        , avatars_response
        , created_at
    ) VALUES (
        ${"$"}url_hash
        , ${"$"}url
        , ${"$"}group_id
        , ${"$"}image_name
        , ${"$"}avatars_response
        , CurrentUtcTimestamp()
    );
""".trimIndent()

class YdbImageProvider(
    private val ydbClient: YdbClient,
    private val getImageTimeout: Long,
    private val upsertImageTimeout: Long,
): ImageProvider {

    override fun get(url: String): Image? {
        logger.info("Getting image by url {}", url)
        val params = Params.of("\$url", PrimitiveValue.utf8(url))
        val resultFuture = ydbClient.readFirstResultSetAsync(
            "get_image_by_id",
            GET_IMAGE,
            params,
            { r -> rowToImage(r) }
        )
        val results = resultFuture.get(getImageTimeout, TimeUnit.MILLISECONDS)
        return if (results.isNotEmpty()) results[0] else null
    }

    private fun rowToImage(reader: ResultSetReader): Image {
        return Image(
            reader.getColumn("url").utf8,
            reader.getColumn("group_id").uint64,
            reader.getColumn("image_name").utf8,
        )
    }

    override fun upsert(image: Image, avatarsResponse: String) {
        val params = Params.of(
            "\$url", image.externalUrl.toUtf8(),
            "\$group_id", image.groupId.toUint64(),
            "\$image_name", image.imageName.toUtf8(),
            "\$avatars_response", avatarsResponse.toUtf8(),
        )
        val resultFuture = ydbClient.executeRwAsync(
            "upsert_image",
            UPSERT_IMAGE,
            params,
        )
        resultFuture.get(upsertImageTimeout, TimeUnit.MILLISECONDS)
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}

@Configuration
open class ImageProviderConfiguration {

    @Bean("imageProvider")
    open fun imageProvider(
        ydbClient: YdbClient,
        @Value("\${image.get_by_id_timeout.ms}") getImageTimeout: Long,
        @Value("\${image.upsert.timeout.ms}") upsertImageTimeout: Long,
    ): ImageProvider {
        return YdbImageProvider(ydbClient, getImageTimeout, upsertImageTimeout)
    }
}
