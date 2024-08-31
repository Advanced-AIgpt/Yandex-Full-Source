package ru.yandex.quasar.billing.dao;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.With;
import org.springframework.data.annotation.Id;
import org.springframework.data.annotation.Immutable;
import org.springframework.data.relational.core.mapping.Table;

@Getter
@AllArgsConstructor
@Table("product_token")
@Immutable
@Builder
public class ProductToken {
    @Id
    @With
    private final Long id;

    private final String provider;

    private final String code;

    private final boolean reusable;

    private final SkillProduct skillProduct;
}
