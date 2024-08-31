package ru.yandex.quasar.billing.services.mediabilling;

import javax.annotation.Nullable;

record PromoCodeFeaturesRequestDto(long uid,
                                   String code,
                                   String ip,
                                   @Nullable String platform,
                                   @Nullable Integer region) {

}
