package ru.yandex.alice.kronstadt.core.text;

import org.junit.jupiter.api.Test;

import ru.yandex.alice.kronstadt.core.domain.Voice;
import ru.yandex.alice.kronstadt.core.layout.TextWithTts;

import static org.junit.jupiter.api.Assertions.assertEquals;

class TextWithTtsTest {

    @Test
    public void testPlus() {
        var a = new TextWithTts("a", "a");
        var b = new TextWithTts("b", "b");
        var sum = a.plus(b);
        assertEquals(sum, new TextWithTts("a b", "a b"));
    }

    @Test
    public void testPlusTtsNull() {
        var a = new TextWithTts("a", null);
        var b = new TextWithTts("b", null);
        var sum = a.plus(b);
        assertEquals(sum, new TextWithTts("a b"));
        assertEquals(sum.getTts(), "a b");
    }

    @Test
    void testSayWithVoice() {
        var text = new TextWithTts("a", "A");
        assertEquals(text.getTts(Voice.SHITOVA_US), "<speaker voice=\"shitova.us\">A");
    }

    @Test
    void testSayWithVoiceUsesTextIfTtsIsNull() {
        var text = new TextWithTts("a", null);
        assertEquals(text.getTts(Voice.SHITOVA_US), "<speaker voice=\"shitova.us\">a");
    }

}
