package ru.yandex.quasar.billing.services.mediabilling;

import lombok.Data;

@Data
public class ErrorResponseDto {
    private final String name;
    private final String message;
}
