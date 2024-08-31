package ru.yandex.alice.social.sharing.document

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Test
import ru.yandex.alice.social.sharing.apphost.handlers.document.ydb.isValidDocumentId

class DocumentIdTest {

    @Test
    fun testValidDocumentId() {
        Assertions.assertTrue(isValidDocumentId("AAAAAAAAAAAAAA"))
    }

    @Test
    fun testInvalidDocumentId() {
        Assertions.assertFalse(isValidDocumentId("BBB"))
        Assertions.assertFalse(isValidDocumentId("123"))
        Assertions.assertFalse(isValidDocumentId("AAAAAAAAAAAAAA?"))
    }
}
