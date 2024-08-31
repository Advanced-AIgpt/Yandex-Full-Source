package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.Countable

internal sealed interface CountableNamedEntity : NamedEntity, Countable
