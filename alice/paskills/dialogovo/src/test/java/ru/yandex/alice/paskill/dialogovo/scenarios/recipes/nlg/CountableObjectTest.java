package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg;

import java.math.BigDecimal;

import org.junit.jupiter.api.Test;

import ru.yandex.alice.kronstadt.core.layout.TextWithTts;

import static org.junit.jupiter.api.Assertions.assertEquals;

class CountableObjectTest {

    private static final CountableObject EMPTY_COUNTABLE = new CountableObject(
            TextWithTts.EMPTY,
            TextWithTts.EMPTY,
            TextWithTts.EMPTY,
            TextWithTts.EMPTY,
            TtsTag.MASCULINE);

    @Test
    public void pluralizeThreeAndAHalf() {
        assertEquals(
                new TextWithTts("3.5", "#mas 3 с половиной"),
                EMPTY_COUNTABLE.pluralize(new BigDecimal("3.5"))
        );
    }

    @Test
    public void pluralizeOne() {
        assertEquals(
                new TextWithTts("1", "#mas 1"),
                EMPTY_COUNTABLE.pluralize(new BigDecimal("1.0"))
        );
    }

    @Test
    public void pluralizeHalf() {
        assertEquals(
                new TextWithTts("0.5", "половина"),
                EMPTY_COUNTABLE.pluralize(new BigDecimal("0.5"))
        );
    }

    @Test
    public void pluralizeThird() {
        assertEquals(
                new TextWithTts("0.33", "одна треть"),
                EMPTY_COUNTABLE.pluralize(new BigDecimal("0.33"))
        );
    }

    @Test
    public void pluralizeQuarter() {
        assertEquals(
                new TextWithTts("0.25", "четверть"),
                EMPTY_COUNTABLE.pluralize(new BigDecimal("0.25"))
        );
    }

    @Test
    public void pluralizeTwoAndAQuarter() {
        assertEquals(
                new TextWithTts("2.25", "#mas 2 с четвертью"),
                EMPTY_COUNTABLE.pluralize(new BigDecimal("2.25"))
        );
    }

}
