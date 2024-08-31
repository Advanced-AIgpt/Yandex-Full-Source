package ru.yandex.alice.paskills.my_alice.blocks.music;

import java.time.OffsetDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import NAppHostHttp.Http;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.base.Strings;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.HttpHeaders;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskills.common.vcs.VcsUtils;
import ru.yandex.alice.paskills.my_alice.apphost.http.ApphostHttp;
import ru.yandex.alice.paskills.my_alice.apphost.http.HttpHeaderName;
import ru.yandex.alice.paskills.my_alice.apphost.http.Method;
import ru.yandex.alice.paskills.my_alice.apphost.request_init.Request;
import ru.yandex.alice.paskills.my_alice.apphost.request_init.RequestInit;
import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.alice.paskills.my_alice.blocks.Block;
import ru.yandex.alice.paskills.my_alice.blocks.BlockType;
import ru.yandex.alice.paskills.my_alice.blocks.SingleBlockWithRequest;
import ru.yandex.alice.paskills.my_alice.layout.ExpandableListGroup;
import ru.yandex.alice.paskills.my_alice.layout.PageLayout;
import ru.yandex.web.apphost.api.request.RequestContext;

@Component
public class Music implements SingleBlockWithRequest {

    private static final Logger logger = LogManager.getLogger();

    private final ApphostHttp apphostHttp;
    private final String musicRequestClient;
    private final RequestInit requestInit;
    private final ObjectMapper objectMapper;
    private final boolean isEnabled;

    public Music(
            ApphostHttp apphostHttp,
            RequestInit requestInit,
            ObjectMapper objectMapper,
            @Value("${block.music.enabled}") boolean isEnabled
    ) {
        this.apphostHttp = apphostHttp;
        this.requestInit = requestInit;
        this.objectMapper = objectMapper;
        this.musicRequestClient = "MyAlice/" + VcsUtils.INSTANCE.getVersion();
        this.isEnabled = isEnabled;
    }

    @Override
    public Optional<Http.THttpRequest> prepare(RequestContext context, SessionId.Response blackboxResponse) {
        var httpHeaders = new HttpHeaders();

        httpHeaders.add("X-Yandex-Music-Client", musicRequestClient);
        httpHeaders.add("X-Yandex-Music-Client-Now", DateTimeFormatter.ISO_DATE_TIME.format(OffsetDateTime.now()));

        String userTicket = blackboxResponse.getTvmTicket();
        if (!Strings.isNullOrEmpty(userTicket)) {
            httpHeaders.add(HttpHeaderName.USER_TICKET, userTicket);
        }


        UriComponentsBuilder uriComponents = UriComponentsBuilder.newInstance();
        uriComponents.path("/internal-api/landing3");
        uriComponents.queryParam("blocks", "new-releases,new-playlists,mixes,chart");

        Optional<String> yandexUidCookie = tryParseYandexUid(context);
        if (blackboxResponse.getUser() != null) {
            uriComponents.queryParam("__uid", blackboxResponse.getUser().getUid());
        } else if (yandexUidCookie.isPresent()) {
            uriComponents.queryParam("yandexuid", yandexUidCookie.get());
        } else {
            logger.error("Failed to parse both uid and yandexuid from request");
            return Optional.empty();
        }

        return Optional.of(apphostHttp.buildRequest(
                Method.GET,
                uriComponents.build(),
                httpHeaders
        ));
    }

    @Override
    public String getRequestContextKey() {
        return "music_http_request";
    }

    private Optional<String> tryParseYandexUid(RequestContext context) {
        Optional<Request> request = requestInit.getRequest(context);
        if (request.isPresent()) {
            return Optional.ofNullable(request.get().getCookiesParsed().get("yandexuid"));
        } else {
            return Optional.empty();
        }
    }

    @Override
    public Optional<Block> render(RequestContext context, SessionId.Response blackboxResponse) {
        MusicLandingResponse musicResponse;
        try {
            musicResponse = parseHttpResponse(
                    context,
                    "music_http_response",
                    MusicLandingResponse.class,
                    objectMapper);
        } catch (Exception e) {
            logger.error("Failed to parse http response", e);
            return Optional.empty();
        }
        long unknownBlockCount = musicResponse.getResult().getBlocks()
                .stream()
                .flatMap(b -> b.getEntities().stream())
                .filter(e -> e instanceof UnknownBlockEntity)
                .count();
        logger.debug("Found {} unknown blocks in music response", unknownBlockCount);

        List<PageLayout.Card> blockEntities = musicResponse.pickNext(Collections.emptySet())
                .stream()
                .map(BlockEntity::toImageCard)
                .collect(Collectors.toList());
        return Optional.of(new Block(BlockType.MUSIC, ExpandableListGroup.create("Послушать", blockEntities)));
    }

    @Override
    public boolean isEnabled() {
        return isEnabled;
    }
}
