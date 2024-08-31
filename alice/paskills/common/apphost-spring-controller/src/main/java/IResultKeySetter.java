package ru.yandex.alice.paskills.common.apphost.spring;

import ru.yandex.web.apphost.api.request.ApphostResponseBuilder;

public interface IResultKeySetter {

    void set(ApphostResponseBuilder context, Object result);

}
