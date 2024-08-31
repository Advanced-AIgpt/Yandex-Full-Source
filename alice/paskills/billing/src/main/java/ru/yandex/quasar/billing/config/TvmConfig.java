package ru.yandex.quasar.billing.config;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;

@Getter
@Setter
@Data
@AllArgsConstructor
@NoArgsConstructor
public class TvmConfig {
    // filled from env variable
    private String tvmToken;
    // TVM client_id of the application
    private Integer clientId;
}
