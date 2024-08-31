package ru.yandex.alice.paskills.common.apphost.spring;

import java.io.IOException;

import org.springframework.context.ApplicationContext;
import org.springframework.context.SmartLifecycle;

import ru.yandex.web.apphost.api.AppHostService;

/**
 * @see org.springframework.boot.web.servlet.context.WebServerStartStopLifecycle
 */
class AppHostServiceStartStopLifecycle implements SmartLifecycle {

    private final AppHostService webServer;
    private final ApplicationContext applicationContext;

    private volatile boolean running;

    AppHostServiceStartStopLifecycle(ApplicationContext applicationContext, AppHostService webServer) {
        this.webServer = webServer;
        this.applicationContext = applicationContext;
    }

    @Override
    public void start() {
        try {
            this.webServer.start();
            this.running = true;
            this.applicationContext.publishEvent(new ApphostServiceInitializedEvent(webServer));
        } catch (IOException e) {
            throw new RuntimeException("Failed to start AppHostService", e);
        }
    }

    @Override
    public void stop() {
        this.webServer.stop();
    }

    @Override
    public boolean isRunning() {
        return this.running;
    }

    @Override
    public int getPhase() {
        return Integer.MAX_VALUE - 1;
    }

}
