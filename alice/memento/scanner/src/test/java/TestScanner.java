package ru.yandex.alice.memento.scanner;

import java.io.IOException;

import com.google.protobuf.Any;
import org.junit.jupiter.api.Test;

import ru.yandex.alice.memento.proto.MementoApiProto;
import ru.yandex.alice.memento.proto.UserConfigsProto.TConfigForTests;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class TestScanner {

    private final KeyMappingScanner scanner;

    public TestScanner() throws IOException {
        scanner = new KeyMappingScanner();
    }

    @Test
    void testDefaultMethod() {
        var defaultValue = scanner.getDefaultForKey(MementoApiProto.EConfigKey.CK_CONFIG_FOR_TESTS);

        assertEquals(Any.pack(TConfigForTests.newBuilder().setDefaultSource("a").build()), defaultValue);
    }

    @Test
    void testClassForKey() {
        assertEquals(TConfigForTests.class,
                scanner.getClassForKey(MementoApiProto.EConfigKey.CK_CONFIG_FOR_TESTS));
    }

    @Test
    void testDbKey() {
        assertEquals("config_for_tests", scanner.getDbKey(MementoApiProto.EConfigKey.CK_CONFIG_FOR_TESTS));
    }

    @Test
    void testTtypeUrlForEnumypeUrlForEnum() {
        assertEquals("type.googleapis.com/ru.yandex.alice.memento.proto.TConfigForTests",
                scanner.typeUrlForEnum(MementoApiProto.EConfigKey.CK_CONFIG_FOR_TESTS));
    }

}
