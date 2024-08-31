package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.NamedEntity
import java.util.Optional

abstract class JsonEntityProvider<T : NamedEntity>(
    resourcePath: String,
    objectMapper: ObjectMapper
) {
    val entities: Map<String, T>

    init {
        val entityStream = javaClass.classLoader.getResourceAsStream(resourcePath)!!
        val entityList = objectMapper.readValue<List<T>>(entityStream)
        entities = entityList.associateBy { it.id }
    }

    fun size(): Int = entities.size

    @Throws(EntityNotFound::class)
    operator fun get(id: String): T {
        return entities[id] ?: throw EntityNotFound(id)
    }

    fun getO(id: String): Optional<T> {
        return Optional.ofNullable(entities[id])
    }

    class EntityNotFound(val ingredient: String) : Exception("Entity not found: $ingredient")
}
