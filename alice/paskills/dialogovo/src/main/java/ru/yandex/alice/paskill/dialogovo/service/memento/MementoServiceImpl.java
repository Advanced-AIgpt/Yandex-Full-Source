package ru.yandex.alice.paskill.dialogovo.service.memento;

import java.net.URI;
import java.util.Optional;

import com.google.protobuf.Any;
import com.google.protobuf.InvalidProtocolBufferException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey;
import ru.yandex.alice.memento.proto.MementoApiProto.TConfigKeyAnyPair;
import ru.yandex.alice.memento.proto.MementoApiProto.TReqChangeUserObjects;
import ru.yandex.alice.memento.proto.MementoApiProto.TReqGetUserObjects;
import ru.yandex.alice.memento.proto.MementoApiProto.TRespGetUserObjects;
import ru.yandex.alice.memento.proto.UserConfigsProto.TNewsConfig;
import ru.yandex.alice.paskill.dialogovo.config.MementoConfig;
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders;
import ru.yandex.alice.protos.data.NewsProvider.TNewsProvider;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.passport.tvmauth.TvmClient;

import static ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders.SERVICE_TICKET_HEADER;
import static ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders.USER_TICKET_HEADER;

class MementoServiceImpl implements MementoService {
    private static final Logger logger = LogManager.getLogger();

    private static final String MEMENTO_TVM_SERVICE_NAME = "memento";

    private final RestTemplate client;
    private final MementoConfig mementoConfig;
    private final MetricRegistry metricRegistry;
    private final TvmClient tvmClient;

    MementoServiceImpl(RestTemplate client,
                       MementoConfig mementoConfig,
                       MetricRegistry metricRegistry,
                       TvmClient tvmClient) {
        this.client = client;
        this.mementoConfig = mementoConfig;
        this.metricRegistry = metricRegistry;
        this.tvmClient = tvmClient;
    }

    @Override
    public void updateNewsProviderSubscription(String tvmUserTicket, NewsProviderSubscription subscription) {
        updateUserSettings(tvmUserTicket, TReqChangeUserObjects
                .newBuilder()
                .addUserConfigs(TConfigKeyAnyPair.newBuilder()
                        .setKey(EConfigKey.CK_NEWS)
                        .setValue(Any.pack(TNewsConfig.newBuilder()
                                .addNewsConfig(TNewsConfig.TNewsConfigProviderPair.newBuilder()
                                        .setNewsConfigType(
                                                TNewsConfig.ENewsConfigType
                                                        .DEFAULT_GENERAL_PROVIDER)
                                        .setNewsProvider(TNewsProvider.newBuilder()
                                                .setNewsSource(subscription.getNewsProviderSlug())
                                                .build()))
                                .build()))
                        .build())
                .build());
    }

    @Override
    public Optional<NewsProviderSubscription> getUserNewsProviderSubscription(String tvmUserTicket) {
        TRespGetUserObjects userSettings;
        try {
            userSettings = getUserSettings(tvmUserTicket,
                    TReqGetUserObjects.newBuilder()
                            .addKeys(EConfigKey.CK_NEWS)
                            .build());
        } catch (Exception ex) {
            logger.error("Error while getting user setting from memento service, return fallback", ex);
            return Optional.empty();
        }

        return userSettings.getUserConfigsList()
                .stream()
                .filter(configPair -> configPair.getKey().equals(EConfigKey.CK_NEWS))
                .map(TConfigKeyAnyPair::getValue)
                .filter(value -> value.is(TNewsConfig.class))
                .map(any -> {
                    try {
                        return Optional.of(any.unpack(TNewsConfig.class));
                    } catch (InvalidProtocolBufferException ex) {
                        logger.error("Got error while parsing user settings value proto ", ex);
                        return Optional.<TNewsConfig>empty();
                    }
                })
                .flatMap(Optional::stream)
                .flatMap(newsConfig -> newsConfig.getNewsConfigList().stream()
                        .filter(newsCfgPair -> newsCfgPair.getNewsConfigType()
                                == TNewsConfig.ENewsConfigType.DEFAULT_GENERAL_PROVIDER)
                        .map(TNewsConfig.TNewsConfigProviderPair::getNewsProvider)
                        .map(tNewsProvider -> new NewsProviderSubscription(
                                tNewsProvider.getNewsSource(),
                                Optional.ofNullable(("".equals(tNewsProvider.getRubric())
                                        ? null
                                        : tNewsProvider.getRubric())),
                                newsConfig.getIsNew()
                        )))
                .findFirst();
    }

    private TRespGetUserObjects getUserSettings(
            String tvmUserTicket,
            TReqGetUserObjects settingsRequest) {
        logger.debug("Start memento getUserSettings request with keys=[{}]", settingsRequest.getKeysList());
        metricRegistry.rate("memento.get_objects.request").inc();

        try {
            var url = UriComponentsBuilder.newInstance()
                    .uri(new URI(mementoConfig.getUrl()))
                    .path("/get_objects")
                    .build()
                    .toUri();

            HttpHeaders headers = new HttpHeaders();

            headers.add(USER_TICKET_HEADER, tvmUserTicket);
            headers.add(SERVICE_TICKET_HEADER,
                    tvmClient.getServiceTicketFor(MEMENTO_TVM_SERVICE_NAME));

            headers.add(HttpHeaders.ACCEPT, "application/protobuf");

            HttpEntity<TReqGetUserObjects> httpEntity = new HttpEntity<>(settingsRequest, headers);

            TRespGetUserObjects tRespGetUserObjects = client.postForObject(url, httpEntity,
                    TRespGetUserObjects.class);

            logger.debug("Memento response: [{}]", () -> tRespGetUserObjects);

            return tRespGetUserObjects;
        } catch (Exception ex) {
            metricRegistry.rate("memento.get_objects.request.fail").inc();
            throw new RuntimeException(ex);
        }
    }

    private TRespGetUserObjects updateUserSettings(String tvmUserTicket,
                                                   TReqChangeUserObjects changeRequest) {
        logger.debug("Start memento updateUserSettings request with keys=[{}]", changeRequest.getUserConfigsList());
        metricRegistry.rate("memento.update_objects.request").inc();

        try {
            var url = UriComponentsBuilder.newInstance()
                    .uri(new URI(mementoConfig.getUrl()))
                    .path("/update_objects")
                    .build()
                    .toUri();

            HttpHeaders headers = new HttpHeaders();

            headers.add(SecurityHeaders.USER_TICKET_HEADER, tvmUserTicket);
            headers.add(SecurityHeaders.SERVICE_TICKET_HEADER,
                    tvmClient.getServiceTicketFor(MEMENTO_TVM_SERVICE_NAME));
            headers.add(HttpHeaders.ACCEPT, "application/protobuf");

            HttpEntity<TReqChangeUserObjects> httpEntity = new HttpEntity<>(changeRequest, headers);

            TRespGetUserObjects response = client.postForObject(url, httpEntity,
                    TRespGetUserObjects.class);

            logger.debug("Memento response: [{}]", () -> response);

            return response;

        } catch (Exception ex) {
            logger.error("Error while updating objects on memento service", ex);
            metricRegistry.rate("memento.update_objects.request.fail").inc();
            throw new RuntimeException(ex);
        }
    }
}
