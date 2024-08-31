package ru.yandex.alice.kronstadt.core.text.nlg;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

class TemplateToTtsConverterTest {

    @Test
    public void testYandexStationFullStopRemoved() {
        assertEquals(
                "Спросите на Яндекс Станции.",
                TemplateToTtsConverter.ttsify("Спросите на Яндекс.Станции.")
        );
    }

}
