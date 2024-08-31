package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import javax.validation.Validation;
import javax.validation.Validator;

import lombok.Data;
import org.hibernate.validator.constraints.URL;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertTrue;

class TestUrlValidation {

    private final Validator validator = Validation.buildDefaultValidatorFactory().getValidator();

    @Data
    static class T {
        @URL(regexp = "^http(s)?.*")
        private final String url;
    }

    @Test
    void testHttp() {
        var violation = validator.validate(new T("http://ya.ru")).stream().findFirst();
        assertTrue(violation.isEmpty());
    }

    @Test
    void testHttps() {
        var violation = validator.validate(new T("https://ya.ru")).stream().findFirst();
        assertTrue(violation.isEmpty());

    }

    @Test
    void testFtp() {
        var violation = validator.validate(new T("ftp://ya.ru")).stream().findFirst();
        assertTrue(violation.isPresent());
    }
}
