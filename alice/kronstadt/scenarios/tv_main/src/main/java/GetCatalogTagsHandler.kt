package ru.yandex.alice.kronstadt.scenarios.tvmain

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ScenePrepareBuilder
import ru.yandex.alice.kronstadt.core.rpc.AbstractRpcHandler
import ru.yandex.alice.kronstadt.core.rpc.RpcRequest
import ru.yandex.alice.kronstadt.core.rpc.RpcResponse
import ru.yandex.alice.protos.data.tv.tags.CatalogTag
import ru.yandex.alice.protos.data.tv.tags.CatalogTag.TCatalogTag.ETagType
import ru.yandex.entity.recommender.runtime.proto.TagsRecommender
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry

@Component
class GetCatalogTagsHandler(metricRegistry: MetricRegistry) :
    AbstractRpcHandler<CatalogTag.TGetTagsRequest, CatalogTag.TGetTagsResponse>(
        path = "get_catalog_tags",
        requestProto = CatalogTag.TGetTagsRequest.getDefaultInstance()
    ) {
    // уточнить имена тегов для рейтинга, качества и сортировки
    private var contentTypeTags = addContentTypeTags()
    private var genreTags = addGenreTags()
    private var countryTags = addCountryTags()
    private var yearTags = addYearTags()
    private var qualityTags = addQualityTags()
    private var sortOrderTags = addSortOrderTags()

    private val metricRegistry: MetricRegistry = metricRegistry.subRegistry(Labels.of("scenario", SCENARIO_META.name))

    private var tagsForTagTypes = mapOf(
        ETagType.TT_CONTENT_TYPE to contentTypeTags,
        ETagType.TT_GENRE to genreTags,
        ETagType.TT_COUNTRY to countryTags,
        ETagType.TT_YEAR to yearTags,
        ETagType.TT_QUALITY to qualityTags,
        ETagType.TT_SORT_ORDER to sortOrderTags
    )

    private val ALL_TAGS = ETagType.values().toSet() - ETagType.TT_UNKNOWN - ETagType.UNRECOGNIZED

    override fun setup(request: RpcRequest<CatalogTag.TGetTagsRequest>, responseBuilder: ScenePrepareBuilder) {
        if (request.hasExperiment("use_recommender_category_tags")) {
            val clientSelectedTags = request.requestBody.selectedTagsList
            responseBuilder.addProtobufItem(
                "tags_recommender_request", TagsRecommender.TTagsRecommenderRequest.newBuilder().apply {
                    addAllSelectedTags(clientSelectedTags.map { toTTag(it) })
                    pageName = "tv_catalog"
                }.build()
            )
        }
    }

    override fun process(request: RpcRequest<CatalogTag.TGetTagsRequest>): RpcResponse<CatalogTag.TGetTagsResponse> {
        if ("tags_recommender_response" in request.additionalSources.items) {
            val recommenderTags =
                request.additionalSources.getSingleItem<TagsRecommender.TTagsRecommenderResponse>("tags_recommender_response")?.recommendedTagsList
            val builder = CatalogTag.TGetTagsResponse.newBuilder()
            if (recommenderTags != null) {
                builder.apply { addAllAvailableTags(recommenderTags.map { toTCatalogTag(it) }) }
            }
            return RpcResponse(payload = builder.build())
        } else {
            if (request.hasExperiment("use_recommender_category_tags")) {
                val absentRecommenderRate = metricRegistry.rate("tags_recommender_response_absent")
                absentRecommenderRate.inc()
                logger.warn("No tags recommender response, fallback to hardcoded tags")
            }
            val tagTypes = ALL_TAGS - request.requestBody.selectedTagsList.map { it.tagType }.toSet()
            val builder = CatalogTag.TGetTagsResponse.newBuilder()
            builder.addAllAvailableTags(tagTypes.flatMap { tagsForTagTypes[it] ?: listOf() })
            return RpcResponse(payload = builder.build())
        }
    }

    private fun toTTag(clientTag: CatalogTag.TCatalogTag): TagsRecommender.TTag {
        return TagsRecommender.TTag.newBuilder().apply {
            id = clientTag.id
            name = clientTag.name
            priority = clientTag.priority
        }.build()
    }

    private fun toTCatalogTag(recommenderTag: TagsRecommender.TTag): CatalogTag.TCatalogTag {
        return CatalogTag.TCatalogTag.newBuilder().apply {
            id = recommenderTag.id
            name = recommenderTag.name
            priority = recommenderTag.priority
        }.build()
    }

    private fun addContentTypeTags(): List<CatalogTag.TCatalogTag> {
        fun createContentTypeTag(tagName: String, tagId: String, tagPriority: Int) =
            CatalogTag.TCatalogTag.newBuilder().apply {
                name = tagName
                id = tagId
                priority = tagPriority
                tagType = ETagType.TT_CONTENT_TYPE
            }.build()

        return listOf(
            createContentTypeTag("Фильмы", "film", 100),
            createContentTypeTag("Сериалы", "series", 99),
            createContentTypeTag("Мультфильмы", "anim", 80)
        )
    }

    private fun addGenreTags(): List<CatalogTag.TCatalogTag> {
        fun createGenreTag(tagName: String, tagId: String, tagPriority: Int) =
            CatalogTag.TCatalogTag.newBuilder().apply {
                name = tagName
                id = tagId
                priority = tagPriority
                tagType = ETagType.TT_GENRE
            }.build()

        return listOf(
            createGenreTag("Комедия", "ruw53972", 100),
            createGenreTag("Боевик", "ruw98599", 95),
            createGenreTag("Аниме", "ruw8153", 90),
            createGenreTag("Хоррор", "ruw46906", 85),
            createGenreTag("Фантастика", "ruw74051", 80),
            createGenreTag("Мелодрама", "ruw33652", 75),
            createGenreTag("Детектив", "ruw35153", 70),
            createGenreTag("Приключения", "ruw98603", 65),
            createGenreTag("Триллер", "ruw98644", 60),
            createGenreTag("Драма", "ruw807374", 55),
            createGenreTag("Военный", "ruw98610", 50),
            createGenreTag("Анимация", "ruw4296", 45),

            createGenreTag("Короткометражн...", "ruw123186", 1),
            createGenreTag("Документальный", "ruw77954", 1),
            createGenreTag("Фэнтези", "ruw11267", 1),
            createGenreTag("Криминал", "ruw20340", 1),
            createGenreTag("Детский", "ruw6156498", 1),
            createGenreTag("Семейный", "ruw3780488", 1),
            createGenreTag("Биография", "ruw267584", 1),
            createGenreTag("Исторический", "ruw98609", 1),
            createGenreTag("Мюзикл", "ruw13849", 1),
            createGenreTag("Музыка", "ruw13990", 1),
            createGenreTag("Вестерн", "ruw98600", 1),
            createGenreTag("Ток-шоу", "ruw1091129", 1),
            createGenreTag("Телеигра", "ruw3191169", 1),
            createGenreTag("Новости", "ruw625751", 1),
            createGenreTag("Нуар", "ruw83285", 1)
        )
    }

    private fun addCountryTags(): List<CatalogTag.TCatalogTag> {
        fun createCountryTag(tagName: String, tagId: String, tagPriority: Int) =
            CatalogTag.TCatalogTag.newBuilder().apply {
                name = tagName
                id = tagId
                priority = tagPriority
                tagType = ETagType.TT_COUNTRY
            }.build()

        return listOf(
            createCountryTag("Россия", "ruw9", 100),
            createCountryTag("СССР", "ruw38", 95),
            createCountryTag("Зарубежное", "foreign", 90),
            createCountryTag("Турция", "ruw6153", 85),
            createCountryTag("Южная Корея", "ruw2821", 80),
            createCountryTag("США", "ruw638", 75),

            createCountryTag("Отечественное", "domestic", 10),
            createCountryTag("Великобритания", "ruw110", 10),
            createCountryTag("Франция", "ruw55", 10),
            createCountryTag("Канада", "ruw1132", 10),
            createCountryTag("Германия", "ruw4104", 10),
            createCountryTag("Италия", "ruw65", 10),
            createCountryTag("Украина", "ruw732", 10),
            createCountryTag("Испания", "ruw1492", 10),
            createCountryTag("Япония", "ruw1472", 10),
            createCountryTag("Австралия", "ruw1121", 10),
            createCountryTag("Китай", "ruw1134", 10),
            createCountryTag("Дания", "ruw184", 10),
            createCountryTag("Беларусь", "ruw2938367", 10),
            createCountryTag("Казахстан", "ruw1657", 10),
            createCountryTag("Швеция", "ruw727", 10),
            createCountryTag("Ирландия", "ruw1573919", 10),
            createCountryTag("Норвегия", "ruw2853", 10),
            createCountryTag("Гонконг", "ruw1467", 10),
            createCountryTag("Мексика", "ruw1481", 10),
            createCountryTag("Нидерланды", "ruw1483", 10),
            createCountryTag("Узбекистан", "ruw3640", 10),
            createCountryTag("Бельгия", "ruw1130", 10),
            createCountryTag("Аргентина", "ruw1123", 10),
            createCountryTag("Финляндия", "ruw728", 10),
            createCountryTag("Польша", "ruw58", 10),
            createCountryTag("Новая Зеландия", "ruw1485", 10),
            createCountryTag("Бразилия", "ruw63", 10),
            createCountryTag("ЮАР", "ruw16887", 10),
            createCountryTag("Таиланд", "ruw8072", 10),
            createCountryTag("Венгрия", "ruw1468", 10),
            createCountryTag("Израиль", "ruw2743", 10),
            createCountryTag("Австрия", "ruw1122", 10),
            createCountryTag("Швейцария", "ruw5240", 10),
            createCountryTag("Чехия", "ruw6411", 10)
        )
    }

    private fun addYearTags(): List<CatalogTag.TCatalogTag> {
        fun createYearTag(tagId: String, tagName: String = tagId, tagPriority: Int) =
            CatalogTag.TCatalogTag.newBuilder().apply {
                name = tagName
                id = tagId
                priority = tagPriority
                tagType = ETagType.TT_YEAR
            }.build()

        return listOf(
            createYearTag("2022", tagPriority = 100),
            createYearTag("2021", tagPriority = 95),
            createYearTag("2011-2020", tagPriority = 90),
            createYearTag("2000-2010", tagPriority = 85),
            createYearTag("1990-2000", tagPriority = 80),
            createYearTag("0-1980", tagPriority = 75)
        )
    }

    private fun addQualityTags(): List<CatalogTag.TCatalogTag> {
        fun createQualityTag(tagName: String, tagId: String, tagPriority: Int) =
            CatalogTag.TCatalogTag.newBuilder().apply {
                name = tagName
                id = tagId
                priority = tagPriority
                tagType = ETagType.TT_QUALITY
            }.build()

        return listOf(
            createQualityTag("4К", "4k", 50)
        )
    }

    private fun addSortOrderTags(): List<CatalogTag.TCatalogTag> {
        fun createSortOrderTag(tagName: String, tagId: String, tagPriority: Int) =
            CatalogTag.TCatalogTag.newBuilder().apply {
                name = tagName
                id = tagId
                priority = tagPriority
                tagType = ETagType.TT_SORT_ORDER
            }.build()

        return listOf(
            createSortOrderTag("По рейтингу", "rating", 1), createSortOrderTag("По дате выхода", "release_date", 1)
        )
    }

    companion object {
        private val logger = LogManager.getLogger(GetCatalogTagsHandler::class.java)
    }
}
