package ru.yandex.alice.memento.proto;

import java.util.List;
import java.util.stream.Collectors;

import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import org.hamcrest.Matchers;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;

import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey;
import ru.yandex.alice.memento.proto.MementoApiProto.EDeviceConfigKey;
import ru.yandex.alice.memento.proto.MementoApiProto.TSurfaceConfig;
import ru.yandex.alice.memento.proto.MementoApiProto.TUserConfigs;

import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.toList;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.junit.jupiter.api.Assertions.assertTrue;

class TestApiProto {

    @DisplayName("Проверка пакетов у полей TUserConfigs")
    @ParameterizedTest
    @MethodSource("userConfigFields")
    void checkUserConfigPackage(FieldDescriptor f) {
        assertThat("Тип поля TUserConfigs." + f.getName() + " " + f.getMessageType().getName() +
                        " должен располагаться в пакете ru.yandex.alice.memento.proto или ru.yandex.alice.protos.data",
                f.getMessageType().getFile().getOptions().getJavaPackage(),
                Matchers.anyOf(
                        Matchers.equalTo("ru.yandex.alice.memento.proto"),
                        Matchers.equalTo("ru.yandex.alice.protos.data"),
                        Matchers.startsWith("ru.yandex.alice.protos.data.")
                ));
    }

    @DisplayName("Проверка аннотированности полей TUserConfigs")
    @ParameterizedTest
    @MethodSource("userConfigFields")
    void checkUserConfigFieldsAnnotatedWithEnum(FieldDescriptor f) {
        assertTrue(f.getOptions().hasExtension(MementoApiProto.key),
                "Поле TUserConfigs." + f.getName() + " должно быть аннотировано [(key) = <значение EConfigKey>]");
    }

    @DisplayName("Проверка пакетов у полей TSurfaceConfig")
    @ParameterizedTest
    @MethodSource("surfaceConfigFields")
    void checkDeviceConfigFieldsPackage(FieldDescriptor f) {
        assertThat("Тип поля TSurfaceConfig." + f.getName() + " " + f.getMessageType().getName() +
                        " должен располагаться в пакете ru.yandex.alice.memento.proto или ru.yandex.alice.protos.data",
                f.getMessageType().getFile().getOptions().getJavaPackage(),
                Matchers.anyOf(
                        Matchers.equalTo("ru.yandex.alice.memento.proto"),
                        Matchers.equalTo("ru.yandex.alice.protos.data"),
                        Matchers.startsWith("ru.yandex.alice.protos.data.")
                ));
    }

    @DisplayName("Проверка аннотированности полей TSurfaceConfig")
    @ParameterizedTest
    @MethodSource("surfaceConfigFields")
    void checkSurfaceConfigFieldsAnnotatedWithEnum(FieldDescriptor f) {
        assertTrue(f.getOptions().hasExtension(MementoApiProto.surfaceKey),
                "Поле TUserConfigs." + f.getName() +
                        " должно быть аннотировано [(surface_key) = <значение EDeviceConfigKey>]");
    }

    @DisplayName("Проверка дубликатов по ключам среди полей TUserConfigs")
    @Test
    void checkUniqueUserConfigKeys() {
        userConfigFields().stream()
                .filter(f -> f.getOptions().hasExtension(MementoApiProto.key))
                .collect(groupingBy(f -> f.getOptions().getExtension(MementoApiProto.key), toList()))
                .entrySet()
                .stream()
                .filter(e -> e.getValue().size() > 1)
                .forEach(e -> Assertions.fail("Ключ " + e.getKey().name() + " используется для нескольких полей: " +
                        e.getValue().stream().map(FieldDescriptor::getName).collect(Collectors.joining(", ")) + ", а " +
                        "должен только для какого-то одного."));

    }

    @DisplayName("Проверка дубликатов по ключам среди полей TSurfaceConfig")
    @Test
    void checkUniqueSurfaceConfigKeys() {
        surfaceConfigFields().stream()
                .filter(f -> f.getOptions().hasExtension(MementoApiProto.surfaceKey))
                .collect(groupingBy(f -> f.getOptions().getExtension(MementoApiProto.surfaceKey), toList()))
                .entrySet()
                .stream()
                .filter(e -> e.getValue().size() > 1)
                .forEach(e -> Assertions.fail("Ключ " + e.getKey().name() + " используется для нескольких полей: " +
                        e.getValue().stream().map(FieldDescriptor::getName).collect(Collectors.joining(", ")) + ", а " +
                        "должен только для какого-то одного."));

    }

    @DisplayName("Проверка дубликатов ключей БД в EConfigKey")
    @Test
    void checkConfigKeyStringDuplicates() {
        EConfigKey.getDescriptor().getValues().stream()
                .filter(f -> f.getOptions().hasExtension(MementoApiProto.dbKey))
                .collect(groupingBy(f -> f.getOptions().getExtension(MementoApiProto.dbKey), toList()))
                .entrySet()
                .stream()
                .filter(e -> e.getValue().size() > 1)
                .forEach(e -> Assertions.fail("Ключ " + e.getKey() + " используется для нескольких элементах " +
                        "EConfigKey: " +
                        e.getValue().stream().map(EnumValueDescriptor::getName).collect(Collectors.joining(", ")) +
                        ", а должен только для какого-то одного."));
    }

    @DisplayName("Проверка дубликатов ключей БД в EDeviceConfigKey")
    @Test
    void checkDeviceConfigKeyStringDuplicates() {
        EDeviceConfigKey.getDescriptor().getValues().stream()
                .filter(f -> f.getOptions().hasExtension(MementoApiProto.dbKey))
                .collect(groupingBy(f -> f.getOptions().getExtension(MementoApiProto.dbKey), toList()))
                .entrySet()
                .stream()
                .filter(e -> e.getValue().size() > 1)
                .forEach(e -> Assertions.fail("Ключ " + e.getKey() + " используется для нескольких элементах " +
                        "EDeviceConfigKey: " +
                        e.getValue().stream().map(EnumValueDescriptor::getName).collect(Collectors.joining(", ")) +
                        ", а должен только для какого-то одного."));
    }

    private static List<FieldDescriptor> userConfigFields() {
        return TUserConfigs.getDescriptor().getFields();
    }

    private static List<FieldDescriptor> surfaceConfigFields() {
        return TSurfaceConfig.getDescriptor().getFields();
    }

}
