package ru.yandex.quasar.billing.services;

import java.util.Map;

import lombok.Data;
import lombok.EqualsAndHashCode;

@Data
public class OfferCardData {
    private final String cardId;
    private final String buttonUrl;
    private final String text;
    @EqualsAndHashCode.Exclude private final long dateFrom;
    @EqualsAndHashCode.Exclude private final long dateTo;
    private final Map<String, Object> params;
}
