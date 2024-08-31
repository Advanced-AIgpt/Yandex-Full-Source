package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.IngredientProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.JsonEntityProvider.EntityNotFound
import java.util.ArrayDeque
import java.util.Queue

sealed interface Ingredient : NamedEntity {
    val children: List<String>

    /**
     * Recursively get all children.
     */
    fun getAllChildren(ip: IngredientProvider): Set<Ingredient> {
        val result: MutableSet<Ingredient> = HashSet()
        val nodesToVisit: Queue<Ingredient> = ArrayDeque()
        nodesToVisit.add(this)
        // use for-loop instead of while(nodesToVisit.isEmpty()) to prevent infinite loop
        var nodeCounter = 0
        while (!nodesToVisit.isEmpty() && nodeCounter < MAX_INGREDIENTS_IN_GROUP) {
            val currentNode = nodesToVisit.poll()
            result.add(currentNode)
            for (childId in currentNode.children) {
                val child: Ingredient = try {
                    ip.get(childId)
                } catch (e: EntityNotFound) {
                    // should never happen because IngredientProvider is validated on bean creation
                    // TODO: add exception-safe IngredientProvider.get()
                    throw RuntimeException("Failed to get ingredient " + e.ingredient, e)
                }
                if (!result.contains(child)) {
                    nodesToVisit.add(child)
                }
            }
            nodeCounter++
        }
        return result
    }

    companion object {
        const val MAX_INGREDIENTS_IN_GROUP = 1000
    }
}
