package ru.yandex.alice.vault;

import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.cert.X509Certificate;
import java.text.MessageFormat;
import java.util.Optional;
import java.util.Set;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLParameters;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class VaultClient {

    private static final Set<Integer> SUCCESSFUL_RESPONSE_CODES = Set.of(200);

    private static final Logger logger = LoggerFactory.getLogger(VaultClient.class);

    private final String host;
    private final String oauthToken;
    private final HttpClient httpClient;
    private final ObjectMapper objectMapper;

    public VaultClient(String oauthToken, Type type) {
        if (oauthToken == null || oauthToken.isEmpty()) {
            throw new RuntimeException("OAuth token must be provided");
        }

        this.host = type.getHost();
        this.oauthToken = "OAuth " + oauthToken;


        httpClient = createHttpClient();
        this.objectMapper = new ObjectMapper()
                .disable(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES);
    }

    public VaultClient(String oauthToken) {
        this(oauthToken, Type.PROD);
    }

    /**
     * Yandex Vault uses self-signed SSL certificate so skip certificate validation for yav http client
     */
    private static HttpClient createHttpClient() {
        try {
            TrustManager[] trustAllCerts = new TrustManager[]{
                    new X509TrustManager() {

                        @Override
                        public void checkClientTrusted(X509Certificate[] chain, String authType) {
                        }

                        @Override
                        public void checkServerTrusted(X509Certificate[] chain, String authType) {
                        }

                        @Override
                        public X509Certificate[] getAcceptedIssuers() {
                            return null;
                        }
                    }
            };

            var sslContext = SSLContext.getInstance("SSL");
            sslContext.init(null, trustAllCerts, new SecureRandom());

            var sslParams = new SSLParameters();
            // This should prevent host validation
            sslParams.setEndpointIdentificationAlgorithm("");

            return HttpClient.newBuilder()
                    .sslContext(sslContext)
                    .sslParameters(sslParams)
                    .build();
        } catch (NoSuchAlgorithmException | KeyManagementException e) {
            throw new RuntimeException(e);
        }
    }

    public VersionResponse getVersion(String versionId) {
        if (versionId == null || versionId.isEmpty()) {
            throw new RuntimeException("Version ID must be provided");
        }

        HttpRequest request = HttpRequest.newBuilder(URI.create(host + "/1/versions/" + versionId))
                .header("Authorization", oauthToken)
                .header("Content-Type", "application/json")
                .header("User-Agent", "AliceYandexVaultJavaClient")
                //.header("Host", host)
                .build();

        try {
            HttpResponse<String> r = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
            return parseResponse(r, versionId, VersionResponse.class);
        } catch (IOException | InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    public Optional<String> getVersionKey(String versionId, String secretKey) {
        VersionResponse versionResponse = getVersion(versionId);
        if (versionResponse.status() == Status.OK) {
            return versionResponse.version()
                    .value()
                    .stream()
                    .filter(it -> secretKey.equals(it.key()))
                    .map(VaultVersionValue::value)
                    .findFirst();
        } else {
            throw new RuntimeException(
                    MessageFormat.format("Fetch version status is {0}", versionResponse.status().getValue())
            );
        }
    }

    private <T> T parseResponse(HttpResponse<String> response, String versionId, Class<T> resultClass) {
        if (SUCCESSFUL_RESPONSE_CODES.contains(response.statusCode())) {
            try {
                return objectMapper.readValue(response.body(), resultClass);
            } catch (IOException e) {
                logger.error("Unable to parse response", e);
                throw new RuntimeException("Unable to parse response", e);
            }
        } else if (response.statusCode() == 401) {
            throw new RuntimeException(MessageFormat.format("Not authorized access to secret version {0}", versionId));
        } else {
            throw new RuntimeException(MessageFormat.format("Do not know how to handle status {0} from vault",
                    response.statusCode()));
        }
    }

    public enum Type {
        PROD("https://vault-api.passport.yandex.net"),
        TESTING("https://vault-api-test.passport.yandex.net");
        private final String host;

        Type(String host) {
            this.host = host;
        }

        public String getHost() {
            return host;
        }
    }
}
