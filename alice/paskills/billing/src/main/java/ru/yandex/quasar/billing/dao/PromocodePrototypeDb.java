package ru.yandex.quasar.billing.dao;

import javax.annotation.Nullable;

import lombok.Data;
import lombok.With;
import org.springframework.data.annotation.Id;
import org.springframework.data.relational.core.mapping.Column;
import org.springframework.data.relational.core.mapping.Table;


@Data
@Table("promocode_prototype")
public class PromocodePrototypeDb {
    @Id
    @With
    @Nullable
    @Column("promocode_prototype_id")
    private final Integer id;
    private final String platform;
    private final String promoType;
    private final String code;
    private final String taskId;
}
