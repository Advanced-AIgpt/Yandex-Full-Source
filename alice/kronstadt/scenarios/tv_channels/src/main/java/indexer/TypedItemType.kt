package ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer

import com.fasterxml.jackson.annotation.JsonValue

// https://doc.yandex-team.ru/Search/saas/saas-overview/concepts/json-doc.html#json-doc__js-types-tab
enum class TypedItemType(@JsonValue val code: String) {
    SEARCH_ATTR_LITERAL("l"),
    GROUPING_ATTR("g"),
    INT_ATTR("i"),
    PROPERTY("p"),
    ZONE("z");
}
