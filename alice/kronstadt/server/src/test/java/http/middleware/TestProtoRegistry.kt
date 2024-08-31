package ru.yandex.alice.kronstadt.server.http.middleware

import com.google.protobuf.util.JsonFormat
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.boot.test.context.SpringBootTest
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto
import ru.yandex.alice.kronstadt.server.http.TestApp

@SpringBootTest(
    classes = [TestApp::class],
    properties = ["tvm.mode=disabled"]
)
internal class TestProtoRegistry {
    @Autowired
    private lateinit var registry: JsonFormat.TypeRegistry

    @Test
    internal fun name() {
        assertEquals(
            ApplyArgsProto.TSceneArguments.getDescriptor(),
            registry.find(ApplyArgsProto.TSceneArguments.getDescriptor().fullName)
        )
    }
}
