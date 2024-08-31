package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonIgnore;

/**
 * An abstract class for DTO containers for collections.
 *
 * @param <T> type for transferred object
 */
public abstract class MultipleDTO<T> extends Metadated {
    @JsonIgnore
    public abstract List<T> getPayload();
}
