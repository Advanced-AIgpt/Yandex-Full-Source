package ru.yandex.quasar.billing.dao;

import java.time.Instant;

import javax.annotation.Nonnull;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.ToString;
import lombok.With;
import org.springframework.data.annotation.Id;
import org.springframework.data.relational.core.mapping.Column;

/**
 * Generic product to be registered in trust and used for all one-time purchases
 */
@Getter
@ToString
@AllArgsConstructor(access = AccessLevel.PACKAGE)
public class GenericProduct {

    @Id
    @Column("generic_product_id")
    @With
    private final Long id;

    @Nonnull
    private final Long partnerId;
    /**
     * trust product id
     */
    @Nonnull
    private final String productCode;
    @Nonnull
    private final Instant createdAt;

    public static GenericProduct create(Long partnerId, String productCode) {
        return new GenericProduct(null, partnerId, productCode, Instant.now());
    }

}
