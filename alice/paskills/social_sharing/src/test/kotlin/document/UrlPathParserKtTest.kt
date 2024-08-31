package ru.yandex.alice.social.sharing.document

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.params.ParameterizedTest
import org.junit.jupiter.params.provider.Arguments
import org.junit.jupiter.params.provider.MethodSource

internal class UrlPathParserKtTest {

    @ParameterizedTest
    @MethodSource("testData")
    fun testParseLinkId(input: String, expected: String?) {
        assertEquals(expected, parseLinkId(input))
    }

    companion object {
        @JvmStatic
        fun testData(): List<Arguments> = listOf(
            Arguments.of("/get_page/id", "id"),
            Arguments.of("/get_page", null),
            Arguments.of("/get_page/", null),
            Arguments.of("/get_page/link_id", "link_id"),
            Arguments.of("/get_page/link_id?a=b", "link_id"),
        )
    }
}
