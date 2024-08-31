package ru.yandex.quasar.billing.dao;

import java.util.List;

import org.springframework.data.repository.CrudRepository;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;


public interface PromocodePrototypeDao extends CrudRepository<PromocodePrototypeDb, Integer> {

    @Override
    @ReadOnlyTransactional
    List<PromocodePrototypeDb> findAll();

}
