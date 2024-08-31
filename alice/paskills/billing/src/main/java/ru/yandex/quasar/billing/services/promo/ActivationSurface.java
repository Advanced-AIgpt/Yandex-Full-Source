package ru.yandex.quasar.billing.services.promo;

public enum ActivationSurface {
    // activation on mobile phone
    PP("PP"),
    // activation on TV
    TV("TV");
    private final String code;

    ActivationSurface(String code) {
        this.code = code;
    }

    public String getCode() {
        return code;
    }
}
