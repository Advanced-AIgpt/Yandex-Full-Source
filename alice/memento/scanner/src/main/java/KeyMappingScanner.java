package ru.yandex.alice.memento.scanner;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.function.BiConsumer;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.common.collect.BiMap;
import com.google.common.collect.EnumHashBiMap;
import com.google.common.collect.HashBiMap;
import com.google.protobuf.Any;
import com.google.protobuf.DescriptorProtos;
import com.google.protobuf.Descriptors;
import com.google.protobuf.GeneratedMessage.GeneratedExtension;
import com.google.protobuf.GeneratedMessageV3;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.ProtocolMessageEnum;
import com.google.protobuf.TextFormat;
import org.apache.commons.lang3.StringUtils;
import org.springframework.beans.factory.config.BeanDefinition;
import org.springframework.context.annotation.ClassPathScanningCandidateComponentProvider;
import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;
import org.springframework.core.type.filter.AssignableTypeFilter;
import org.springframework.stereotype.Component;

import ru.yandex.alice.memento.proto.MementoApiProto;
import ru.yandex.alice.memento.proto.MementoApiProto.EDeviceConfigKey;
import ru.yandex.alice.memento.proto.MementoApiProto.TSurfaceConfig;
import ru.yandex.alice.memento.proto.MementoApiProto.TUserConfigs;

import static java.util.Objects.requireNonNull;
import static java.util.stream.Collectors.toMap;
import static java.util.stream.Collectors.toSet;
import static ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey;

// Component to make it initialize on application startup
@Component
public class KeyMappingScanner {

    private final KeyMapping<EConfigKey> userConfigKeys;
    private final KeyMapping<EDeviceConfigKey> deviceConfigKeys;

    public KeyMappingScanner() throws IOException {
        ClassPathScanningCandidateComponentProvider scanner = new ClassPathScanningCandidateComponentProvider(false);
        scanner.addIncludeFilter(new AssignableTypeFilter(GeneratedMessageV3.class));
        Set<BeanDefinition> beans = Stream.concat(
                scanner.findCandidateComponents("ru.yandex.alice.memento.proto").stream(),
                scanner.findCandidateComponents("ru.yandex.alice.protos").stream()
        ).collect(toSet());

        Set<Class<GeneratedMessageV3>> messageClasses = beans.stream()
                .map(BeanDefinition::getBeanClassName)
                .filter(Objects::nonNull)
                .map(KeyMappingScanner::findClass)
                .collect(toSet());

        BiMap<Class<GeneratedMessageV3>, Descriptors.Descriptor> classToDescriptorMap =
                HashBiMap.create(messageClasses.stream().collect(toMap(x -> x, KeyMappingScanner::getDescriptor)));

        Map<String, Descriptors.Descriptor> nameToDescriptor = classToDescriptorMap.values().stream()
                .collect(toMap(Descriptors.Descriptor::getFullName, x -> x));

        var resolver = new PathMatchingResourcePatternResolver();
        List<Resource> defaultUserConfigsResources = Arrays.asList(
                resolver.getResources("classpath*:user_configs/*.pb.txt")
        );

        List<Resource> defaultDeviceConfigsResources = Arrays.asList(
                resolver.getResources("classpath*:device_configs/*.pb.txt")
        );

        Map<Descriptors.Descriptor, GeneratedMessageV3> loadedUserConfigsDefaults = defaultUserConfigsResources.stream()
                .filter(res -> res.getFilename() != null)
                .map(res -> getDescriptorEntry(res, nameToDescriptor, classToDescriptorMap.inverse()))
                .collect(toMap(Map.Entry::getKey, Map.Entry::getValue));

        Map<Descriptors.Descriptor, GeneratedMessageV3> loadedDeviceConfigsDefaults =
                defaultDeviceConfigsResources.stream()
                        .filter(res -> res.getFilename() != null)
                        .map(res -> getDescriptorEntry(res, nameToDescriptor, classToDescriptorMap.inverse()))
                        .collect(toMap(Map.Entry::getKey, Map.Entry::getValue));

        this.userConfigKeys = scan(MementoApiProto.key, TUserConfigs.class, TUserConfigs.Builder.class,
                loadedUserConfigsDefaults, classToDescriptorMap);

        this.deviceConfigKeys = scan(MementoApiProto.surfaceKey, TSurfaceConfig.class, TSurfaceConfig.Builder.class,
                loadedDeviceConfigsDefaults, classToDescriptorMap);
    }

    public String getDbKey(EConfigKey enumKey) {
        return userConfigKeys.getDbKey(enumKey);
    }

    public String getDbKey(EDeviceConfigKey enumKey) {
        return deviceConfigKeys.getDbKey(enumKey);
    }

    public EConfigKey userConfigEnumByKey(String key) {
        return userConfigKeys.getEnumKey(key);
    }

    public EDeviceConfigKey deviceConfigEnumByKey(String key) {
        return deviceConfigKeys.getEnumKey(key);
    }

    public String typeUrlForEnum(EConfigKey key) {
        return userConfigKeys.getTypeUrl(key);
    }

    public String typeUrlForEnum(EDeviceConfigKey key) {
        return deviceConfigKeys.getTypeUrl(key);
    }

    public Any getDefaultForKey(EConfigKey key) {
        return userConfigKeys.getDefault(key);
    }

    public Any getDefaultForKey(EDeviceConfigKey key) {
        return deviceConfigKeys.getDefault(key);
    }

    public Class<GeneratedMessageV3> getClassForKey(EConfigKey key) {
        return userConfigKeys.getClass(key);
    }

    public Class<GeneratedMessageV3> getClassForKey(EDeviceConfigKey key) {
        return deviceConfigKeys.getClass(key);
    }

    public void setValue(
            EConfigKey key, GeneratedMessageV3.Builder<?> builder, GeneratedMessageV3 value
    ) {
        userConfigKeys.setField(key, builder, value);
    }

    public Map<EConfigKey, GeneratedMessageV3> getUserConfigExplicitDefaults() {
        return userConfigKeys.getExplicitDefaults();
    }

    public Map<EDeviceConfigKey, GeneratedMessageV3> getDeviceConfigExplicitDefaults() {
        return deviceConfigKeys.getExplicitDefaults();
    }

    public void setValue(
            EDeviceConfigKey key, GeneratedMessageV3.Builder<?> builder, GeneratedMessageV3 value
    ) {
        deviceConfigKeys.setField(key, builder, value);
    }

    private static <E extends Enum<E> & ProtocolMessageEnum> KeyMapping<E> scan(
            GeneratedExtension<DescriptorProtos.FieldOptions, E> extension,
            Class<? extends GeneratedMessageV3> configClass,
            Class<? extends GeneratedMessageV3.Builder<?>> builderClass,
            Map<Descriptors.Descriptor, GeneratedMessageV3> loadedExplicitDefaults,
            BiMap<Class<GeneratedMessageV3>, Descriptors.Descriptor> classToDescriptorMap
    ) {
        Class<E> clazz = extension.getDefaultValue().getDeclaringClass();

        E unrecognized = Stream.of(clazz.getEnumConstants())
                .filter(KeyMappingScanner::isUnrecognized)
                .findAny()
                .get();

        Map<E, String> keyToTypeUrl = getDescriptor(configClass).getFields().stream()
                .filter(field -> field.getOptions().hasExtension(extension))
                .collect(toMap(field -> field.getOptions().getExtension(extension),
                        field -> "type.googleapis.com/" + field.getMessageType().getFullName()));

        EnumHashBiMap<E, String> dbKeys = EnumHashBiMap.create(Stream.of(clazz.getEnumConstants())
                .collect(toMap(Function.identity(),
                        elem -> elem == unrecognized ? "unrecognized" :
                                elem.getValueDescriptor().getOptions().getExtension(MementoApiProto.dbKey)
                )));

        Map<E, Descriptors.Descriptor> enumToDescriptor = getDescriptor(configClass).getFields().stream()
                .filter(field -> field.getOptions().hasExtension(extension))
                .collect(toMap(field -> field.getOptions().getExtension(extension),
                        Descriptors.FieldDescriptor::getMessageType));

        Map<E, Class<GeneratedMessageV3>> keyToClass = enumToDescriptor.entrySet().stream()
                .collect(toMap(Map.Entry::getKey, entry -> classToDescriptorMap.inverse().get(entry.getValue())));

        Map<E, BiConsumer<GeneratedMessageV3.Builder<?>, GeneratedMessageV3>> fieldSetterMap =
                getDescriptor(configClass).getFields().stream()
                        .filter(field -> field.getOptions().hasExtension(extension))
                        .collect(toMap(field -> field.getOptions().getExtension(extension),
                                field -> makeSetter(field,
                                        keyToClass.get(field.getOptions().getExtension(extension)),
                                        builderClass
                                )
                        ));

        Map<E, Any> allDefaults = enumToDescriptor.entrySet().stream()
                .collect(Collectors.toMap(
                        Map.Entry::getKey,
                        e -> loadedExplicitDefaults.containsKey(e.getValue()) ?
                                Any.pack(loadedExplicitDefaults.get(e.getValue())) :
                                Any.newBuilder().setTypeUrl(keyToTypeUrl.get(e.getKey())).build()
                ));

        Map<E, GeneratedMessageV3> explicitDefaults = enumToDescriptor.entrySet().stream()
                .filter(e -> loadedExplicitDefaults.containsKey(e.getValue()))
                .collect(toMap(Map.Entry::getKey, e -> loadedExplicitDefaults.get(e.getValue())));


        testSetters(configClass, fieldSetterMap, allDefaults, keyToClass);


        return new KeyMapping<>(dbKeys, dbKeys.inverse(), keyToTypeUrl, unrecognized,
                allDefaults, keyToClass, fieldSetterMap, explicitDefaults);

    }

    private static <E extends Enum<E> & ProtocolMessageEnum> void testSetters(
            Class<? extends GeneratedMessageV3> configClass,
            Map<E, BiConsumer<GeneratedMessageV3.Builder<?>, GeneratedMessageV3>> fieldSetterMap,
            Map<E, Any> allDefaults,
            Map<E, Class<GeneratedMessageV3>> keyToClass
    ) {
        String key = null;
        try {
            var builder = (GeneratedMessageV3.Builder<?>) configClass.getMethod("newBuilder").invoke(null);
            for (Map.Entry<E, Any> entry : allDefaults.entrySet()) {
                key = entry.getKey().name();
                var value = entry.getValue().unpack(keyToClass.get(entry.getKey()));
                fieldSetterMap.get(entry.getKey()).accept(builder, value);
            }
        } catch (IllegalAccessException | InvocationTargetException | NoSuchMethodException |
                InvalidProtocolBufferException e) {
            throw new RuntimeException("Field setter test failed for key " + key, e);
        }
    }

    private static BiConsumer<GeneratedMessageV3.Builder<?>, GeneratedMessageV3> makeSetter(
            Descriptors.FieldDescriptor field,
            Class<GeneratedMessageV3> valueClazz,
            Class<? extends GeneratedMessageV3.Builder<?>> builderClass
    ) {
        try {
            Method m = builderClass.getMethod("set" + StringUtils.capitalize(field.getName()), valueClazz);
            return (builder, value) -> {
                try {
                    m.invoke(builder, value);
                } catch (IllegalAccessException | InvocationTargetException e) {
                    throw new RuntimeException(e);
                }
            };
        } catch (NoSuchMethodException e) {
            throw new RuntimeException(e);
        }

    }

    private Map.Entry<Descriptors.Descriptor, GeneratedMessageV3> getDescriptorEntry(
            Resource resource, Map<String, Descriptors.Descriptor> descriptorMap,
            Map<Descriptors.Descriptor, Class<GeneratedMessageV3>> descriptorToClasses
    ) {
        var name = requireNonNull(resource.getFilename())
                .replaceAll("\\.pb\\.txt$", "");
        Descriptors.Descriptor descriptor = requireNonNull(descriptorMap.get(name),
                () -> "Can't find Message descriptor for file " + resource.getFilename());

        Class<GeneratedMessageV3> clazz = requireNonNull(descriptorToClasses.get(descriptor),
                () -> "No Proto class found for file " + resource.getFilename());
        try {
            var protoMsg = TextFormat.parse(new String(resource.getInputStream().readAllBytes()), clazz);
            return Map.entry(descriptor, protoMsg);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private static <E extends Enum<E> & ProtocolMessageEnum> boolean isUnrecognized(E value) {
        try {
            value.getNumber();
            return false;
        } catch (IllegalArgumentException e) {
            return true;
        }
    }

    @SuppressWarnings("unchecked")
    private static Class<GeneratedMessageV3> findClass(String className) {
        try {
            return (Class<GeneratedMessageV3>) Class.forName(className);
        } catch (ClassNotFoundException | ClassCastException e) {
            throw new RuntimeException(e);
        }
    }

    private static Descriptors.Descriptor getDescriptor(Class<? extends GeneratedMessageV3> clazz) {
        try {
            return (Descriptors.Descriptor) clazz.getMethod("getDescriptor").invoke(null);
        } catch (IllegalAccessException | InvocationTargetException | NoSuchMethodException | ClassCastException e) {
            throw new RuntimeException(e);
        }
    }

}
