package ru.yandex.alice.nlu.libs.fstnormalizer;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

public class FstNormalizerTest {

    @Test
    void test100To1() {
        var actual = new FstNormalizer().normalize(Lang.RUS, "сто к одному");
        assertEquals("100 к 1", actual);
    }

    @Test
    void testLongText() {
        var actual = new FstNormalizer().normalize(Lang.RUS, "попроси розового слона принести швабры");
        assertEquals("попроси розового слона принести швабры", actual);
    }

    @Test
    void testFirst() {
        var actual = new FstNormalizer().normalize(Lang.RUS, "первый номер");
        assertEquals("1 номер", actual);
    }

    @Test
    void test3TimeWinner() {
        var actual = new FstNormalizer().normalize(Lang.RUS, "трижды финалист");
        assertEquals("трижды финалист", actual);
    }

    @Test
    void testEmptyString() {
        var actual = new FstNormalizer().normalize(Lang.RUS, "");
        assertEquals("", actual);
    }

    @Test
    void testNpeOnNullText() {
        assertThrows(NullPointerException.class, ()-> new FstNormalizer().normalize(Lang.RUS, null));
    }

    @Test
    void testNpeOnNullLang() {
        assertThrows(NullPointerException.class, ()-> new FstNormalizer().normalize(null, "привет"));
    }
}