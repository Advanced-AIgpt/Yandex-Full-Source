package ru.yandex.quasar.billing.beans;

public final class Experiments {

    public static final String CHANGE_DEVICE_ACTIVATION_TIME_TO_WEEK_AGO = "change_device_activation_time_to_week_ago";
    public static final String PLUS120_EXPERIMENT = "plus120_EXPERIMENT";
    // эксперимент в ТВ по промокодам на подписку за 149 рублей
    public static final String PLUS90_TV_149RUB = "plus90_tv_149rub";

    public static final String PLUS_MULTI_1M = "plus30_multi_exp";
    public static final String PLUS_MULTI_3M = "plus90_multi_exp";
    public static final String PLUS_MULTI_3M_STATION2 = "plus90_multi_exp_station2";

    public static final String KP_A_3M = "kinopoisk_a3m_multi_exp";
    public static final String KP_A_3M_KPA = "kinopoisk_a3m_kpa_exp";
    public static final String KP_A_6M_KPA = "kinopoisk_a6m_kpa_exp";

    private Experiments() {
        throw new UnsupportedOperationException();
    }
}
