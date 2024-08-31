package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain

import ru.yandex.alice.kronstadt.core.utils.StringEnum
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeTag

enum class RecipeTag @JvmOverloads constructor(
    private val value: String,
    val type: Type,
    val order: Int = Int.MAX_VALUE
) : StringEnum {
    // dish types
    BREAD("bread", Type.DISH_TYPE),
    BAKED_GOODS("baked_goods", Type.DISH_TYPE),
    DESSERT("dessert", Type.DISH_TYPE),
    DRINKS("drinks", Type.DISH_TYPE),
    JAM("jam", Type.DISH_TYPE),
    PORRIDGE("porridge", Type.DISH_TYPE),
    PRESERVES("preserves", Type.DISH_TYPE),
    SALAD("salad", Type.DISH_TYPE),
    SAUCE("sauce", Type.DISH_TYPE),
    SOUP("soup", Type.DISH_TYPE),  // public tags
    BREAKFAST("breakfast", Type.PUBLIC, 0),
    CHRISTMAS("christmas", Type.PUBLIC),
    DINNER("dinner", Type.PUBLIC, 2),
    EASTER("easter", Type.PUBLIC),
    EASY("easy", Type.PUBLIC, 3),
    FAST("fast", Type.PUBLIC),
    FASTING("fasting", Type.PUBLIC),
    LONG("long", Type.PUBLIC),
    LUNCH("lunch", Type.PUBLIC, 1),
    MASLENITSA("maslenitsa", Type.PUBLIC),
    NEW_YEAR("new_year", Type.PUBLIC),
    HARD("hard", Type.PUBLIC),
    HOLIDAY("holiday", Type.PUBLIC),
    POLDNIK("poldnik", Type.PUBLIC),
    VEGETARIAN("vegetarian", Type.PUBLIC),

    // hidden tags
    FISH("fish", Type.HIDDEN),
    CHICKEN("chicken", Type.HIDDEN),
    MEAT("meat", Type.HIDDEN),
    MILK("milk", Type.HIDDEN),
    HOT("hot", Type.HIDDEN),
    SWEET("sweet", Type.HIDDEN),
    SOUR("sour", Type.HIDDEN);

    override fun value(): String {
        return value
    }

    enum class Type {
        DISH_TYPE, PUBLIC, HIDDEN
    }

    companion object {
        @JvmField
        val COMPARATOR = Comparator
            .comparingInt { obj: RecipeTag -> obj.order }
            .thenComparing { obj: RecipeTag -> obj.value() }

        @JvmField
        val STRING_ENUM_RESOLVER = StringEnumResolver(RecipeTag::class.java)
    }
}
