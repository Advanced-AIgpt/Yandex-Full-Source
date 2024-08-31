package ru.yandex.alice.kronstadt.core.directive

data class TypeTextSilentDirective(val text: String) : MegaMindDirective {

    companion object {
        @JvmField
        val THUMBS_UP = TypeTextSilentDirective(ru.yandex.alice.kronstadt.core.layout.THUMBS_UP)

        @JvmField
        val THUMBS_DOWN = TypeTextSilentDirective(ru.yandex.alice.kronstadt.core.layout.THUMBS_DOWN)
    }
}
