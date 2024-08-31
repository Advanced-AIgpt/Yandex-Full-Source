package ru.yandex.quasar.billing.services.processing.yapay;

import org.springframework.web.client.HttpClientErrorException;

public class TokenNotFound extends Exception {
    TokenNotFound(HttpClientErrorException.NotFound e) {
    }
}
