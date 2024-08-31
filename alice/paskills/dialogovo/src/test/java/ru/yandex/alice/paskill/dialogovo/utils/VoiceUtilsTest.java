package ru.yandex.alice.paskill.dialogovo.utils;

import org.junit.jupiter.api.Test;

import static org.junit.Assert.assertEquals;

class VoiceUtilsTest {

    @Test
    public void test() {
        assertEquals(VoiceUtils.normalize("«Угадай животное»"), "«Угадай животное»");
        assertEquals(VoiceUtils.normalize("\uD83D\uDD25Угадай животное\uD83D\uDE02\uD83D\uDC4D"), "Угадай животное");
        assertEquals(VoiceUtils.normalize("Угадай животное 24./7"), "Угадай животное 24./7");
        assertEquals(VoiceUtils.normalize("Угадай животное 24,/7"), "Угадай животное 24,/7");
    }
}
