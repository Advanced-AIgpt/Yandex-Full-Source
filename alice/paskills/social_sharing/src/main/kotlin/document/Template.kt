package ru.yandex.alice.social.sharing.document

enum class Template(
    val id: Int
) {
    NONE(0),
    DIALOG_WITH_IMAGE(1),
    EXTERNAL_SKILL(2),
    GENERATIVE_FAIRY_TALE(3);

    companion object {
        private val typeIdToTemplate = Template.values().map { it.id to it }.toMap()

        fun fromId(id: Int): Template {
            val template = typeIdToTemplate[id]
            if (template != null) {
                return template
            } else {
                throw InvalidTemplateError(id)
            }
        }
    }
}

class InvalidTemplateError(val id: Int): Exception("Invalid template id: $id")

