package ru.yandex.alice.kronstadt.core.utils

import com.fasterxml.jackson.databind.annotation.JsonDeserialize
import com.fasterxml.jackson.databind.annotation.JsonSerialize
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.boot.test.autoconfigure.json.JsonTest
import org.springframework.boot.test.json.JacksonTester
import org.springframework.boot.test.json.JsonContent
import ru.yandex.alice.megamind.protos.common.FrameProto.TPutMoneyOnPhoneSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame

@JsonTest
class ActivationTSFJacksonSerializerDeserializerTest {

    @Autowired
    private lateinit var jsonTester: JacksonTester<DummyWithTSF>

    @Test
    fun testPutMoneyOnPhone2WayConversion() {
        val putMoneyOnPhoneSTF = TTypedSemanticFrame.newBuilder()
            .mergePutMoneyOnPhoneSemanticFrame(TPutMoneyOnPhoneSemanticFrame.newBuilder().build()).build()
        val dummy = DummyWithTSF(putMoneyOnPhoneSTF)

        val json: JsonContent<DummyWithTSF> = jsonTester.write(dummy)

        assertEquals(
            dummy,
            jsonTester.parseObject(json.json)
        )
    }

    data class DummyWithTSF(
        @JsonDeserialize(using = ActivationTypedSemanticFrameFromBase64StringDeserializer::class)
        @JsonSerialize(using = ActivationTypedSemanticFrameToBase64StringSerializer::class)
        val activationTypedSemanticFrame: TTypedSemanticFrame
    )
}
