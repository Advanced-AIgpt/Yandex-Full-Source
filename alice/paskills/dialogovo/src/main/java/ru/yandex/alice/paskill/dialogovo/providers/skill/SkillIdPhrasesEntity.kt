package ru.yandex.alice.paskill.dialogovo.providers.skill

import java.util.UUID

data class SkillIdPhrasesEntity(val id: UUID, val inflectedActivationPhrases: List<String> = listOf())
