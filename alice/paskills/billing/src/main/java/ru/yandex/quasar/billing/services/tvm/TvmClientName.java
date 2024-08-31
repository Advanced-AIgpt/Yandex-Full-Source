package ru.yandex.quasar.billing.services.tvm;

public enum TvmClientName {
    quasar_backend("quasar-backend"),
    bass("bass"),
    blackbox("blackbox"),
    yb_trust_payments("yb-trust-payments"),
    yb_trust_paysys("yb-trust-paysys"),
    music("music"),
    ya_pay("ya-pay"),
    self("self"),
    ott_backoffice("ott-backoffice"),
    dialogovo("dialogovo"),
    droideka("droideka");

    private final String alias;

    TvmClientName(String alias) {
        this.alias = alias;
    }

    public String getAlias() {
        return alias;
    }
}
