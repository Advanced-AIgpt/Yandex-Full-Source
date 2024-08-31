package ru.yandex.alice.kronstadt.core.text

object OrdinalRenderer {

    fun renderOrdinal(ordinal: Int): String? {
        return when (ordinal) {
            1 -> "Первый"
            2 -> "Второй"
            3 -> "Третий"
            4 -> "Четвертый"
            5 -> "Пятый"
            6 -> "Шестой"
            7 -> "Седьмой"
            8 -> "Восьмой"
            9 -> "Девятый"
            10 -> "Десятый"
            else -> null
        }
    }
}
