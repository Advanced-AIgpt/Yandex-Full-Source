package ru.yandex.quasar.billing.dao;

import java.util.Optional;

import org.springframework.data.jdbc.repository.query.Query;
import org.springframework.data.repository.CrudRepository;
import org.springframework.data.repository.query.Param;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;

public interface GenericProductDao extends CrudRepository<GenericProduct, Long> {
    @ReadOnlyTransactional
    @Query("select * from generic_product where partner_id = :partner_id")
    Optional<GenericProduct> findByPartnerId(@Param("partner_id") Long partnerId);
}
