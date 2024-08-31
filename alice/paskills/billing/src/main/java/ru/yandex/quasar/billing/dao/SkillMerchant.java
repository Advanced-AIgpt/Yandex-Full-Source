package ru.yandex.quasar.billing.dao;

import javax.annotation.Nonnull;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.ToString;
import org.springframework.data.relational.core.mapping.Column;
import org.springframework.data.relational.core.mapping.Table;

@ToString
@Getter
@EqualsAndHashCode
@Table("skill_merchant")
@AllArgsConstructor(access = AccessLevel.PRIVATE)
@Builder
public class SkillMerchant {

    @Column("merchant_key")
    @Nonnull
    private final String token;

    /**
     * identifier of the merchant request for a given token
     */
    @Column("service_merchant_id")
    @Nonnull
    private final Long serviceMerchantId;

    @Column("entity_id")
    @Nonnull
    private final String entityId;

    @Column("description")
    @Nonnull
    private final String description;
}
