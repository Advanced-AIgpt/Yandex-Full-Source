package ru.yandex.quasar.billing.beans;

public class DefaultPaymentInfo {

    private String cardId;

    private String email;

    public DefaultPaymentInfo() {
    }

    public DefaultPaymentInfo(String cardId, String email) {
        this.cardId = cardId;
        this.email = email;
    }

    public String getCardId() {
        return cardId;
    }

    public void setCardId(String cardId) {
        this.cardId = cardId;
    }

    public String getEmail() {
        return email;
    }

    public void setEmail(String email) {
        this.email = email;
    }
}
