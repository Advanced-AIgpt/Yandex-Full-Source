package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg

import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import java.time.Duration

class DurationToString private constructor() {
    init {
        throw UnsupportedOperationException()
    }

    data class Component(val value: Int, val pluralForm: TextWithTts, val fullText: TextWithTts)

    companion object {
        private val SECOND = CountableObject(
            TextWithTts("секунда", "сек+унда"),
            TextWithTts("секунды", "сек+унды"),
            TextWithTts("секунд", "сек+унд"),
            TextWithTts("секунды"),
            TtsTag.FEMININE
        )
        private val MINUTE = CountableObject(
            TextWithTts("минута", "мин+ута"),
            TextWithTts("минуты", "мин+уты"),
            TextWithTts("минут", "минут"),
            TextWithTts("минуты"),
            TtsTag.FEMININE
        )
        private val HOUR = CountableObject(
            TextWithTts("час", "час"),
            TextWithTts("часа", "час+а"),
            TextWithTts("часа", "час+а"),
            TextWithTts("часа"),
            TtsTag.MASCULINE
        )
        private val HOUR_SHORT = CountableObject(
            TextWithTts("ч", "час"),
            TextWithTts("ч", "час+а"),
            TextWithTts("ч", "час+а"),
            TextWithTts("ч", "час"),
            TtsTag.MASCULINE
        )
        private val ONE_DAY = Duration.ofDays(1)
        private val DAY = CountableObject(
            TextWithTts("день"),
            TextWithTts("дня"),
            TextWithTts("дней"),
            TextWithTts("дня"),
            TtsTag.MASCULINE
        )

        fun toComponents(duration: Duration, useShortHours: Boolean): List<Component> {
            val hours: Int
            val days: Int
            if (ONE_DAY == duration) {
                days = 0
                hours = 24
            } else {
                days = duration.toDaysPart().toInt()
                hours = duration.toHoursPart()
            }
            val components: MutableList<Component> = ArrayList()
            if (days > 0) {
                components.add(Component(days, DAY.getPluralForm(days.toLong()), DAY.pluralize(days.toLong())))
            }
            if (hours > 0) {
                val hour = if (useShortHours) HOUR_SHORT else HOUR
                components.add(Component(hours, hour.getPluralForm(hours.toLong()), hour.pluralize(hours.toLong())))
            }
            val minutes = duration.toMinutesPart()
            if (minutes > 0) {
                components.add(
                    Component(
                        minutes,
                        MINUTE.getPluralForm(minutes.toLong()),
                        MINUTE.pluralize(minutes.toLong())
                    )
                )
            }
            val seconds = duration.toSecondsPart()
            if (seconds > 0 && !(days > 0 || hours > 0)) {
                components.add(
                    Component(
                        seconds,
                        SECOND.getPluralForm(seconds.toLong()),
                        SECOND.pluralize(seconds.toLong())
                    )
                )
            }
            return components
        }

        @JvmStatic
        fun render(duration: Duration): TextWithTts {
            return toComponents(duration, false)
                .map { it.fullText }
                .reduceOrNull(TextWithTts::plus)
                ?: TextWithTts.EMPTY
        }
    }
}
