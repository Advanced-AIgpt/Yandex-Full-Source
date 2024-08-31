package ru.yandex.alice.paskill.dialogovo.service.xiva;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import lombok.Data;

@JsonIgnoreProperties(ignoreUnknown = true)
@Data
public class XivaSubscriptionInfo {

    /**
     * subscriptions identified
     */
    private final String id;
    /**
     * User uid
     */
    private final String client;
    /**
     * Session provided on subscription of the device
     */
    private final String session;
    /**
     * uuid (not available for websocket subscriptions
     */
    @Nullable
    private final String uuid;
    @Nullable
    private final Integer ttl;
}
