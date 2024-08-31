package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents

abstract class RecipeIntent(val name: String) {

    val analyticsInfoName: String = name.replace("alice.recipes.", "")
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is RecipeIntent) return false

        if (name != other.name) return false
        if (analyticsInfoName != other.analyticsInfoName) return false

        return true
    }

    override fun hashCode(): Int {
        var result = name.hashCode()
        result = 31 * result + analyticsInfoName.hashCode()
        return result
    }
}
