package ru.yandex.alice.library.protobufutils

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test

class FromMessageConverterTest {

    private val converter = FromMessageConverter()

    @Test
    internal fun toStructConversionTest() {
        assertEquals(
            CommonTestBenchData.TEST_DATA_STRUCT_WITHOUT_NULLS,
            converter.convertToStruct(CommonTestBenchData.TEST_DATA_PROTO)
        )
    }

    @Test
    internal fun toMapTest() {
        assertEquals(
            CommonTestBenchData.TEST_DATA_MAP_WITHOUT_NULLS,
            converter.convertToMap(CommonTestBenchData.TEST_DATA_PROTO)
        )
    }

    @Test
    internal fun toObjectNodeTest() {
        assertEquals(
            CommonTestBenchData.TEST_DATA_OBJECT_NODE_WITHOUT_NULLS,
            converter.convertToObjectNode(CommonTestBenchData.TEST_DATA_PROTO)
        )
    }
}
