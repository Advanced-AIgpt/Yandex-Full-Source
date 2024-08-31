package ru.yandex.quasar.billing.services.tvm;

import java.util.Map;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class TvmToolConfig {
    private final Map<String, Client> clients;

    @Data
    public static class Client {
        private final Map<String, Dst> dsts;
    }

    @Data
    public static class Dst {
        @JsonProperty("dst_id")
        private final int dstId;
    }
}
