package ru.yandex.quasar.billing.providers.universal;

import java.util.List;

import javax.annotation.Nonnull;

import lombok.Data;

@Data
class ProductItems {
    @Nonnull
    private final List<ProductItem> products;
}
