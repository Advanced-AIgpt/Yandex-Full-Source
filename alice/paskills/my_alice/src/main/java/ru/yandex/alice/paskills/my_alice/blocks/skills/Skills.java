package ru.yandex.alice.paskills.my_alice.blocks.skills;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import NAppHostHttp.Http;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponents;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskills.my_alice.apphost.http.ApphostHttp;
import ru.yandex.alice.paskills.my_alice.apphost.http.Method;
import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.alice.paskills.my_alice.blocks.Block;
import ru.yandex.alice.paskills.my_alice.blocks.BlockType;
import ru.yandex.alice.paskills.my_alice.blocks.MultipleBlocksWithRequest;
import ru.yandex.alice.paskills.my_alice.layout.CarouselGroup;
import ru.yandex.alice.paskills.my_alice.layout.PageLayout;
import ru.yandex.web.apphost.api.request.RequestContext;

import static ru.yandex.alice.paskills.my_alice.blocks.skills.SkillTag.GAME;
import static ru.yandex.alice.paskills.my_alice.blocks.skills.SkillTag.NEW;

@Component
public class Skills implements MultipleBlocksWithRequest {

    private static final Logger logger = LogManager.getLogger();

    private final List<SkillTag> tags = List.of(GAME, NEW);
    private final String tagsJoined = tags.stream().map(SkillTag::getValue).collect(Collectors.joining(","));

    private final ApphostHttp apphostHttp;
    private final ObjectMapper objectMapper;

    public Skills(ApphostHttp apphostHttp, ObjectMapper objectMapper) {
        this.apphostHttp = apphostHttp;
        this.objectMapper = objectMapper;
    }

    @Override
    public Optional<Http.THttpRequest> prepare(RequestContext context, SessionId.Response blackboxResponse) {
        UriComponents uriComponents = UriComponentsBuilder.newInstance()
                .path("/api/external/v2/skills")
                .queryParam("tags", tagsJoined)
                .build();

        return Optional.of(apphostHttp.buildRequest(Method.GET, uriComponents));
    }

    @Override
    public String getRequestContextKey() {
        return "skills_api_http_request";
    }

    @Override
    public List<Block> render(RequestContext context, SessionId.Response blackboxResponse) {
        Response response;
        try {
            response = parseHttpResponse(context, "skills_api_http_response", Response.class, objectMapper);
        } catch (Exception e) {
            // TODO: add metrics, move try/catch block to interface/abstract class (MYALICE-26)
            // TODO: log failed response body
            logger.error("Failed to parse http response", e);
            return Collections.emptyList();
        }
        List<Block> blocks = new ArrayList<>();
        buildBlock(BlockType.SKILLS_GAME, response, "Поиграть", GAME, 10).ifPresent(blocks::add);
        buildBlock(BlockType.SKILLS_NEW, response, "Узнать новое", NEW, 10).ifPresent(blocks::add);
        return blocks;
    }

    private Optional<Block> buildBlock(BlockType block, Response response, String title, SkillTag tag, int count) {
        List<SkillInfo> skills = response.skillsWithTag(tag);
        if (skills.isEmpty()) {
            return Optional.empty();
        }
        Collections.shuffle(skills);
        List<PageLayout.Card> cards = skills.stream()
                .limit(count)
                .map(SkillInfo::toSkillCard)
                .collect(Collectors.toList());
        return Optional.of(new Block(block, new CarouselGroup(title, cards)));
    }

    @Override
    public boolean isEnabled() {
        return true;
    }

    @Data
    private static class Response {
        private final List<SkillInfo> skills;

        public List<SkillInfo> skillsWithTag(SkillTag tag) {
            return skills
                    .stream()
                    .filter(s -> s.getTags().stream().anyMatch(skillTag -> tag.getValue().equals(skillTag)))
                    .collect(Collectors.toList());
        }
    }

}
