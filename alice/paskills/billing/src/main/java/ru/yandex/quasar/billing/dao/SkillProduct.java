package ru.yandex.quasar.billing.dao;

import java.math.BigDecimal;
import java.util.UUID;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import org.springframework.data.annotation.Id;
import org.springframework.data.annotation.Immutable;

@Getter
@AllArgsConstructor
@Immutable
@EqualsAndHashCode(onlyExplicitlyIncluded = true)
@Builder
public class SkillProduct {
    @Id
    @EqualsAndHashCode.Include
    private final UUID uuid;

    @EqualsAndHashCode.Include
    private final String skillUuid;

    private final String name;

    private final SkillProductType type;

    private final BigDecimal price;

    private final boolean deleted;
}
