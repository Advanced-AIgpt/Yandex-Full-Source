package ru.yandex.quasar.billing.providers.universal;

import java.time.Duration;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Executors;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.boot.web.server.LocalServerPort;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.EpisodeProviderContentItem;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.beans.SeasonProviderContentItem;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.ProviderConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.config.UniversalProviderConfig;
import ru.yandex.quasar.billing.providers.amediateka.TestAmediatekaIntegration;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.UnistatService;

@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        classes = {TestConfigProvider.class})
@ExtendWith(EmbeddedPostgresExtension.class)
class UniversalProviderAmediatekaBasedTest extends TestAmediatekaIntegration {

    @LocalServerPort
    private int port;

    @Autowired
    private BillingConfig config;

    @Autowired
    private RestTemplateBuilder restTemplate;
    @Autowired
    private AuthorizationContext authorizationContext;
    @Autowired
    private UnistatService unistatService;

    @BeforeEach
    @Override
    public void setUp() {
        super.setUp();
        ProviderConfig providerConfig = config.getProvidersConfig().get("amediateka");
        UniversalProviderConfig universalProviderConfig = config.getUniversalProviders().get("amediateka");
        super.amediatekaContentProvider = new UniversalProvider("amediatekaUP",
                providerConfig.getSocialAPIServiceName(),
                providerConfig.getSocialClientId(),
                config.getAmediatekaConfig().getClientId(),
                "23b0df5a-0f35-4955-9b8b-104e6d1a7b90",
                "http://localhost:" + port + "/provider/amediateka/",
                restTemplate
                        .setConnectTimeout(Duration.ofMillis(universalProviderConfig.getConnectionTimeoutMs()))
                        .setReadTimeout(Duration.ofMillis(universalProviderConfig.getReadTimeoutMs())),
                authorizationContext,
                Executors.newSingleThreadExecutor(),
                unistatService) {

            @Override
            ProviderContentItem toProviderItem(String contentItemId) {
                String[] strings = contentItemId.split(":");
                Map<String, String> map = new HashMap<>();
                for (int i = 0; i < strings.length; i += 2) {
                    map.put(strings[i], strings[i + 1]);
                }

                if (map.containsKey(ContentType.EPISODE.getUniversalProviderName())) {
                    return ProviderContentItem.createEpisode(map.get(ContentType.EPISODE.getUniversalProviderName()),
                            map.get(ContentType.SEASON.getUniversalProviderName()),
                            map.get(ContentType.TV_SHOW.getUniversalProviderName()));
                } else if (map.containsKey(ContentType.SEASON.getUniversalProviderName())) {
                    return ProviderContentItem.createSeason(map.get(ContentType.SEASON.getUniversalProviderName()),
                            map.get(ContentType.TV_SHOW.getUniversalProviderName()));
                } else {
                    ContentType type = ContentType.forUniversalProviderName(strings[0]);
                    if (type == null) {
                        throw new IllegalArgumentException("Undefined content type: " + strings[0]);
                    }
                    return ProviderContentItem.create(type, strings[1]);
                }
            }

            @Override
            String toContentId(ProviderContentItem providerContentItem) {
                StringBuilder buf = new StringBuilder();

                if (providerContentItem instanceof EpisodeProviderContentItem) {
                    EpisodeProviderContentItem seasonProviderContentItem =
                            (EpisodeProviderContentItem) providerContentItem;
                    buf.append(ContentType.TV_SHOW.getUniversalProviderName()).append(":")
                            .append(seasonProviderContentItem.getTvShowId()).append(":");
                    buf.append(ContentType.SEASON.getUniversalProviderName()).append(":")
                            .append(seasonProviderContentItem.getSeasonId()).append(":");
                } else if (providerContentItem instanceof SeasonProviderContentItem) {
                    SeasonProviderContentItem seasonProviderContentItem =
                            (SeasonProviderContentItem) providerContentItem;
                    buf.append(ContentType.TV_SHOW.getUniversalProviderName()).append(":")
                            .append(seasonProviderContentItem.getTvShowId()).append(":");
                }

                buf.append(providerContentItem.getContentType().getUniversalProviderName()).append(":")
                        .append(providerContentItem.getId());

                return buf.toString();
            }
        };
    }

}
