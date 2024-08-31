package ru.yandex.alice.kronstadt.scenarios.tvmain.tags

import org.assertj.core.api.Assertions.assertThat
import org.junit.jupiter.api.Test
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.rpc.RpcRequest
import ru.yandex.alice.kronstadt.scenarios.tvmain.GetCatalogTagsHandler
import ru.yandex.alice.kronstadt.scenarios.tvmain.SCENARIO_META
import ru.yandex.alice.protos.data.tv.tags.CatalogTag
import ru.yandex.monlib.metrics.registry.MetricRegistry


class GetCatalogTagsTest {
    @Test
    fun testTagsWithSelected() {
        val selectedTags = CatalogTag.TGetTagsResponse.newBuilder().addAllSelectedTags(
            listOf(
                CatalogTag.TCatalogTag.newBuilder().apply {
                    name = "Сериалы"
                    id = "series"
                    priority = 99
                    tagType = CatalogTag.TCatalogTag.ETagType.TT_CONTENT_TYPE
                }.build(),
                CatalogTag.TCatalogTag.newBuilder().apply {
                    name = "Триллер"
                    id = "ruw98644"
                    priority = 1
                    tagType = CatalogTag.TCatalogTag.ETagType.TT_GENRE
                }.build()
            )
        ).build().selectedTagsList
        val selectedTagTypes =
            setOf(CatalogTag.TCatalogTag.ETagType.TT_CONTENT_TYPE, CatalogTag.TCatalogTag.ETagType.TT_GENRE)

        val request = CatalogTag.TGetTagsRequest.newBuilder().addAllSelectedTags(selectedTags).build()
        val availableTags = GetCatalogTagsHandler(MetricRegistry()).process(
            RpcRequest(
                "",
                requestBody = request,
                SCENARIO_META,
                clientInfo = ClientInfo()
            )
        ).payload.availableTagsList
        val availableTagTypes = CatalogTag.TCatalogTag.ETagType.values()
            .toSet() - CatalogTag.TCatalogTag.ETagType.TT_UNKNOWN - CatalogTag.TCatalogTag.ETagType.UNRECOGNIZED - selectedTagTypes
        availableTags.forEach { tag ->
            assertThat(availableTagTypes.contains(tag.tagType))
        }
    }

    @Test
    fun testTagsWithoutSelected() {
        val request = CatalogTag.TGetTagsRequest.newBuilder().build()
        val availableTags = GetCatalogTagsHandler(MetricRegistry()).process(
            RpcRequest(
                "",
                requestBody = request,
                SCENARIO_META,
                clientInfo = ClientInfo()
            )
        ).payload.availableTagsList
        val availableTagTypes = setOf(availableTags.map { it.tagType })
        val requiredTagTypes = CatalogTag.TCatalogTag.ETagType.values()
            .toSet() - CatalogTag.TCatalogTag.ETagType.TT_UNKNOWN - CatalogTag.TCatalogTag.ETagType.UNRECOGNIZED
        assertThat(requiredTagTypes == availableTagTypes)
    }
}
