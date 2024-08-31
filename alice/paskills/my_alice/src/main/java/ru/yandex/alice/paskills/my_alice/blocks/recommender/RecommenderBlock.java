package ru.yandex.alice.paskills.my_alice.blocks.recommender;

import java.time.Instant;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.List;
import java.util.Optional;

import NAppHostHttp.Http;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskills.my_alice.apphost.http.ApphostHttp;
import ru.yandex.alice.paskills.my_alice.apphost.http.Method;
import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.alice.paskills.my_alice.blocks.Block;
import ru.yandex.alice.paskills.my_alice.blocks.BlockType;
import ru.yandex.alice.paskills.my_alice.blocks.SingleBlockWithRequest;
import ru.yandex.alice.paskills.my_alice.geobase.GeoBase;
import ru.yandex.alice.paskills.my_alice.layout.ListGroup;
import ru.yandex.geobase6.Timezone;
import ru.yandex.web.apphost.api.request.RequestContext;

@Component
public class RecommenderBlock implements SingleBlockWithRequest {

    private static final Logger logger = LogManager.getLogger();

    private final ApphostHttp apphostHttp;
    private final ObjectMapper objectMapper;
    private final GeoBase geoBase;

    public RecommenderBlock(
            ApphostHttp apphostHttp,
            ObjectMapper objectMapper,
            GeoBase geoBase) {
        this.apphostHttp = apphostHttp;
        this.objectMapper = objectMapper;
        this.geoBase = geoBase;
    }

    @Override
    public Optional<Block> render(RequestContext context, SessionId.Response blackboxResponse) {
        RecommenderResponse recommenderResponse;
        try {
            recommenderResponse = parseHttpResponse(
                    context,
                    "skills_rec_http_response",
                    RecommenderResponse.class,
                    objectMapper);
        } catch (Exception e) {
            logger.error("Failed to parse http response", e);
            return Optional.empty();
        }
        return recommenderResponse.getItems().stream()
                .findFirst()
                .flatMap(RecommenderItem::toCard)
                .map(card -> new ListGroup(null, List.of(card), null))
                .map(group -> new Block(BlockType.RECOMMENDER, group));
    }


    @Override
    public Optional<Http.THttpRequest> prepare(RequestContext context, SessionId.Response blackboxResponse) {
        UriComponentsBuilder uriComponents = UriComponentsBuilder.newInstance();
        uriComponents.path("/recommender");
        uriComponents.queryParam("client", "my_alice");
        uriComponents.queryParam("card_name", "get_greetings");
        uriComponents.queryParam("data_experiment", "my_alice");
        uriComponents.queryParam("experiment", "editorial");
        Timezone timezone = geoBase.timezone(context);
        ZonedDateTime now = Instant.now().atZone(ZoneId.of(timezone.getName()));
        uriComponents.queryParam("timetz", now.toEpochSecond() + "@" + now.getZone().getId());
        return Optional.of(apphostHttp.buildRequest(Method.GET, uriComponents.build()));
    }

    @Override
    public String getRequestContextKey() {
        return "skills_rec_http_request";
    }

    @Override
    public boolean isEnabled() {
        return true;
    }
}
