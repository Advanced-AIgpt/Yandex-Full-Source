package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg;

import java.time.Duration;

import org.junit.jupiter.api.Test;

import ru.yandex.alice.kronstadt.core.layout.TextWithTts;

import static org.junit.jupiter.api.Assertions.assertEquals;

class DurationToStringTest {

    void testImpl(
            final Duration duration,
            final String expectedText
    ) {
        TextWithTts textWithTts = DurationToString.render(duration);
        assertEquals(expectedText, textWithTts.getText());
    }

    @Test
    void test_12_seconds() {
        testImpl(Duration.ofSeconds(12), "12 секунд");
    }

    @Test
    void test_2_seconds() {
        testImpl(Duration.ofSeconds(2), "2 секунды");
    }

    @Test
    void test_1_hour() {
        testImpl(Duration.ofHours(1), "1 час");
    }

    @Test
    void test_1h_5m_15s_ignores_seconds() {
        testImpl(Duration.ofSeconds(3915), "1 час 5 минут");
    }

    @Test
    void test_24h() {
        testImpl(Duration.parse("PT24H"), "24 часа");
    }

    @Test
    void test_25h() {
        testImpl(Duration.parse("PT25H"), "1 день 1 час");
    }


}
