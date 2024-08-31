package ru.yandex.alice.paskills.common.apphost.spring;

import org.springframework.context.SmartLifecycle;

import ru.yandex.web.apphost.api.AppHostService;

/**
 * @see org.springframework.boot.web.servlet.context.WebServerGracefulShutdownLifecycle
 */
public class AppHostServiceGracefulShutdownLifecycle implements SmartLifecycle {

    /**
     * {@link SmartLifecycle#getPhase() SmartLifecycle phase} in which graceful shutdown
     * of the web server is performed.
     */
    public static final int SMART_LIFECYCLE_PHASE = SmartLifecycle.DEFAULT_PHASE;

    private final AppHostService appHostService;
    private volatile boolean running;


    public AppHostServiceGracefulShutdownLifecycle(AppHostService appHostService) {
        this.appHostService = appHostService;
    }

    @Override
    public void start() {
        running = true;
    }

    @Override
    public void stop() {
        throw new UnsupportedOperationException("Stop must not be invoked directly");
    }

    @Override
    public void stop(Runnable callback) {
        this.running = false;
        this.appHostService.stop();
        new Thread(() -> {
            try {
                this.appHostService.awaitTermination();
                callback.run();
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }

        }, "apphost-service-shutdown").start();

    }

    @Override
    public boolean isRunning() {
        return running;
    }

    @Override
    public int getPhase() {
        return SMART_LIFECYCLE_PHASE;
    }
}
