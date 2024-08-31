package ru.yandex.quasar.billing.services.processing;

import java.util.Currency;

import ru.yandex.quasar.billing.exception.NotFoundException;

/**
 * {@link
 * <a href="https://wiki.yandex-team.ru/TRUST/Payments/API/Conventions/#znachenijaizvnutrennixspravochnikov">
 * Справочник валют Траста</a>}
 * Currency list is based on ISO-4217 codes.
 * <p>
 * Note: If we need dynamic set of currencies not only supported by Trust, migrate {@link TrustCurrency} to
 * {@link Currency}
 */
public enum TrustCurrency {
    RUB(Currency.getInstance("RUB")),
    USD(Currency.getInstance("USD")),
    EUR(Currency.getInstance("EUR")),
    CHF(Currency.getInstance("CHF")),
    UAH(Currency.getInstance("UAH")),
    BYN(Currency.getInstance("BYN")),
    KZT(Currency.getInstance("KZT")),
    AMD(Currency.getInstance("AMD")),
    GEL(Currency.getInstance("GEL")),
    TRY(Currency.getInstance("TRY")),
    AZN(Currency.getInstance("AZN")),
    UZS(Currency.getInstance("UZS"));

    private final Currency currency;

    TrustCurrency(Currency currency) {
        this.currency = currency;
    }

    // use this instead of {@code #valueOf} method as it throws handles exception properly
    public static TrustCurrency findByCode(String code) {
        for (TrustCurrency value : TrustCurrency.values()) {
            if (value.getCurrencyCode().equals(code)) {
                return value;
            }
        }
        throw new NotFoundException("Currency not found: " + code);
    }

    /**
     * to mimic {@link Currency} class to switch later
     *
     * @return ISO-4217 code of the currency
     */
    public String getCurrencyCode() {
        return currency.getCurrencyCode();
    }
}
