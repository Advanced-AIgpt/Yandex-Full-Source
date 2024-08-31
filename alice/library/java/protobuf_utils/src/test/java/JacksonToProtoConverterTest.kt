package ru.yandex.alice.library.protobufutils

import com.fasterxml.jackson.databind.DeserializationFeature
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import com.google.protobuf.Struct
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import ru.yandex.alice.library.protobufutils.CommonTestBenchData.TEST_DATA
import ru.yandex.alice.library.protobufutils.CommonTestBenchData.TEST_DATA_MAP
import ru.yandex.alice.library.protobufutils.CommonTestBenchData.TEST_DATA_STRUCT
import ru.yandex.alice.library.protobufutils.CommonTestBenchData.TestData

internal class JacksonToProtoConverterTest {

    private val mapper = jacksonObjectMapper().copy()
        .disable(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES)
    private val jacksonToProtoConverter = JacksonToProtoConverter(mapper)
    private val expected = TEST_DATA_STRUCT

    @Test
    fun objectToStructViaStringTest() {
        val actual: Struct = jacksonToProtoConverter.objectToStructViaString(TEST_DATA)
        assertEquals(expected, actual)
    }

    @Test
    fun objectToStructViaTreeTest() {
        val actual: Struct = jacksonToProtoConverter.objectToStructViaTree(TEST_DATA)
        assertEquals(expected, actual)
    }

    @Test
    fun objectToStructTest() {
        val actual: Struct = jacksonToProtoConverter.objectToStruct(TEST_DATA)
        assertEquals(expected, actual)
    }

    @Test
    fun objectToStructForNullTest() {
        val actual: Struct = jacksonToProtoConverter.objectToStruct(null)
        assertEquals(Struct.getDefaultInstance(), actual)
    }

    @Test
    fun objectToMapDirectTest() {
        assertEquals(TEST_DATA_MAP, jacksonToProtoConverter.structToMapDirect(TEST_DATA_STRUCT))
    }

    @Test
    fun structToMapViaObjectMapperTest() {
        assertEquals(TEST_DATA_MAP, jacksonToProtoConverter.structToMapViaObjectMapper(TEST_DATA_STRUCT))
    }

    @Test
    fun structToMapTest() {
        assertEquals(TEST_DATA_MAP, jacksonToProtoConverter.structToMap(TEST_DATA_STRUCT))
    }

    @Test
    fun jsonStringToStructTest() {
        assertEquals(TEST_DATA_STRUCT, jacksonToProtoConverter.jsonStringToStruct(mapper.writeValueAsString(TEST_DATA)))
    }

    @Test
    fun strutToObjectTest() {
        assertEquals(TEST_DATA, jacksonToProtoConverter.structToObject<TestData>(TEST_DATA_STRUCT))
    }
}
