package ru.yandex.alice.paskills.my_alice.blocks.station;

import java.util.List;
import java.util.Optional;

import NAppHostHttp.Http;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpHeaders;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponents;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskills.my_alice.apphost.http.ApphostHttp;
import ru.yandex.alice.paskills.my_alice.apphost.http.HttpHeaderName;
import ru.yandex.alice.paskills.my_alice.apphost.http.Method;
import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.alice.paskills.my_alice.blocks.Block;
import ru.yandex.alice.paskills.my_alice.blocks.BlockType;
import ru.yandex.alice.paskills.my_alice.blocks.SingleBlockWithRequest;
import ru.yandex.alice.paskills.my_alice.layout.Card;
import ru.yandex.alice.paskills.my_alice.layout.ListGroup;
import ru.yandex.web.apphost.api.request.RequestContext;

@Component
public class StationPromo implements SingleBlockWithRequest {

    private static final Logger logger = LogManager.getLogger();
    private final ApphostHttp apphostHttp;
    private final ObjectMapper objectMapper;

    private static final Block STATION_BLOCK = new Block(BlockType.STATION, new ListGroup(
            null,
            List.of(
                    new Card(
                            Card.Kind.STATION,
                            "Купить Яндекс Станцию",
                            "Кажется, у вас нет колонки...",
                            "Умная колонка с Алисой внутри",
                            "https://avatars.mds.yandex.net/get-dialogs/1530877/5f05958a4807117c4c09/orig",
                            null,
                            null
                    )
            ),
            null
    ));

    private static final Block STATION_MINI_BLOCK = new Block(BlockType.STATION, new ListGroup(
            null,
            List.of(
                    new Card(
                            Card.Kind.STATION_MINI,
                            "Купить Яндекс Станцию.Мини",
                            "Кажется, у вас нет колонки...",
                            "Умная колонка с Алисой внутри",
                            "https://avatars.mds.yandex.net/get-dialogs/1676983/99b4d835c80744970133/orig",
                            null,
                            null
                    )
            ),
            null
    ));


    public StationPromo(ApphostHttp apphostHttp, ObjectMapper objectMapper) {
        this.apphostHttp = apphostHttp;
        this.objectMapper = objectMapper;
    }

    @Override
    public Optional<Http.THttpRequest> prepare(RequestContext context, SessionId.Response blackboxResponse) {
        if (blackboxResponse.getTvmTicket() != null) {
            HttpHeaders headers = new HttpHeaders();
            headers.add(HttpHeaderName.USER_TICKET, blackboxResponse.getTvmTicket());
            headers.add(HttpHeaderName.REQUEST_ID, context.getGuid());
            UriComponents uriComponents = UriComponentsBuilder.newInstance()
                    .path("/v1.0/user/info")
                    .build();
            return Optional.of(apphostHttp.buildRequest(Method.GET, uriComponents, headers));
        } else {
            return Optional.empty();
        }
    }

    @Override
    public String getRequestContextKey() {
        return "iot_http_request";
    }

    @Override
    public Optional<Block> render(RequestContext context, SessionId.Response blackboxResponse) {
        if (blackboxResponse.getUser() == null) {
            return Optional.of(STATION_BLOCK);
        }
        Response response;
        try {
            response = parseHttpResponse(context, "iot_http_response", Response.class, objectMapper);
            logger.debug("IOT response: {}", response);
        } catch (Exception e) {
            logger.error("Failed to parse http response", e);
            return Optional.of(STATION_BLOCK);
        }
        if (!response.hasStation()) {
            logger.info("Showing station block");
            return Optional.of(STATION_BLOCK);
        } else if (!response.hasStationMini()) {
            logger.info("Showing station mini block");
            return Optional.of(STATION_MINI_BLOCK);
        } else {
            logger.info("Not showing any station blocks: user already has all smart speakers");
            return Optional.empty();
        }
    }

    @Override
    public boolean isEnabled() {
        return true;
    }
}
