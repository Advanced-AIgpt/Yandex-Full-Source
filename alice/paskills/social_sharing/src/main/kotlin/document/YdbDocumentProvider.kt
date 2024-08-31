package ru.yandex.alice.social.sharing.document

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import com.yandex.ydb.table.query.Params
import com.yandex.ydb.table.result.ResultSetReader
import com.yandex.ydb.table.values.OptionalValue
import com.yandex.ydb.table.values.PrimitiveValue
import org.apache.logging.log4j.LogManager
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.paskills.common.ydb.YdbClient
import ru.yandex.alice.social.sharing.proto.WebPageProto
import java.time.Duration
import java.time.Instant
import java.util.concurrent.TimeUnit

private val GET_BY_ID_TEMPLATE = """
    --!syntax_v1
    DECLARE ${"$"}key as Utf8;
    ${"$"}key_hash = Digest::CityHash(${"$"}key);
    SELECT
        key
        , opengraph_title
        , opengraph_type
        , opengraph_url
        , opengraph_description
        , opengraph_image_url
        , opengraph_image_type
        , opengraph_image_width
        , opengraph_image_height
        , opengraph_image_alt
        , required_features
        , template_type
        , template_data
        , typed_semantic_frame
    FROM
        %s
    WHERE
        key_hash = ${"$"}key_hash
        AND key = ${"$"}key
    ;
""".trimIndent()

private val UPSERT_TEMPLATE = """
    --!syntax_v1
    DECLARE ${"$"}key as Utf8;
    DECLARE ${"$"}opengraph_title AS Utf8?;
    DECLARE ${"$"}opengraph_type AS Utf8?;
    DECLARE ${"$"}opengraph_url AS Utf8?;
    DECLARE ${"$"}opengraph_description AS Utf8?;
    DECLARE ${"$"}opengraph_image_url AS Utf8?;
    DECLARE ${"$"}opengraph_image_type AS Utf8?;
    DECLARE ${"$"}opengraph_image_width AS Uint32?;
    DECLARE ${"$"}opengraph_image_height AS Uint32?;
    DECLARE ${"$"}opengraph_image_alt AS Utf8?;
    DECLARE ${"$"}required_features AS Utf8?;
    DECLARE ${"$"}template_type AS Uint32?;
    DECLARE ${"$"}template_data AS String?;
    DECLARE ${"$"}typed_semantic_frame AS String?;
    DECLARE ${"$"}current_utc_timestamp AS Timestamp;
    ${"$"}key_hash = Digest::CityHash(${"$"}key);
    UPSERT INTO %s (
        key_hash
        , key
        , opengraph_title
        , opengraph_type
        , opengraph_url
        , opengraph_description
        , opengraph_image_url
        , opengraph_image_type
        , opengraph_image_width
        , opengraph_image_height
        , opengraph_image_alt
        , required_features
        , template_type
        , template_data
        , typed_semantic_frame
        , created_at
    ) VALUES (
        ${"$"}key_hash
        , ${"$"}key
        , ${"$"}opengraph_title
        , ${"$"}opengraph_type
        , ${"$"}opengraph_url
        , ${"$"}opengraph_description
        , ${"$"}opengraph_image_url
        , ${"$"}opengraph_image_type
        , ${"$"}opengraph_image_width
        , ${"$"}opengraph_image_height
        , ${"$"}opengraph_image_alt
        , ${"$"}required_features
        , ${"$"}template_type
        , ${"$"}template_data
        , ${"$"}typed_semantic_frame
        , ${"$"}current_utc_timestamp
    );
""".trimIndent()

private enum class DocumentTable(
    val table: String
) {
    CANDIDATES("document_candidates"),
    DOCUMENTS("documents"),
}

private val GET_CANDIDATE_BY_ID = GET_BY_ID_TEMPLATE.format(DocumentTable.CANDIDATES.table)
private val GET_DOCUMENT_BY_ID = GET_BY_ID_TEMPLATE.format(DocumentTable.DOCUMENTS.table)
private val UPSERT_CANDIDATE = UPSERT_TEMPLATE.format(DocumentTable.CANDIDATES.table)
private val UPSERT_DOCUMENT = UPSERT_TEMPLATE.format(DocumentTable.DOCUMENTS.table)

private fun mapSocialNetworkImage(reader: ResultSetReader): WebPageProto.TScenarioSharePage.TSocialNetworkMarkup.TImage {
    return WebPageProto.TScenarioSharePage.TSocialNetworkMarkup.TImage.newBuilder()
        .setUrl(reader.getColumn("opengraph_image_url").utf8)
        .setType(reader.getColumn("opengraph_image_type").utf8)
        .setWidth(reader.getColumn("opengraph_image_width").uint32.toInt())
        .setHeight(reader.getColumn("opengraph_image_height").uint32.toInt())
        .setAlt(reader.getColumn("opengraph_image_alt").utf8)
        .build()
}

class YdbDocumentProvider(
    private val ydbClient: YdbClient,
    private val objectMapper: ObjectMapper,
    private val getDocumentTimeout: Duration = Duration.ofMillis(200),
    private val upsertDocumentTimeout: Duration = Duration.ofMillis(500),
) : DocumentProvider() {

    private fun get(id: String, table: DocumentTable): Document? {
        val query = when (table) {
            DocumentTable.CANDIDATES -> GET_CANDIDATE_BY_ID
            DocumentTable.DOCUMENTS -> GET_DOCUMENT_BY_ID
        }
        logger.info("Getting document by id {} from {}", id, table)
        val params = Params.of("\$key", PrimitiveValue.utf8(id))
        logger.debug("executing query\n{}", GET_BY_ID_TEMPLATE)
        val resultFuture = ydbClient.readFirstResultSetAsync(
            "get_${table.table}_by_id",
            query,
            params,
            { r -> mapRowToDocument(r) }
        )
        val results = resultFuture.get(getDocumentTimeout.toMillis(), TimeUnit.MILLISECONDS)
        return if (results.isNotEmpty()) results[0] else null
    }

    override fun get(id: String): Document? {
        return get(id, DocumentTable.DOCUMENTS)
    }

    private fun mapRowToDocument(reader: ResultSetReader): Document {
        val page = WebPageProto.TScenarioSharePage.newBuilder()
        page.socialNetworkMarkup = WebPageProto.TScenarioSharePage.TSocialNetworkMarkup.newBuilder()
            .setTitle(reader.getColumn("opengraph_title").utf8)
            .setType(reader.getColumn("opengraph_type").utf8)
            .setImage(mapSocialNetworkImage(reader))
            .setUrl(reader.getColumn("opengraph_url").utf8)
            .setDescription(reader.getColumn("opengraph_description").utf8)
            .build()
        val serializedRequiredFeatures = reader.getColumn("required_features").utf8
        if (!serializedRequiredFeatures.isNullOrBlank()) {
            page.addAllRequiredFeatures(objectMapper.readValue(serializedRequiredFeatures))
        }
        val templateType = Template.fromId(reader.getColumn("template_type").uint32.toInt())
        val templateDataBytes = reader.getColumn("template_data").string
        when (templateType) {
            Template.NONE -> throw InvalidTemplateError(0)
            Template.DIALOG_WITH_IMAGE -> {
                val template = WebPageProto.TDialogWithImageTemplate.parseFrom(
                    templateDataBytes
                )
                page.setDialogWithImage(template)
            }
            Template.EXTERNAL_SKILL -> {
                val template = WebPageProto.TExternalSkillTemplate.parseFrom(
                    reader.getColumn("template_data").string
                )
                page.setExternalSkill(template)
            }
            Template.GENERATIVE_FAIRY_TALE -> {
                val template = WebPageProto.TGenerativeFairyTaleTemplate.parseFrom(
                    reader.getColumn("template_data").string
                )
                page.setFairyTaleTemplate(template)
            }
        }
        page.setFrame(FrameProto.TTypedSemanticFrame.parseFrom(reader.getColumn("typed_semantic_frame").string))
        return Document(reader.getColumn("key").utf8, page.build())
    }

    private fun upsert(document: Document, table: DocumentTable) {
        val query = when (table) {
            DocumentTable.CANDIDATES -> UPSERT_CANDIDATE
            DocumentTable.DOCUMENTS -> UPSERT_DOCUMENT
        }
        logger.info("executing query:\n{}", query)
        logger.info("Upserting document with id {} into {}", document.id, table)
        val params = Params.create()
        params.put("\$key", document.id.toUtf8())
        params.put("\$opengraph_title", document.opengraphTitle.toUtf8Opt())
        params.put("\$opengraph_type", document.opengraphType.toUtf8Opt())
        params.put("\$opengraph_url", document.opengraphUrl.toUtf8Opt())
        params.put("\$opengraph_description", document.opengraphDescription.toUtf8Opt())
        params.put("\$opengraph_image_url", document.opengraphImageUrl.toUtf8Opt())
        params.put("\$opengraph_image_type", document.opengraphImageType.toUtf8Opt())
        params.put("\$opengraph_image_width", document.opengraphImageWidth.toUint32Opt())
        params.put("\$opengraph_image_height", document.opengraphImageHeight.toUint32Opt())
        params.put("\$opengraph_image_alt", document.opengraphImageAlt.toUtf8Opt())
        val serializedRequiredFeatures: OptionalValue = if (!document.requiredFeatures.isNullOrEmpty()) {
            objectMapper.writeValueAsString(document.requiredFeatures).toUtf8Opt()
        } else {
            null.toUtf8Opt()
        }
        params.put("\$required_features", serializedRequiredFeatures)
        params.put("\$template_type", document.templateType.id.toUint32Opt())
        params.put("\$template_data", document.templateData?.toByteArray().toYdbStringOpt())
        params.put("\$typed_semantic_frame", document.typedSemanticFrame.toYdbStringOpt())
        params.put("\$current_utc_timestamp", Instant.now().toTimestamp())
        val resultFuture = ydbClient.executeRwAsync(
            "upsert_document_" + table.table,
            query,
            params,
        )
        resultFuture.get(upsertDocumentTimeout.toMillis(), TimeUnit.MILLISECONDS)
    }

    override fun upsertDocument(document: Document) {
        upsert(document, DocumentTable.DOCUMENTS)
    }

    override fun createCandidate(document: Document) {
        upsert(document, DocumentTable.CANDIDATES)
    }

    override fun commitCandidate(id: String) {
        val candidate = get(id, DocumentTable.CANDIDATES)
        if (candidate == null) {
            throw CandidateNotFoundException(id)
        }
        upsertDocument(candidate)
    }

    companion object {
        val logger = LogManager.getLogger()
    }
}
