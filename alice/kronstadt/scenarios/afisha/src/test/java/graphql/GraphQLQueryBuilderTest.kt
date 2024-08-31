package graphql

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import org.mockito.kotlin.mock
import org.mockito.kotlin.whenever
import org.mockito.kotlin.verify
import ru.yandex.alice.kronstadt.scenarios.afisha.graphql.EventType
import ru.yandex.alice.kronstadt.scenarios.afisha.graphql.GraphQLQueryBuilder
import ru.yandex.alice.kronstadt.scenarios.afisha.graphql.Parameters
import ru.yandex.alice.kronstadt.scenarios.afisha.graphql.QueryTemplateProvider

class GraphQLQueryBuilderTest {
    private val queryTemplateProviderMock: QueryTemplateProvider = mock()
    private val graphQLQueryBuilder = GraphQLQueryBuilder(queryTemplateProviderMock)
    private val actualEvents =
        """
            actualEvents {
                eventInfo {
                    name
                    image {
                        url
                    }
                }
            }
        """.trimIndent()

    @Test
    fun `Check that query is built correctly`() {
        whenever(queryTemplateProviderMock.getQuery(EventType.ACTUAL_EVENTS)).thenReturn(actualEvents)
        val res = graphQLQueryBuilder.buildQuery("queryName", listOf(EventType.ACTUAL_EVENTS), listOf(Parameters.IMAGE_SIZE, Parameters.SHORT_DATE))
        verify(queryTemplateProviderMock).getQuery(EventType.ACTUAL_EVENTS)
        assertEquals(res, "query queryName(\$imageSize: MediaImageSizes!, \$shortDate: Boolean){actualEvents { eventInfo { name image { url } } }}")
    }


}
