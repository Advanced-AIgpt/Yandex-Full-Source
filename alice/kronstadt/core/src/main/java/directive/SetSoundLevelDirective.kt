package ru.yandex.alice.kronstadt.core.directive

class SetSoundLevelDirective private constructor(val level: Int) : MegaMindDirective {

    companion object {
        @JvmStatic
        fun create(level: Int): SetSoundLevelDirective {
            return SetSoundLevelDirective(level)
        }
    }
}
