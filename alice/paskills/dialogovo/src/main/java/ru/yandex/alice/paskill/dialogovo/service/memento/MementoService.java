package ru.yandex.alice.paskill.dialogovo.service.memento;

import java.util.Optional;

public interface MementoService {

    void updateNewsProviderSubscription(String tvmUserTicket, NewsProviderSubscription subscription);

    Optional<NewsProviderSubscription> getUserNewsProviderSubscription(String tvmUserTicket);
}
