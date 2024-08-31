package ru.yandex.quasar.billing.controller;

import lombok.Data;

@Data
public class SimpleResult {
    public static final SimpleResult OK = new SimpleResult("ok");
    private final String result;
}
