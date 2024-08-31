package ru.yandex.quasar.billing;

import javax.net.ssl.HttpsURLConnection;

import com.github.tomakehurst.wiremock.WireMockServer;
import com.github.tomakehurst.wiremock.client.WireMock;
import com.github.tomakehurst.wiremock.core.Options;
import com.github.tomakehurst.wiremock.core.WireMockConfiguration;
import org.apache.http.conn.ssl.NoopHostnameVerifier;
import org.apache.http.conn.ssl.TrustSelfSignedStrategy;
import org.apache.http.ssl.SSLContexts;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.extension.AfterAllCallback;
import org.junit.jupiter.api.extension.AfterEachCallback;
import org.junit.jupiter.api.extension.BeforeAllCallback;
import org.junit.jupiter.api.extension.BeforeEachCallback;
import org.junit.jupiter.api.extension.ExtensionContext;
import org.springframework.util.ClassUtils;

import static com.github.tomakehurst.wiremock.client.WireMock.recordSpec;

public class RemoteServiceProxyExtension implements BeforeAllCallback, BeforeEachCallback, AfterEachCallback,
        AfterAllCallback {

    private final String providerUrl;
    private final RemoteServiceProxyMode mode;
    private final WireMockServer server;

    public RemoteServiceProxyExtension(String providerUrl, RemoteServiceProxyMode mode) {
        this(providerUrl, mode, 8080);
    }

    public RemoteServiceProxyExtension(String providerUrl, RemoteServiceProxyMode mode, int port) {
        this(providerUrl, mode, WireMockSpring.options().port(port));
    }

    public RemoteServiceProxyExtension(String providerUrl, RemoteServiceProxyMode mode, Options wireMockOptions) {
        this.providerUrl = providerUrl;
        this.mode = mode;
        this.server = new WireMockServer(wireMockOptions);


    }

    @Override
    public void beforeAll(ExtensionContext context) {
        this.server.start();
        WireMock.configureFor("localhost", server.port());
    }

    @Override
    public void beforeEach(ExtensionContext context) {
        if (!server.isRunning()) {
            server.start();
        }

        if (mode == RemoteServiceProxyMode.RECORDING) {
            server.startRecording(
                    recordSpec()
                            .forTarget(providerUrl)
                            .ignoreRepeatRequests()
            );
        }
    }

    @Override
    public void afterEach(ExtensionContext context) {
        if (mode == RemoteServiceProxyMode.RECORDING) {
            // stop recording
            server.stopRecording();
        }
        if (server.isRunning()) {
            server.stop();
        }
    }

    @Override
    public void afterAll(ExtensionContext context) {
        if (server.isRunning()) {
            server.stop();
        }
        //WireMock.shutdownServer();
    }

    public String getUrl() {
        if (mode == RemoteServiceProxyMode.DISABLED) {
            return providerUrl;
        } else {
            return server.url("");
        }
    }

    public WireMockServer getServer() {
        return server;
    }

    private static class WireMockSpring {

        private static boolean initialized = false;

        public static WireMockConfiguration options() {
            if (!initialized) {
                if (ClassUtils.isPresent("org.apache.http.conn.ssl.NoopHostnameVerifier",
                        null)) {
                    HttpsURLConnection
                            .setDefaultHostnameVerifier(NoopHostnameVerifier.INSTANCE);
                    try {
                        HttpsURLConnection
                                .setDefaultSSLSocketFactory(SSLContexts.custom()
                                        .loadTrustMaterial(null,
                                                TrustSelfSignedStrategy.INSTANCE)
                                        .build().getSocketFactory());
                    } catch (Exception e) {
                        Assertions.fail("Cannot install custom socket factory: [" + e.getMessage() + "]");
                    }
                }
                initialized = true;
            }
            return new WireMockConfiguration();
        }

    }
}
