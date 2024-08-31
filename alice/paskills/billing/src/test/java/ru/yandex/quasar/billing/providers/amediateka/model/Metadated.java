package ru.yandex.quasar.billing.providers.amediateka.model;

import lombok.Getter;

/**
 * A base class for wrappers that have `metadata` field -- actuall, all wrappers for amediateka api.
 */
@Getter
public class Metadated {
    private Meta meta;

}
