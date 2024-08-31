package ru.yandex.quasar.billing.dao;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.With;
import org.springframework.data.annotation.Id;
import org.springframework.data.annotation.Immutable;
import org.springframework.data.relational.core.mapping.Table;

@Getter
@AllArgsConstructor
@Table("user_skill_product")
@Immutable
@Builder
public class UserSkillProduct {
    @Id
    @With
    private final Long id;

    private final String uid;

    @Nullable
    private final ProductToken productToken;

    private final SkillProduct skillProduct;

    @Nullable
    private final String skillName;

    @Nullable
    private final String skillImageUrl;
}
