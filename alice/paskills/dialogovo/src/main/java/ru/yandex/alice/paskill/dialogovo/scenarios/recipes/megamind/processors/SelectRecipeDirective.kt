package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.directive.Directive

@Directive("select_recipe")
internal class SelectRecipeDirective(val recipeId: String) : CallbackDirective
