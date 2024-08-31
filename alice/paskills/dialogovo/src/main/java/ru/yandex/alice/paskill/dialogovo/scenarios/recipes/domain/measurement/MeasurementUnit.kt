package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement

import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.Countable
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.CountableObject
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.TtsTag
import java.math.BigDecimal

enum class MeasurementUnit(private val pluralForms: CountableObject) : Countable {
    CLOVE(
        CountableObject(
            TextWithTts("зубчик"),
            TextWithTts("зубчика"),
            TextWithTts("зубчиков"),
            TextWithTts("зубчика"),
            TtsTag.MASCULINE
        )
    ),
    CUP(
        CountableObject(
            TextWithTts("чашка"),
            TextWithTts("чашки"),
            TextWithTts("чашек"),
            TextWithTts("чашки"),
            TtsTag.FEMININE
        )
    ),
    GRAM(
        CountableObject(
            TextWithTts("грамм"),
            TextWithTts("грамма"),
            TextWithTts("грамм"),
            TextWithTts("грамма"),
            TtsTag.MASCULINE
        )
    ),
    KILOGRAM(
        CountableObject(
            TextWithTts("килограмм"),
            TextWithTts("килограмма"),
            TextWithTts("килограмм"),
            TextWithTts("килограмма"),
            TtsTag.MASCULINE
        )
    ),
    NONE(
        CountableObject(
            TextWithTts(""),
            TextWithTts(""),
            TextWithTts(""),
            TextWithTts(""),
            TtsTag.DEFAULT
        )
    ),
    PINCH(
        CountableObject(
            TextWithTts("щепотка", "щеп+отка"),
            TextWithTts("щепотки", "щеп+отки"),
            TextWithTts("щепоток", "щеп+оток"),
            TextWithTts("щепотки", "щеп+отки"),
            TtsTag.FEMININE
        )
    ),
    TEA_SPOON(
        CountableObject(
            TextWithTts("чайная ложка"),
            TextWithTts("чайные ложки"),
            TextWithTts("чайных ложек"),
            TextWithTts("чайной ложки"),
            TtsTag.FEMININE
        )
    ),
    TABLE_SPOON(
        CountableObject(
            TextWithTts("столовая ложка"),
            TextWithTts("столовых ложки"),
            TextWithTts("столовых ложек"),
            TextWithTts("столовой ложки"),
            TtsTag.FEMININE
        )
    ),
    LOAF(
        CountableObject(
            TextWithTts("кочан"),
            TextWithTts("кочана"),
            TextWithTts("кочанов"),
            TextWithTts("кочана"),
            TtsTag.MASCULINE
        )
    ),
    LITER(
        CountableObject(
            TextWithTts("литр"),
            TextWithTts("литра"),
            TextWithTts("литров"),
            TextWithTts("литра"),
            TtsTag.MASCULINE
        )
    ),
    MILLILITER(
        CountableObject(
            TextWithTts("миллилитр"),
            TextWithTts("миллилитра"),
            TextWithTts("миллилитров"),
            TextWithTts("миллилитра"),
            TtsTag.MASCULINE
        )
    ),
    HANDFUL(
        CountableObject(
            TextWithTts("горсть"),
            TextWithTts("горсти"),
            TextWithTts("горстей"),
            TextWithTts("горсти"),
            TtsTag.FEMININE
        )
    ),
    SLICE(
        CountableObject(
            TextWithTts("кусочек"),
            TextWithTts("кусочка"),
            TextWithTts("кусочков"),
            TextWithTts("кусочка"),
            TtsTag.MASCULINE
        )
    ),
    HAM_SLICE(
        CountableObject(
            TextWithTts("ломтик"),
            TextWithTts("ломтика"),
            TextWithTts("ломтиков"),
            TextWithTts("ломтика"),
            TtsTag.MASCULINE
        )
    ),
    BUNCH(
        CountableObject(
            TextWithTts("пучок"),
            TextWithTts("пучка"),
            TextWithTts("пучков"),
            TextWithTts("пучка"),
            TtsTag.MASCULINE
        )
    ),
    CHEESE_SLICE(
        CountableObject(
            TextWithTts("слайс"),
            TextWithTts("слайса"),
            TextWithTts("слайсов"),
            TextWithTts("слайса"),
            TtsTag.MASCULINE
        )
    );

    override fun getPluralForm(number: BigDecimal): TextWithTts {
        return pluralForms.getPluralForm(number)
    }

    override fun pluralize(number: BigDecimal): TextWithTts {
        return pluralForms.pluralize(number)
    }
}
