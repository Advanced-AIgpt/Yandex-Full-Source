package ru.yandex.alice.paskill.dialogovo.scenarios.news.service;

import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;

import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;

import static org.junit.jupiter.api.Assertions.assertEquals;

class NewsContentsResolverTest {

    private final NewsContentsResolver newsContentsResolver = new NewsContentsResolver();

    @Test
    void findNewsContentOnActivate() {
        assertEqualsById(
                newsContentsResolver.findNextOne(nCWithIds(1), Optional.empty(), 5),
                ids(1));

        assertEqualsById(
                newsContentsResolver.findNextOne(nCWithIds(1, 2, 3, 4, 5, 6), Optional.empty(), 5),
                ids(5));

        assertEqualsById(
                newsContentsResolver.findNextOne(nCWithIds(), Optional.empty(), 5),
                ids());

        assertEqualsById(
                newsContentsResolver.findNextOne(nCWithIds(1, 2, 3), Optional.of("2"), 5),
                ids(1));

        assertEqualsById(
                newsContentsResolver.findNextOne(nCWithIds(1, 2, 3, 4, 5, 6), Optional.of("10"), 5),
                ids(5));

        assertEqualsById(
                newsContentsResolver.findNextOne(nCWithIds(1, 2, 3, 4, 5, 6), Optional.of("1"), 5),
                ids());
    }

    @Test
    void findPrevOne() {
        assertEqualsById(
                newsContentsResolver.findPrevOne(nCWithIds(1), "1"),
                ids());

        assertEqualsById(
                newsContentsResolver.findPrevOne(nCWithIds(1, 2, 3, 4, 5, 6), "5"),
                ids(6));

        assertEqualsById(
                newsContentsResolver.findPrevOne(nCWithIds(1, 2, 3, 4, 5, 6), "7"),
                ids());
    }

    private void assertEqualsById(Optional<NewsContent> newsContents, List<String> ids) {
        assertEquals(ids, newsContents.stream().map(NewsContent::getId).collect(Collectors.toList()));
    }

    private List<NewsContent> nCWithIds(int... ids) {
        return Arrays.stream(ids).mapToObj(this::withId).collect(Collectors.toList());
    }

    private List<String> ids(int... ids) {
        return Arrays.stream(ids).mapToObj(id -> id + "").collect(Collectors.toList());
    }

    private NewsContent withId(int id) {
        return new NewsContent(id + "", null, null, null, null, null, null,
                null, null, null, null);
    }
}
