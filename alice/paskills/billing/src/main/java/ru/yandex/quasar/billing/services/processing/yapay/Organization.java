package ru.yandex.quasar.billing.services.processing.yapay;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
public class Organization {
    private final String type;
    @JsonProperty("englishName")
    private final String englishName;
    private final String inn;
    private final String ogrn;
    private final String siteUrl;
    private final String fullName;
    private final String name;
    private final String kpp;
    // Режим работы
    @JsonProperty("scheduleText")
    @Nullable
    private final String scheduleText;
}
