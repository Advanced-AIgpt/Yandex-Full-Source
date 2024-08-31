package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities

import ru.yandex.alice.kronstadt.core.layout.TextWithTts

interface UncountableNamedEntity : NamedEntity {
    val inflectedName: TextWithTts
}
