package ru.yandex.alice.paskills.my_alice.bunker.main_promo;

import java.util.Optional;

import ru.yandex.alice.paskills.my_alice.layout.PageLayout;

public interface MainPromo {
    Optional<PageLayout.Card> getCard(boolean isAuthorized);

    boolean isReady();
}
