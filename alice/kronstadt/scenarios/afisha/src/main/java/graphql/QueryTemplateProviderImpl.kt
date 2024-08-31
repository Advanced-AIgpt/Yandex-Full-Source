package ru.yandex.alice.kronstadt.scenarios.afisha.graphql

import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import org.springframework.stereotype.Component
import javax.annotation.PostConstruct

@Component
class QueryTemplateProviderImpl : QueryTemplateProvider {
    private val resolver = PathMatchingResourcePatternResolver()
    private lateinit var queryRequests: Map<EventType, String>

    @PostConstruct
    fun init() {
        queryRequests = EventType.values().associateWith { getQueryFromFile(it) }
    }

    override fun getQuery(eventType: EventType): String {
        return queryRequests[eventType] ?: error("no content for eventType $eventType")
    }

    private fun getQueryFromFile(eventType: EventType): String {
        val path = "${GRAPHQL_DIR}/${eventType.fileName}.graphql"
        val resource = resolver.getResource(path)
        if (!resource.exists()) {
            throw RuntimeException("Resource at path '$path' does not exists")
        }
        return String(resource.inputStream.readBytes())
    }

    companion object {
        private const val GRAPHQL_DIR = "graphql"
    }

}
