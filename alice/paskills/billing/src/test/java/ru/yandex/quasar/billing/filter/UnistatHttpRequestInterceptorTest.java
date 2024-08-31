package ru.yandex.quasar.billing.filter;

import java.io.IOException;
import java.net.SocketTimeoutException;
import java.net.URI;
import java.net.URISyntaxException;

import javax.annotation.Nonnull;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mock;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.http.HttpRequest;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpResponse;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.config.UniversalProviderConfig;
import ru.yandex.quasar.billing.services.UnistatService;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@SpringJUnitConfig(classes = {TestConfigProvider.class, UnistatHttpRequestInterceptor.class})
class UnistatHttpRequestInterceptorTest {

    @Autowired
    private UnistatHttpRequestInterceptor interceptor;

    @MockBean
    private UnistatService unistatService;

    @Autowired
    private BillingConfig billingConfig;

    @Mock
    private HttpRequest request;
    @Mock
    private ClientHttpResponse response;
    @Mock
    private ClientHttpRequestExecution execution;
    private String host;
    private byte[] body = {};

    @BeforeEach
    void setUp() throws Exception {
        when(execution.execute(eq(request), any())).thenReturn(response);
        host = billingConfig.getAmediatekaConfig().getApiUrl().replace("https://", "");
    }

    @Test
    void testBasicIntercept() throws URISyntaxException, IOException {
        when(request.getURI()).thenReturn(new URI("https://" + host + "v1/bundles/123123123-asd/items.json"));

        interceptor.intercept(request, body, execution);

        String expectedSignal = signalForHost(host) +
                "_v1-bundles-@-items-json";

        verify(unistatService).logOperationDurationHist(eq("quasar_billing_remote_method_" + expectedSignal +
                "_duration_dhhh"), anyLong());
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_calls_dmmm");
    }

    @Test
    void testTimeoutSignal() throws IOException, URISyntaxException {
        when(request.getURI()).thenReturn(new URI("https://" + host + "v1/bundles/123123123-asd/items.json"));
        when(execution.execute(eq(request), eq(body)))
                .thenThrow(new SocketTimeoutException());

        try {
            interceptor.intercept(request, body, execution);
        } catch (SocketTimeoutException e) { }

        String expectedSignal = signalForHost(host) +
                "_v1-bundles-@-items-json";

        verify(unistatService).logOperationDurationHist(eq("quasar_billing_remote_method_" + expectedSignal +
                "_duration_dhhh"), anyLong());
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_timeout_dmmm");
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_calls_dmmm");
    }

    @Test
    void testInterceptOnDotsInPattern() throws URISyntaxException, IOException {
        // the initial pattern is v1/bundles/@/items.json
        // if the URL is not ".json" but "/json" it doesn't match the pattern so the variable inside it should not be
        // cut out
        when(request.getURI()).thenReturn(new URI("https://" + host + "v1/bundles/123123123-asd/items/json"));

        interceptor.intercept(request, body, execution);

        // attention to "123123123-asd" in signal name
        String expectedSignal = signalForHost(host) + "_v1-bundles-123123123-asd-items-json";

        verify(unistatService).logOperationDurationHist(eq("quasar_billing_remote_method_" + expectedSignal +
                "_duration_dhhh"), anyLong());
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_calls_dmmm");
    }

    @Test
    void testSocialApiTokenDel() throws URISyntaxException, IOException {
        // the initial pattern is v1/bundles/@/items.json
        // if the URL is not ".json" but "/json" it doesn't match the pattern so the variable inside it should not be
        // cut out

        URI uri = new URI(billingConfig.getSocialAPIClientConfig().getSocialApiBaseUrl() + "/token/123123123");
        when(request.getURI()).thenReturn(uri);

        interceptor.intercept(request, body, execution);

        // attention to "123123123-asd" in signal name
        String expectedSignal = signalForHost(uri.getHost()) + "_api-token-@";

        verify(unistatService).logOperationDurationHist(eq("quasar_billing_remote_method_" + expectedSignal +
                "_duration_dhhh"), anyLong());
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_calls_dmmm");
    }

    @Test
    void testSocialApiTokenNewest() throws URISyntaxException, IOException {
        // the initial pattern is v1/bundles/@/items.json
        // if the URL is not ".json" but "/json" it doesn't match the pattern so the variable inside it should not be
        // cut out

        URI uri = new URI(billingConfig.getSocialAPIClientConfig().getSocialApiBaseUrl() + "/token/newest");
        when(request.getURI()).thenReturn(uri);

        interceptor.intercept(request, body, execution);

        // attention to "123123123-asd" in signal name
        String expectedSignal = signalForHost(uri.getHost()) + "_api-token-newest";

        verify(unistatService).logOperationDurationHist(eq("quasar_billing_remote_method_" + expectedSignal +
                "_duration_dhhh"), anyLong());
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_calls_dmmm");
    }

    @Test
    void testKPUniversalProvider() throws URISyntaxException, IOException {

        URI uri = new URI(billingConfig.getUniversalProviders().get("kinopoisk").getBaseUrl() +
                "products/98a493bc-de3d-41c9-af63-bb4e12430de6/0fa7e7c4-fd6d-42ac-bce0-1a5c6d1b7be2/purchase");
        when(request.getURI()).thenReturn(uri);

        interceptor.intercept(request, body, execution);

        // attention to "123123123-asd" in signal name
        String expectedSignal = signalForHost(uri.getHost()) +
                "_products-@-@-purchase";

        verify(unistatService).logOperationDurationHist(eq("quasar_billing_remote_method_" + expectedSignal +
                "_duration_dhhh"), anyLong());
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_calls_dmmm");
    }

    @Test
    void testOttUniversalProvider() throws URISyntaxException, IOException {

        // https://api.ott.yandex.net/
        URI uri = new URI(billingConfig.getUniversalProviders().get("kinopoisk").getBaseUrl() + "content" +
                "/tv_show_episode:432bb1c185cbfa6f86dd6a891d607c4a/options");
        when(request.getURI()).thenReturn(uri);

        interceptor.intercept(request, body, execution);

        // attention to "123123123-asd" in signal name
        String expectedSignal = signalForHost(uri.getHost()) + "_content-@-options";

        verify(unistatService).logOperationDurationHist(eq("quasar_billing_remote_method_" + expectedSignal +
                "_duration_dhhh"), anyLong());
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_calls_dmmm");
    }

    @Test
    void testAmediatekaUniversalProvider() throws URISyntaxException, IOException {

        // http://localhost/provider/amediateka/
        UniversalProviderConfig kinopoisk = billingConfig.getUniversalProviders().get("amediateka");

        URI uri = new URI(kinopoisk.getBaseUrl() + "content/tv_show_episode:432bb1c185cbfa6f86dd6a891d607c4a/options");
        when(request.getURI()).thenReturn(uri);

        interceptor.intercept(request, body, execution);

        // attention to "123123123-asd" in signal name
        String expectedSignal = signalForHost(uri.getHost()) + "_provider-amediateka-content-@-options";

        verify(unistatService).logOperationDurationHist(eq("quasar_billing_remote_method_" + expectedSignal +
                "_duration_dhhh"), anyLong());
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_calls_dmmm");

    }

    @Test
    void testYandexPay() throws URISyntaxException, IOException {

        // https://payments.mail.yandex.net/
        String url = billingConfig.getYaPayConfig().getApiBaseUrl();

        URI uri = new URI(url + "v1/merchant_by_key/bcfb2e42-5136-40f7-8924-4f7fbedc7a5a");
        when(request.getURI()).thenReturn(uri);

        interceptor.intercept(request, body, execution);

        // attention to "123123123-asd" in signal name
        String expectedSignal = signalForHost(uri.getHost()) + "_v1-merchant_by_key-@";

        verify(unistatService).logOperationDurationHist(eq("quasar_billing_remote_method_" + expectedSignal +
                "_duration_dhhh"), anyLong());
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_calls_dmmm");

    }

    @Nonnull
    private String signalForHost(String host) {
        return host.replace('.', '-').replace("/", "");
    }
}
