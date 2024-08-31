package ru.yandex.alice.paskill.dialogovo.utils.client;

import javax.annotation.Nullable;

import org.apache.http.ConnectionReuseStrategy;
import org.apache.http.client.AuthenticationStrategy;
import org.apache.http.client.UserTokenHandler;
import org.apache.http.conn.ConnectionKeepAliveStrategy;
import org.apache.http.conn.HttpClientConnectionManager;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.impl.execchain.ClientExecChain;
import org.apache.http.protocol.HttpProcessor;
import org.apache.http.protocol.HttpRequestExecutor;
import org.apache.http.protocol.ImmutableHttpProcessor;
import org.apache.http.protocol.RequestTargetHost;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.webhook.client.GoZoraRequestHeaders;
import ru.yandex.alice.paskill.dialogovo.webhook.client.GozoraClientId;
import ru.yandex.alice.paskills.common.resttemplate.factory.client.HttpClientFactory;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.passport.tvmauth.TvmClient;

import static ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders.SERVICE_TICKET_HEADER;

@Component
public class GozoraHttpClientFactory extends HttpClientFactory {

    private final TvmClient tvmClient;
    private final GozoraConnectContextHolder gozoraContext;

    public GozoraHttpClientFactory(
            MetricRegistry metricRegistry,
            MetricRegistry metricRegistry1, TvmClient tvmClient,
            GozoraConnectContextHolder gozoraContext
    ) {
        super(metricRegistry);
        this.tvmClient = tvmClient;
        this.gozoraContext = gozoraContext;
    }

    public Builder builderForGozora(GozoraClientId gozoraClientId) {
        return new Builder(new GozoraHttpClientBuilder(tvmClient, gozoraContext, gozoraClientId), false)
                .proxy("go.zora.yandex.net", 1080, "http");
    }

    private static class GozoraHttpClientBuilder extends HttpClientBuilder {

        private final TvmClient tvmClient;
        private final GozoraConnectContextHolder gozoraContext;
        private final GozoraClientId gozoraClientId;

        private GozoraHttpClientBuilder(
                TvmClient tvmClient,
                GozoraConnectContextHolder gozoraContext,
                GozoraClientId gozoraClientId
        ) {
            this.tvmClient = tvmClient;
            this.gozoraContext = gozoraContext;
            this.gozoraClientId = gozoraClientId;
        }

        @SuppressWarnings("ParameterNumber")
        @Override
        protected ClientExecChain createMainExec(
                final HttpRequestExecutor requestExec,
                final HttpClientConnectionManager connManager,
                final ConnectionReuseStrategy reuseStrategy,
                final ConnectionKeepAliveStrategy keepAliveStrategy,
                final HttpProcessor proxyHttpProcessor,
                final AuthenticationStrategy targetAuthStrategy,
                final AuthenticationStrategy proxyAuthStrategy,
                final UserTokenHandler userTokenHandler) {

            final HttpProcessor myProxyHttpProcessor =
                    new ImmutableHttpProcessor(new RequestTargetHost(), (request, context) -> {
                        request.addHeader(SERVICE_TICKET_HEADER, tvmClient.getServiceTicketFor("gozora"));
                        request.addHeader(GoZoraRequestHeaders.CLIENT_ID, gozoraClientId.value());
                        @Nullable String requestId = gozoraContext.getContext().getRequestId();
                        if (requestId != null) {
                            request.addHeader(GoZoraRequestHeaders.REQUEST_ID, requestId);
                        }
                        if (gozoraContext.getContext().getDisableSslValidation()) {
                            request.addHeader(GoZoraRequestHeaders.IGNORE_SSL_ERRORS, "true");
                        }
                    }
                    );

            return super.createMainExec(requestExec, connManager, reuseStrategy, keepAliveStrategy,
                    myProxyHttpProcessor, targetAuthStrategy, proxyAuthStrategy, userTokenHandler);
        }


    }
}
