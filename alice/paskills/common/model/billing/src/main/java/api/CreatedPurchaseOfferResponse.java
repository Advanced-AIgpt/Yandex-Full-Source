package ru.yandex.alice.paskills.common.billing.model.api;

import java.net.URI;

import com.fasterxml.jackson.annotation.JsonProperty;

public class CreatedPurchaseOfferResponse {
    @JsonProperty("order_id")
    private final String orderId;
    private final URI url;

    public CreatedPurchaseOfferResponse(
            @JsonProperty("order_id") String orderId,
            @JsonProperty("url") URI url) {
        this.orderId = orderId;
        this.url = url;
    }

    public String getOrderId() {
        return orderId;
    }

    public URI getUrl() {
        return url;
    }

    @Override
    public String toString() {
        return "CreatedPurchaseOfferResponse{" +
                "orderId='" + orderId + '\'' +
                ", url=" + url +
                '}';
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        CreatedPurchaseOfferResponse that = (CreatedPurchaseOfferResponse) o;

        if (orderId != null ? !orderId.equals(that.orderId) : that.orderId != null) {
            return false;
        }
        return url != null ? url.equals(that.url) : that.url == null;
    }

    @Override
    public int hashCode() {
        int result = orderId != null ? orderId.hashCode() : 0;
        result = 31 * result + (url != null ? url.hashCode() : 0);
        return result;
    }
}
