package ru.yandex.alice.kronstadt.core.text

import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import java.util.Optional

object ListRenderer {
    private fun render(
        list: List<TextWithTts>,
        ordinaryDelimiter: TextWithTts?,
        lastDelimiter: TextWithTts?,
        itemPrefixO: TextWithTts?,
        pauseO: Int?,
    ): TextWithTts {
        return if (list.isEmpty()) {
            TextWithTts.EMPTY
        } else {
            val itemPrefix = itemPrefixO ?: TextWithTts.EMPTY
            val pause = pauseO?.let { p: Int -> "sil<[$p]> " } ?: ""
            val text = StringBuilder()
                .append(itemPrefix.text)
                .append(list[0].text)
            val tts = StringBuilder()
                .append(itemPrefix.tts)
                .append(list[0].tts)
            for (i in 1 until list.size) {
                val isLastIndex = i == list.size - 1
                val item = list[i]
                val delimiter = (if (isLastIndex) lastDelimiter else ordinaryDelimiter) ?: TextWithTts.EMPTY
                text.append(delimiter.text)
                    .append(itemPrefix.text)
                    .append(item.text)
                tts.append(delimiter.tts)
                    .append(if (!isLastIndex) pause else "")
                    .append(itemPrefix.tts)
                    .append(item.tts)
            }
            TextWithTts(text.toString(), tts.toString())
        }
    }

    @JvmStatic
    fun render(list: List<TextWithTts>, pause: Optional<Int>): TextWithTts {
        return render(
            list,
            TextWithTts(", "),
            TextWithTts(" и "),
            null,
            pause.orElse(null)
        )
    }

    @JvmStatic
    fun render(list: List<TextWithTts>, pause: Int?): TextWithTts {
        return render(
            list,
            TextWithTts(", "),
            TextWithTts(" и "),
            null,
            pause,
        )
    }

    @JvmStatic
    fun renderWithLineBreaks(list: List<TextWithTts>, pause: Optional<Int>): TextWithTts {
        return render(
            list,
            TextWithTts("\n", ", "),
            TextWithTts("\n", " и "),
            TextWithTts("- ", ""),
            pause.orElse(null),
        )
    }
}
