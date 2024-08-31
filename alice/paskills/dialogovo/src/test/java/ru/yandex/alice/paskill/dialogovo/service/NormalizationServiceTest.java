package ru.yandex.alice.paskill.dialogovo.service;

import org.junit.jupiter.api.Test;

import ru.yandex.alice.paskill.dialogovo.service.normalizer.NormalizationService;
import ru.yandex.alice.paskill.dialogovo.service.normalizer.NormalizationServiceImpl;

import static org.junit.jupiter.api.Assertions.assertEquals;

class NormalizationServiceTest {

    private NormalizationService normalizationService = new NormalizationServiceImpl();

    @Test
    public void testOk() {
        var actual = normalizationService.normalize("сто к одному");
        assertEquals("100 к 1", actual);
    }

    @Test
    public void testToLowercase() {
        var actual = normalizationService.normalize("Привет первый!");
        assertEquals("привет 1", actual);
    }

    @Test
    public void test2008Ok() {
        var actual = normalizationService.normalize("из две тысячи восьмого года");
        assertEquals("из 2008 года", actual);
    }
}
