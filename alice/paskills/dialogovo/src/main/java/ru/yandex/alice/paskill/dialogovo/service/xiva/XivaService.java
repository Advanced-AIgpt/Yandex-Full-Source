package ru.yandex.alice.paskill.dialogovo.service.xiva;

import javax.annotation.Nullable;

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective;

public interface XivaService {
    void sendCallbackDirectiveAsync(@Nullable String userId, String initialDeviceId, CallbackDirective directive);
}
