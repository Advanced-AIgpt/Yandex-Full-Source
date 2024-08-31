package ru.yandex.quasar.billing.services.mediabilling;

import javax.annotation.Nonnull;

import lombok.Data;

@Data
public class PromoCodeCloneResult {

    @Nonnull
    private final String code;

    @Data
    public static class Wrapper {
        private final InvocationInfo invocationInfo;
        private final PromoCodeCloneResult result;
    }

}
