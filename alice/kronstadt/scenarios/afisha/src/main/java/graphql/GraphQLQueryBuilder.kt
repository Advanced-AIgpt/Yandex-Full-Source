package ru.yandex.alice.kronstadt.scenarios.afisha.graphql

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component

@Component
class GraphQLQueryBuilder(private val queryTemplateProvider: QueryTemplateProvider) {

    fun buildQuery(name: String, eventTypes: List<EventType>, parameters: List<Parameters>): String {
        val params = getParameters(parameters)
        val queries = eventTypes.joinToString(" ") {
            queryTemplateProvider.getQuery(it).replace(Regex("\\s+"), " ")
        }
        logger.debug("Building query with parameters $params, eventTypes $eventTypes")
        return "query $name($params){$queries}"
    }

    private fun getParameters(parameters: List<Parameters>): String {
        return parameters.joinToString(transform = Parameters::getFieldInitialization)
    }

    companion object {
        private val logger = LogManager.getLogger(GraphQLQueryBuilder::class.java)
    }
}
