package ru.yandex.alice.paskills.common.apphost.spring;

import org.springframework.context.ApplicationEvent;

import ru.yandex.web.apphost.api.AppHostService;

/**
 * Event to be published when the ApphostService is ready. Useful for obtaining the local port of a running server
 */
public class ApphostServiceInitializedEvent extends ApplicationEvent {

    ApphostServiceInitializedEvent(AppHostService source) {
        super(source);
    }

    @Override
    public AppHostService getSource() {
        return (AppHostService) super.getSource();
    }
}
