package ru.yandex.alice.kronstadt.core.text.nlg;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

class TemplateToTextConverterTest {

    @Test
    public void testRemoveAccentMarkup() {
        assertEquals(
                "переходим к следующему шагу",
                TemplateToTextConverter.stripMarkup("переходим к следующему ш+агу")
        );
    }

    @Test
    public void testRemoveMultipleAccents() {
        assertEquals(
                "«Поооовар спрашивает поооовара: какова твоя профессия, поооовар?» Помните, это шутку? " +
                        "Это про вас! Вы все приготовили. Ура!",
                TemplateToTextConverter.stripMarkup("«П+о+о+о+овар спрашивает п+о+о+о+овара: какова твоя профессия, " +
                        "п+о+о+о+овар?» Помните, это шутку? Это про вас! Вы все приготовили. Ура!")
        );
    }

}
